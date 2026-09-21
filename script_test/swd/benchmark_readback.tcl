# ============================================
# OpenOCD Cross-Write-Read-Verify Speed Test
# ============================================
#
# Windows-compatible, English output.
# Uses dump_image + host-side file comparison instead of verify_image
# to avoid CRC algorithm overwriting target RAM data.
#
# Usage:
#   1. Prepare two random files:
#        dd if=/dev/urandom of=test1.bin bs=1 count=65536
#        dd if=/dev/urandom of=test2.bin bs=1 count=65536
#   2. Run: openocd -f speed_test.cfg
# ============================================

# ---------- User Configuration ----------
set iterations 10
set file1 "test1.bin"
set file2 "test2.bin"
set ram_addr 0x20000000
set file_size 65536
# ----------------------------------------

source [find interface/cmsis-dap.cfg]
transport select swd
source [find target/stm32f1x.cfg]
adapter speed 60000

init

# ---------- Helper Procs ----------

proc filter_load_image_output {str} {
    set lines [split [string trim $str] "\n"]
    set filtered {}
    foreach line $lines {
        if {![string match "*bytes written at address*" $line]} {
            lappend filtered $line
        }
    }
    return [join $filtered "\n"]
}

proc extract_speed {str} {
    if {[regexp {\(([0-9.]+) KiB/s\)} $str match speed]} {
        return $speed
    }
    return 0
}

proc avg {list} {
    set count [llength $list]
    if {$count == 0} {return 0}
    set sum 0
    foreach val $list {
        set sum [expr {$sum + $val}]
    }
    return [expr {double($sum) / $count}]
}

proc get_adapter_speed {} {
    set result [adapter speed]
    if {[regexp {adapter speed:\s+(\d+)\s+kHz} $result match speed]} {
        return $speed
    }
    return -1
}

# Compare two files by reading them in binary mode.
# Returns empty string if identical, otherwise error description.
proc compare_files {file1 file2} {
    set f1 [open $file1 rb]
    set f2 [open $file2 rb]
    set data1 [read $f1]
    set data2 [read $f2]
    close $f1
    close $f2

    set len1 [string length $data1]
    set len2 [string length $data2]
    if {$len1 != $len2} {
        return "Length mismatch: $len1 vs $len2"
    }

    if {$data1 eq $data2} {
        return ""
    }

    # Find first difference using hex representation
    binary scan $data1 H* hex1
    binary scan $data2 H* hex2

    set hex_len [string length $hex1]
    for {set i 0} {$i < $hex_len} {incr i} {
        if {[string index $hex1 $i] ne [string index $hex2 $i]} {
            set byte_offset [expr {$i / 2}]
            set byte1 [string range $hex1 [expr {$byte_offset * 2}] [expr {$byte_offset * 2 + 1}]]
            set byte2 [string range $hex2 [expr {$byte_offset * 2}] [expr {$byte_offset * 2 + 1}]]
            return "First diff at byte offset $byte_offset: expected 0x$byte1, got 0x$byte2"
        }
    }
    return "Data mismatch"
}

# ---------- Pre-check ----------

foreach f [list $file1 $file2] {
    if {![file exists $f]} {
        echo "FATAL ERROR: Test file '$f' not found."
        shutdown
        return
    }
}

set size1 [file size $file1]
set size2 [file size $file2]

if {$size1 != $size2} {
    echo "WARNING: File sizes differ ($file1: $size1, $file2: $size2)"
    echo "         Using smaller size as transfer length."
    if {$size1 < $size2} {
        set file_size $size1
    } else {
        set file_size $size2
    }
} else {
    if {$file_size != $size1} {
        echo "INFO: Adjusting file_size from $file_size to actual $size1"
        set file_size $size1
    }
}

# ---------- Test Loop ----------

set read_speeds  {}
set write_speeds {}
set pass_count   0

echo ""
echo "========= Cross-Write-Read-Verify Speed Test ========="
echo "Iterations  : $iterations"
echo "File 1      : $file1 ($size1 bytes)"
echo "File 2      : $file2 ($size2 bytes)"
echo "RAM Address : [format 0x%08X $ram_addr]"
echo "Transfer Len: $file_size bytes"
echo "Adapter Spd : [get_adapter_speed] kHz"
echo "======================================================"
echo ""

# Halt target before testing to prevent firmware from corrupting RAM
echo "Halting target..."
halt

for {set i 0} {$i < $iterations} {incr i} {
    if {[expr {$i % 2}] == 0} {
        set current_file $file1
        set file_label "FILE1"
    } else {
        set current_file $file2
        set file_label "FILE2"
    }

    set round [expr {$i + 1}]
    echo "--- Round $round/$iterations ($file_label: $current_file) ---"

    # 1. Write
    set write_raw [load_image $current_file $ram_addr]
    set write_filtered [filter_load_image_output $write_raw]
    echo "Write: $write_filtered"
    set w_speed [extract_speed $write_raw]
    if {$w_speed > 0} {
        lappend write_speeds $w_speed
    }

    # Ensure target remains halted so running code does not modify RAM
    halt

    # 2. Read back to temp file for speed measurement and verification
    set dump_file "readback_${i}.bin"
    set read_raw [dump_image $dump_file $ram_addr $file_size]
    echo "Read:  $read_raw"
    set r_speed [extract_speed $read_raw]
    if {$r_speed > 0} {
        lappend read_speeds $r_speed
    }

    # 3. Verify by comparing original file and dumped file on host side.
    # This avoids verify_image's CRC algorithm which overwrites target RAM.
    set cmp_result [compare_files $current_file $dump_file]
    if {$cmp_result ne ""} {
        echo ""
        echo "FATAL ERROR: Round $round data verification FAILED!"
        echo "  File  : $current_file"
        echo "  Addr  : [format 0x%08X $ram_addr]"
        echo "  Detail: $cmp_result"
        echo ""
        catch {file delete $dump_file}
        resume
        shutdown
        return
    } else {
        echo "Verify: PASSED"
        incr pass_count
    }

    catch {file delete $dump_file}
    echo ""
}

# ---------- Summary ----------

set read_avg  [avg $read_speeds]
set write_avg [avg $write_speeds]
set current_speed [get_adapter_speed]

echo "================== Speed Test Summary =================="
echo "Adapter Request Speed: $current_speed kHz"
echo "Passed Rounds        : $pass_count / $iterations"
echo "Write Samples        : [llength $write_speeds]"
echo "Write Avg Speed      : [format %.3f $write_avg] KiB/s"
echo "Read Samples         : [llength $read_speeds]"
echo "Read Avg Speed       : [format %.3f $read_avg] KiB/s"
echo "========================================================"

echo "Resuming target..."
resume

shutdown
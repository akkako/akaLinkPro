SECTIONS
{
  . = 0x00000000;
  .fast : {
    KEEP(*(.fast))
  }

  /DISCARD/ : {
    *(.comment)
    *(.riscv.attributes)
    *(.debug*)
    *(.debug_*)
  }
}
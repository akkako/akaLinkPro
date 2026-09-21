import os
import re
import sys
import subprocess

# Generate a temporary variant of benchmark_readback.tcl with the requested
# adapter speed / iteration count and run it with OpenOCD.
#
# Usage: python run_benchmark.py <speed_khz> <iterations>
#   e.g. python run_benchmark.py 36000 100

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "benchmark_readback.tcl")


def main():
    speed = int(sys.argv[1]) if len(sys.argv) > 1 else 20000
    iters = int(sys.argv[2]) if len(sys.argv) > 2 else 100

    with open(SRC, "r", encoding="utf-8") as f:
        text = f.read()

    text = re.sub(r"^set iterations \d+", "set iterations %d" % iters, text, flags=re.M)
    text = re.sub(r"^adapter speed \d+", "adapter speed %d" % speed, text, flags=re.M)

    tmp = os.path.join(HERE, ".bench_%dk_%d.tcl" % (speed // 1000, iters))
    with open(tmp, "w", encoding="utf-8") as f:
        f.write(text)

    print("=== OpenOCD SWD benchmark: speed=%d kHz, iterations=%d ===" % (speed, iters))
    try:
        proc = subprocess.run(["openocd", "-f", tmp], cwd=HERE,
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                              text=True, errors="replace")
        out = proc.stdout
        print(out)
        rc = proc.returncode
    finally:
        try:
            os.remove(tmp)
        except OSError:
            pass

    bad = ("FATAL ERROR" in out) or ("FAILED" in out)
    print("BENCH_RESULT speed=%d rc=%d %s" % (speed, rc, "FAIL" if bad else "PASS"))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()

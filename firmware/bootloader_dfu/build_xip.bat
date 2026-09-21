@echo off
@REM ---------------------------------------------------------------------------
@REM Bootloader build (flash_xip, linked at 0x80000000).
@REM Separated into ./build_xip so the J-Link flow has its own output dir.
@REM The original build.bat is untouched.
@REM ---------------------------------------------------------------------------
@set "HPM_SDK_BASE=D:\_tools\hpm_sdk\hpm_sdk"
@set "GNURISCV_TOOLCHAIN_PATH=D:\_tools\hpm_sdk\toolchains\rv32imac_zicsr_zifencei_multilib_b_ext-win"
@set "HPM_SDK_TOOLCHAIN_VARIANT=gcc"
@set "PYTHON_EXECUTABLE=D:\_tools\hpm_sdk\tools\python3\python3"
@set "PATH=D:\_tools\hpm_sdk\tools\python3;D:\_tools\hpm_sdk\tools\cmake\bin;D:\_tools\hpm_sdk\tools\ninja;%PATH%"

@cmake -GNinja -DBOARD=akaLinkPro -DHPM_BUILD_TYPE=flash_xip -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Release -B=./build_xip .
@cmake --build ./build_xip

@echo.
@echo [build_xip] Bootloader image: build_xip\output\akaLinkPro_Boot.hex / .bin (linked at 0x80000000)

@echo off
@REM ---------------------------------------------------------------------------
@REM Application build (same memory layout as the DFU firmware).
@REM
@REM Links the APP at 0x80000000 + 0x20000 = 0x80020000 (right after the 128K
@REM bootloader) using SDK flash_dfu.ld. Output can be either:
@REM   - flashed by J-Link directly at 0x80020000 (see flash_jlink.bat), or
@REM   - transferred by dfu-util (see build.bat).
@REM No dfu-util call here, so this is safe to use for J-Link one-click flash.
@REM ---------------------------------------------------------------------------
@set "HPM_TOOLCHAIN_PATH=D:\_tools\hpm_sdk"
@REM @set "HPM_TOOLCHAIN_PATH=C:\hpm"

@set "PATH=%HPM_TOOLCHAIN_PATH%\tools\python3;%HPM_TOOLCHAIN_PATH%\tools\cmake\bin;%HPM_TOOLCHAIN_PATH%\tools\ninja;%PATH%"
@set "HPM_SDK_BASE=%HPM_TOOLCHAIN_PATH%\hpm_sdk"
@set "GNURISCV_TOOLCHAIN_PATH=%HPM_TOOLCHAIN_PATH%\toolchains\rv32imac_zicsr_zifencei_multilib_b_ext-win"
@set "HPM_SDK_TOOLCHAIN_VARIANT=gcc"
@set "BOARD=akaLinkPro"
@set "HPM_BUILD_TYPE=flash_dfu"

@cmake -G Ninja -DBOARD=%BOARD% -DHPM_BUILD_TYPE=%HPM_BUILD_TYPE% -DCMAKE_BUILD_TYPE=debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B=./build_dfu -S .
@cmake --build ./build_dfu

@echo.
@echo [build_dfu] APP image: build_dfu\output\akaLinkPro_App.hex / .bin (linked at 0x80020000)

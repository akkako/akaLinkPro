@REM @set "HPM_TOOLCHAIN_PATH=D:\_tools\hpm_sdk"
@set "HPM_TOOLCHAIN_PATH=C:\hpm"

@set "PATH=%HPM_TOOLCHAIN_PATH%\tools\python3;%HPM_TOOLCHAIN_PATH%\tools\cmake\bin;%HPM_TOOLCHAIN_PATH%\tools\ninja;%PATH%"
@set "HPM_SDK_BASE=%HPM_TOOLCHAIN_PATH%\hpm_sdk"
@set "GNURISCV_TOOLCHAIN_PATH=%HPM_TOOLCHAIN_PATH%\toolchains\rv32imac_zicsr_zifencei_multilib_b_ext-win"
@set "HPM_SDK_TOOLCHAIN_VARIANT=gcc"
@set "BOARD=akaLinkPro"
@set "HPM_BUILD_TYPE=flash_dfu"

@cmake -G Ninja -DBOARD=%BOARD% -DHPM_BUILD_TYPE=%HPM_BUILD_TYPE% -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B=./build -S .
@cmake --build ./build

@REM @dfu-util -a 0 -E 4 -s 0x80020000:leave -D build/output/akaLinkPro_App.bin
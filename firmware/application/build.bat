@set "PATH=C:\hpm\tools\python3;C:\hpm\tools\cmake\bin;C:\hpm\tools\ninja;%PATH%"
@set "HPM_SDK_BASE=C:\hpm\hpm_sdk"
@set "GNURISCV_TOOLCHAIN_PATH=C:\hpm\toolchains\rv32imac_zicsr_zifencei_multilib_b_ext-win"
@set "HPM_SDK_TOOLCHAIN_VARIANT=gcc"

@REM @set "PATH=D:\_tools\hpm_sdk\tools\python3;D:\_tools\hpm_sdk\tools\cmake\bin;D:\_tools\hpm_sdk\tools\ninja;%PATH%"
@REM @set "HPM_SDK_BASE=D:\_tools\hpm_sdk\hpm_sdk"
@REM @set "GNURISCV_TOOLCHAIN_PATH=D:\_tools\hpm_sdk\toolchains\rv32imac_zicsr_zifencei_multilib_b_ext-win"
@REM @set "HPM_SDK_TOOLCHAIN_VARIANT=gcc"

@cmake -G Ninja -DBOARD=akaLinkPro -DHPM_BUILD_TYPE=flash_dfu -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B=./build -S .
@cmake --build ./build

@REM @dfu-util -a 0 -E 4 -s 0x80020000:leave -D build/output/akaLinkPro_App.bin
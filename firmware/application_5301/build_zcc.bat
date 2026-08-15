@set "HPM_TOOLCHAIN_PATH=D:\_tools\hpm_sdk"

@set "PATH=%HPM_TOOLCHAIN_PATH%\tools\python3;%HPM_TOOLCHAIN_PATH%\tools\cmake\bin;%HPM_TOOLCHAIN_PATH%\tools\ninja;%PATH%"
@set "HPM_SDK_BASE=%HPM_TOOLCHAIN_PATH%\hpm_sdk"

@REM --- 修改开始：指向 ZCC 工具链 ---
@set "GNURISCV_TOOLCHAIN_PATH=D:\_tools\Terapines\ZCC\4.1.9"
@set "HPM_SDK_TOOLCHAIN_VARIANT=zcc"
@REM --------------------------------

@set "BOARD=akaLinkPro"
@set "HPM_BUILD_TYPE=flash_dfu"

@cmake -G Ninja -DBOARD=%BOARD% -DHPM_BUILD_TYPE=%HPM_BUILD_TYPE% -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B=./build -S .
@cmake --build ./build
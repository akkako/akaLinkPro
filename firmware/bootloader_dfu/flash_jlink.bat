@echo off
@REM ---------------------------------------------------------------------------
@REM One-click J-Link flash for the DFU bootloader.
@REM
@REM   1. Builds the bootloader (flash_xip, linked at 0x80000000)
@REM   2. Programs the bootloader sectors (loadfile erases what it writes)
@REM   3. Resets and runs. An already-flashed APP at 0x80020000 is preserved.
@REM
@REM Override the J-Link install dir by setting JLINK_EXE before running.
@REM ---------------------------------------------------------------------------
setlocal

@if not defined JLINK_EXE set "JLINK_EXE=C:\Program Files\SEGGER\JLink\JLink.exe"
@if not exist "%JLINK_EXE%" set "JLINK_EXE=C:\Program Files (x86)\SEGGER\JLink\JLink.exe"
@if not exist "%JLINK_EXE%" (
    echo [ERROR] JLink.exe not found. Set JLINK_EXE to the full path and retry.
    exit /b 1
)

@set "PROJ=%~dp0"
@REM Use the packed image: it carries the bootloader info block @0x8001F000.
@set "HEX=%PROJ%build_xip\output\akaLinkPro_Boot_pack.hex"

@REM --- 1. Build bootloader ------------------------------------------------------
call "%PROJ%build_xip.bat"
if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)
if not exist "%HEX%" (
    echo [ERROR] Output not found: %HEX%
    exit /b 1
)

@REM Give the target/USB some settle time after the build.
ping -n 2 127.0.0.1 >nul

@REM --- 2. Generate J-Link command script ----------------------------------------
@set "JLCMD=%TEMP%\akaLinkPro_boot_flash.jlink"
> "%JLCMD%" echo device HPM5301xEGx
>>"%JLCMD%" echo si JTAG
>>"%JLCMD%" echo jtagconf -1 -1
>>"%JLCMD%" echo speed 4000
>>"%JLCMD%" echo connect
>>"%JLCMD%" echo Sleep 200
@REM loadfile erases the flash sectors it writes; the APP region at 0x80020000 is preserved.
>>"%JLCMD%" echo loadfile "%HEX%"
>>"%JLCMD%" echo Sleep 200
>>"%JLCMD%" echo r
>>"%JLCMD%" echo Sleep 300
>>"%JLCMD%" echo go
>>"%JLCMD%" echo Sleep 200
>>"%JLCMD%" echo Exit

@REM --- 3. Flash ----------------------------------------------------------------
echo.
echo Flashing DFU bootloader: %HEX%
@REM -NoGui 1 prevents the J-Link GUI from popping up / blocking the script.
@REM -ExitOnError 1 makes J-Link terminate instead of waiting on errors.
"%JLINK_EXE%" -NoGui 1 -ExitOnError 1 -CommanderScript "%JLCMD%"
if errorlevel 1 (
    echo [ERROR] J-Link flash failed.
    exit /b 1
)

@REM Let the target boot and re-enumerate before returning.
ping -n 3 127.0.0.1 >nul

echo.
echo [OK] Bootloader flashed. Device will run the APP or enter DFU mode.
endlocal

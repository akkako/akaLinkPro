@echo off
@REM ---------------------------------------------------------------------------
@REM One-click J-Link flash for the application.
@REM
@REM   1. Builds the app at 0x80020000 (flash_dfu layout, see build_dfu.bat)
@REM   2. Programs only the APP region via J-Link; the bootloader at
@REM      0x80000000 is preserved.
@REM   3. Resets and runs. The bootloader validates the APP signature at
@REM      0x80020000 and jumps to it.
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
@set "HEX=%PROJ%build_dfu\output\akaLinkPro_App.hex"

@REM --- 1. Build app image -------------------------------------------------------
call "%PROJ%build_dfu.bat"
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
@set "JLCMD=%TEMP%\akaLinkPro_app_flash.jlink"
> "%JLCMD%" echo device HPM5301xEGx
>>"%JLCMD%" echo si JTAG
>>"%JLCMD%" echo jtagconf -1 -1
>>"%JLCMD%" echo speed 4000
>>"%JLCMD%" echo connect
>>"%JLCMD%" echo Sleep 200
@REM loadfile only erases the sectors it writes -> bootloader stays untouched.
>>"%JLCMD%" echo loadfile "%HEX%"
>>"%JLCMD%" echo Sleep 200
>>"%JLCMD%" echo r
>>"%JLCMD%" echo Sleep 300
>>"%JLCMD%" echo go
>>"%JLCMD%" echo Sleep 200
>>"%JLCMD%" echo Exit

@REM --- 3. Flash ----------------------------------------------------------------
echo.
echo Flashing APP (@0x80020000): %HEX%
@REM -NoGui 1 prevents the J-Link GUI from popping up / blocking the script.
@REM -ExitOnError 1 makes J-Link terminate instead of waiting on error dialogs.
"%JLINK_EXE%" -NoGui 1 -ExitOnError 1 -CommanderScript "%JLCMD%"
if errorlevel 1 (
    echo [ERROR] J-Link flash failed.
    exit /b 1
)

@REM Let the target boot and re-enumerate before returning.
ping -n 3 127.0.0.1 >nul

echo.
echo [OK] APP flashed. Device should run the application.
endlocal

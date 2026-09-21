@echo off
@REM ---------------------------------------------------------------------------
@REM DFU download of the APP image via dfu-util.
@REM
@REM   Program the already-built image at 0x80020000. Run build_dfu.bat first
@REM   (or flash_jlink.bat). dfu-util sends DFU_DETACH to the APP's DFU runtime
@REM   interface so the bootloader takes over, then transfers the image.
@REM
@REM   Uses the PACKED image (_pack.bin, with the APP integrity/version header);
@REM   the raw .bin would fail the bootloader CRC check.
@REM ---------------------------------------------------------------------------
@set "PROJ=%~dp0"

@if not exist "%PROJ%build_dfu\output\akaLinkPro_App_pack.bin" (
    echo [ERROR] Not built yet: run build_dfu.bat first.
    exit /b 1
)

dfu-util -a 0 -E 1 -s 0x80020000:leave -D "%PROJ%build_dfu\output\akaLinkPro_App_pack.bin"

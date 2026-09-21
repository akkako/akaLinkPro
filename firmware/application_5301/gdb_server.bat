@echo off
@REM ---------------------------------------------------------------------------
@REM J-Link GDB Server launcher for VSCode debugging (application_5301).
@REM Started as a background task by .vscode/tasks.json; cppdbg then attaches to
@REM localhost:2331. -singlerun makes the server exit when GDB disconnects.
@REM ---------------------------------------------------------------------------
setlocal
@if not defined JLINK_GDBSERVER set "JLINK_GDBSERVER=C:\Program Files\SEGGER\JLink\JLinkGDBServerCL.exe"
@if not exist "%JLINK_GDBSERVER%" set "JLINK_GDBSERVER=C:\Program Files (x86)\SEGGER\JLink\JLinkGDBServerCL.exe"
@if not exist "%JLINK_GDBSERVER%" (
    echo [ERROR] JLinkGDBServerCL.exe not found. Set JLINK_GDBSERVER and retry.
    exit /b 1
)
"%JLINK_GDBSERVER%" -device HPM5301xEGx -if JTAG -speed 4000 -port 2331 -nogui -singlerun %*
endlocal

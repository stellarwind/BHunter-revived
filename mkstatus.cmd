@echo off
setlocal
where py >nul 2>nul
if not errorlevel 1 (
    py -3 "%~dp0tools\mkstatus.py" %*
    exit /b
)
python "%~dp0tools\mkstatus.py" %*
exit /b

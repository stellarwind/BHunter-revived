@echo off
setlocal
python "%~dp0tools\build_server.py" %*
exit /b %errorlevel%

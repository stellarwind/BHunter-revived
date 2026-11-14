@echo off
setlocal
python "%~dp0build_server.py" %*
exit /b %errorlevel%

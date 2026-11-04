@echo off
python "%~dp0tools\build_gcc.py" %*
exit /b %errorlevel%

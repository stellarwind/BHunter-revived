@echo off
python "%~dp0tools\m0.py" build %*
exit /b %errorlevel%

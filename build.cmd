@echo off
python "%~dp0tools\build_game.py" %*
exit /b %errorlevel%

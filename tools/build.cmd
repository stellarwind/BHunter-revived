@echo off
python "%~dp0build_game.py" %*
exit /b %errorlevel%

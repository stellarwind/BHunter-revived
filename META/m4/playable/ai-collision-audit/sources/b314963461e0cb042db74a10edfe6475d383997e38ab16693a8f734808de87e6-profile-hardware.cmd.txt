@echo off
call "%~dp0play-source.cmd" --hardware --fullscreen --profile %*
exit /b %errorlevel%

@echo off
call "%~dp0play-source.cmd" --hardware --fullscreen %*
exit /b %errorlevel%

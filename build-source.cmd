@echo off
pushd "%~dp0"
python tools\build_game.py %*
set "bhunter_result=%errorlevel%"
popd
exit /b %bhunter_result%

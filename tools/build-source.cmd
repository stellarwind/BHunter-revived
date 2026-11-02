@echo off
pushd "%~dp0.."
if errorlevel 1 exit /b 1
python "%~dp0build_game.py" %*
set "bhunter_result=%errorlevel%"
popd
exit /b %bhunter_result%

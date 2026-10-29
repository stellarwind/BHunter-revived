@echo off
if not exist "%~dp0build\game\BHunter-source.exe" (
  echo Build the game first with build-source.cmd
  exit /b 2
)
pushd "%~dp0build\game"
BHunter-source.exe %*
set "bhunter_result=%errorlevel%"
popd
exit /b %bhunter_result%

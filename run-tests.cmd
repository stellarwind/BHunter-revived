@echo off
setlocal
title BHunter restored-source tests
echo BHunter restored-source integration tests
echo This is a console test build. A playable source game is not available yet.
echo.
if not exist "%~dp0build\m0\bhunter-integration.exe" (
  echo Build missing. Run build.cmd first.
  pause
  exit /b 1
)
pushd "%~dp0build\m0"
if exist "%~dp0GAME_DATA\Bhunter.bin" (
  bhunter-integration.exe "%~dp0GAME_DATA\Bhunter.bin"
) else (
  bhunter-integration.exe
)
set "BH_TEST_EXIT=%errorlevel%"
echo.
echo Test exit code: %BH_TEST_EXIT% -- zero means passed.
popd
pause
exit /b %BH_TEST_EXIT%

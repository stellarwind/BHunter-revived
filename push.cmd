@echo off
setlocal
where py >nul 2>nul
if errorlevel 1 goto use_python
py -3 -c "import sys; sys.exit(sys.version_info < (3, 10))" >nul 2>nul
if errorlevel 1 goto use_python
py -3 -B "%~dp0tools\push_status.py" %*
goto finish
:use_python
python -B "%~dp0tools\push_status.py" %*
:finish
set "push_status_result=%errorlevel%"
if not "%push_status_result%"=="0" echo Command failed with exit code %push_status_result%.
exit /b %push_status_result%

@echo off

REM Change to bin directory
cd "%~dp0\..\bin"

echo S-ONNX Compiler Run Script
echo ========================
echo 1. Run Test Case 1
echo 2. Run All Test Cases
echo 3. Show Help Information
echo 4. Exit

echo.
set /p choice=Please select an operation: 

echo.
if "%choice%"=="1" goto run_test1
if "%choice%"=="2" goto run_all_tests
if "%choice%"=="3" goto show_help
if "%choice%"=="4" goto exit

goto invalid_choice

:run_test1
echo Running Test Case 1...
echo ==================
sonnx-compiler "%~dp0\..\test\test1.txt"
goto end

:run_all_tests
echo Running All Test Cases...
echo ===================
for /l %%i in (1,1,10) do (
    echo.
    echo Running Test Case %%i...
    sonnx-compiler "%~dp0\..\test\test%%i.txt"
    echo -------------------
)
goto end

:show_help
echo S-ONNX Compiler Help Information
echo ===================
sonnx-compiler -h
goto end

:invalid_choice
echo Invalid choice, please run the script again.
goto end

:exit
echo Exiting script...
goto end

:end
echo.
echo Operation completed, press any key to exit...
pause >nul
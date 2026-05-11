@echo off
cd /d "%~dp0"

where python >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] Python not found, please install Python first.
    pause
    exit /b 1
)

echo [1/2] Installing dependencies...
pip install -r requirements.txt -q
if %errorlevel% neq 0 (
    echo [ERROR] Failed to install dependencies.
    pause
    exit /b 1
)

echo [2/2] Starting server...
echo.
echo ========================================
echo   Server running at http://localhost:5000
echo   Press Ctrl+C to stop
echo ========================================
echo.
python app.py
pause

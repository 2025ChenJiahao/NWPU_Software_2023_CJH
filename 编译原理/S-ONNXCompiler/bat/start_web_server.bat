@echo off

REM Start the S-ONNX Compiler Web Server

cd "%~dp0\.."

echo Starting S-ONNX Compiler Web Server...
echo ===================================
echo The server will run on http://localhost:5000
echo Press Ctrl+C to stop the server

echo.
echo Installing required dependencies...
pip install -r web\requirements.txt

echo.
echo Starting server...
cd web
waitress-serve --host=localhost --port=5000 wsgi:app

pause
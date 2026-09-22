@echo off
cd /d "%~dp0"
python KONTAKTSerial.py %*
if errorlevel 1 pause

@echo off
cd /d "%~dp0"
python KONTAKTSerial_CLI.py %*
if errorlevel 1 pause

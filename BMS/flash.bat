@echo off

REM Get firmware file from argument, or use the default
if "%~1"=="" (
    set "firmware_file=bms.hex"
) else (
    set "firmware_file=%~1"
)

REM Check that the firmware file exists
if not exist "%firmware_file%" (
    echo File not found: %firmware_file%
    pause
    exit /b 1
)

REM Get the extension of the firmware file
for %%i in ("%firmware_file%") do set "extension=%%~xi"
set "extension=%extension:~1%"

REM Verify that the firmware file is a .hex
if /i not "%extension%"=="hex" (
    echo File is not a .hex file
    pause
    exit /b 1
)

REM Check that the uv command is installed
uvx --version >nul 2>nul
if errorlevel 1 (
    echo uv is not installed or was not found in your PATH
    echo Install it with: winget install --id=astral-sh.uv -e
    echo or: powershell -ExecutionPolicy ByPass -c "irm https://astral.sh/uv/install.ps1 | iex"
    echo or choose one of the alternative installation methods documented at https://docs.astral.sh/uv/getting-started/installation/
    pause
    exit /b 1
)

REM Check that NSING.N32L40x_DFP.1.0.1.pack exists
if not exist "NSING.N32L40x_DFP.1.0.1.pack" (
    echo NSING.N32L40x_DFP.1.0.1.pack not found
    echo Download it from https://github.com/michalmo/floatwheel/raw/refs/heads/bms-vesc/BMS/NSING.N32L40x_DFP.1.0.1.pack
    pause
    exit /b 1
)

REM Delimiter to separate the output
set "delimiter========"

REM Flash the firmware
echo %delimiter%
echo Flashing the new firmware...
echo %delimiter%
uvx --managed-python --python 3.13 pyocd@0.35.1 load "%firmware_file%" -t N32L403KB --pack NSING.N32L40x_DFP.1.0.1.pack

echo %delimiter%
echo Please check the flashing messages to make sure the flash was successful.
echo Float on!
echo %delimiter%

pause

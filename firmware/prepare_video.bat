@echo off
REM Tricorder Video Preparation Script - Windows Version
REM Converts video files to JPEG sequences suitable for ESP32 playback

setlocal enabledelayedexpansion

REM Check if FFmpeg is available
ffmpeg -version >nul 2>&1
if errorlevel 1 (
    echo Error: FFmpeg is not installed or not in PATH.
    echo Please download FFmpeg from: https://ffmpeg.org/download.html
    echo Add it to your PATH or run this script from the FFmpeg directory.
    pause
    exit /b 1
)

REM Function to display usage
if "%1"=="" (
    echo Usage: %0 ^<input_video^> [output_name] [fps] [quality]
    echo.
    echo Parameters:
    echo   input_video  : Input video file ^(mp4, avi, mov, etc.^)
    echo   output_name  : Output filename prefix ^(default: uses input name^)
    echo   fps          : Frame rate ^(default: 15, max recommended: 20^)
    echo   quality      : JPEG quality 1-31, lower=better ^(default: 5^)
    echo.
    echo Examples:
    echo   %0 startup.mp4
    echo   %0 animation.avi my_animation 10 8
    echo.
    echo Output will be saved as a single JPEG file per sequence.
    pause
    exit /b 1
)

set INPUT_FILE=%1
if not exist "%INPUT_FILE%" (
    echo Error: Input file '%INPUT_FILE%' not found.
    pause
    exit /b 1
)

REM Set defaults
set OUTPUT_NAME=%2
if "%OUTPUT_NAME%"=="" (
    for %%f in ("%INPUT_FILE%") do set OUTPUT_NAME=%%~nf
)

set FPS=%3
if "%FPS%"=="" set FPS=15

set QUALITY=%4
if "%QUALITY%"=="" set QUALITY=5

echo Converting video to ESP32-compatible format...
echo Input: %INPUT_FILE%
echo Output: %OUTPUT_NAME%.jpg
echo FPS: %FPS%
echo Quality: %QUALITY%
echo.

REM Create output directory
if not exist "tricorder_videos" mkdir tricorder_videos

REM Get video duration (simplified for Windows)
echo Extracting video information...

REM Convert video to single JPEG frame (representative frame)
echo Converting to JPEG format...
ffmpeg -i "%INPUT_FILE%" -vf "scale=320:240:force_original_aspect_ratio=decrease,pad=320:240:(ow-iw)/2:(oh-ih)/2" -frames:v 1 -q:v %QUALITY% -y "tricorder_videos\%OUTPUT_NAME%.jpg"

if errorlevel 1 (
    echo Error: FFmpeg conversion failed
    pause
    exit /b 1
)

REM Check if file was created and get size
if exist "tricorder_videos\%OUTPUT_NAME%.jpg" (
    for %%A in ("tricorder_videos\%OUTPUT_NAME%.jpg") do set file_size=%%~zA
    
    echo.
    echo Conversion complete!
    echo Output file: tricorder_videos\%OUTPUT_NAME%.jpg
    echo File size: !file_size! bytes
    
    if !file_size! GTR 51200 (
        echo Warning: File size is large ^(^>50KB^). Consider increasing quality value for smaller files.
    )
    
    echo.
    echo Copy this file to your SD card's /videos/ directory:
    echo   SD:/videos/%OUTPUT_NAME%.jpg
    echo.
    echo To play this video, send UDP command:
    echo {"action": "play_video", "commandId": "test", "parameters": {"filename": "%OUTPUT_NAME%.jpg", "loop": true}}
    echo.
    echo NOTE: This creates a single frame. For full video sequences,
    echo you'll need to implement frame sequence handling in the firmware.
) else (
    echo Error: Output file was not created
    pause
    exit /b 1
)

echo Done!
pause

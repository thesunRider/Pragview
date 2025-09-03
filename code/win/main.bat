@echo off
color 0a
setlocal enabledelayedexpansion
title File copyer

echo.
echo.
echo Please wait while converting files...

REM Source and destination folders
set "SOURCE=%cd%\src"
set "DEST=%cd%\dst"


set PATH=%PATH%;%cd%\imgmagick\;%cd%\curl\
del /Q "%cd%\dst\*"

REM Create destination folder if it doesn't exist
if not exist "%DEST%" mkdir "%DEST%"

REM Loop through all JPG/JPEG files in source folder
for %%F in ("%SOURCE%\*.jpg") do (
    REM Get image width and height
    for /f "tokens=1,2" %%a in ('magick identify -format "%%w %%h" "%%F"') do (
        set WIDTH=%%a
        set HEIGHT=%%b

        REM Check if portrait or landscape
        if !HEIGHT! GTR !WIDTH! (
            REM Portrait: resize to 240x320 max
            magick  "%%F" -resize 960x640^> "%DEST%\%%~nxF"
        ) else (
            REM Landscape: resize to 320x240 max
            magick  "%%F" -resize 640x960^> "%DEST%\%%~nxF"
        )
    )
)


for %%F in ("%SOURCE%\*.jpeg") do (
    REM Get image width and height
    for /f "tokens=1,2" %%a in ('magick identify -format "%%w %%h" "%%F"') do (
        set WIDTH=%%a
        set HEIGHT=%%b

        REM Check if portrait or landscape
        if !HEIGHT! GTR !WIDTH! (
            REM Portrait: resize to 240x320 max
            magick "%%F" -resize 960x640^> "%DEST%\%%~nxF"
        ) else (
            REM Landscape: resize to 320x240 max
            magick  "%%F" -resize 640x960^> "%DEST%\%%~nxF"
        )
    )
)

echo Files converted
set /p ipaddr=Enter Ip address of device:
echo.
echo Please wait uploading files.
for /f "delims=" %%f in ('dir /b /s "%cd%\dst\"') do (
echo Uploading %%f
curl -T "%%f" -u pragview:1234 ftp://%ipaddr%:21/ --progress-bar
)
echo.
echo Done. Enjoy
pause
exit
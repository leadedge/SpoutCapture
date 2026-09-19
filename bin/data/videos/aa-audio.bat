@echo off
rem

rem
rem To add audio to a video file produced by SpoutRecorder
rem

echo.
echo.
echo ADD AUDIO TO VIDEO
echo.
echo Video file and Audio file in this folder
echo.

set /P videoname=Video file name (without extension) : 
if "%videoname%"=="" goto Error

set /P ext=Video type (e.g. mp4, avi, mov) : 
if "%ext%"=="" goto Error

set /P audiofile=Audio file to add (with extension) : 
if "%audiofile%"=="" goto Error

..\\FFMPEG\\ffmpeg -hwaccel auto -i "%videoname%.%ext%" -i "%audiofile%" -c:v copy -map 0:v -map 1:a -shortest -y "%videoname%_out.%ext%"

goto End

:Error
echo Incorrect entry..
:End

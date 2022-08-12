@ECHO OFF
"%~dp0png2webp.exe" -v -- %*
IF %ERRORLEVEL% NEQ 0 PAUSE
EXIT /B %ERRORLEVEL%

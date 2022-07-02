@ECHO OFF
"%~dp0png2webp.exe" -rv -- %*
IF %ERRORLEVEL% NEQ 0 PAUSE
EXIT /B %ERRORLEVEL%

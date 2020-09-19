"%~dpn0.exe" -bv -- %*
IF %ERRORLEVEL% NEQ 0 (
  PAUSE
  EXIT /B %ERRORLEVEL%
)

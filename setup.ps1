$ErrorActionPreference = "Stop"
# Delegate to the shared Windows entry point.
& "$PSScriptRoot\setup.bat" @args
exit $LASTEXITCODE

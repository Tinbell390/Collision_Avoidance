@echo off
setlocal
echo ========================================
echo CollisionAvoidance setup
echo ========================================

set PYTHON=
where py >nul 2>nul
if not errorlevel 1 (
  py -3.12 -c "import sys; sys.exit(0 if sys.version_info >= (3,11) else 1)" >nul 2>nul
  if not errorlevel 1 set PYTHON=py -3.12
)
if not defined PYTHON (
  where py >nul 2>nul
  if not errorlevel 1 (
    py -3.11 -c "import sys; sys.exit(0 if sys.version_info >= (3,11) else 1)" >nul 2>nul
    if not errorlevel 1 set PYTHON=py -3.11
  )
)
if not defined PYTHON (
  python -c "import sys; sys.exit(0 if sys.version_info >= (3,11) else 1)" >nul 2>nul
  if not errorlevel 1 set PYTHON=python
)
if not defined PYTHON (
  echo ERROR: Python 3.11 or newer not found.
  echo Install Python from https://www.python.org/downloads/
  echo Enable the Python launcher and Add Python to PATH during installation.
  exit /b 1
)
%PYTHON% setup.py %*
exit /b %errorlevel%

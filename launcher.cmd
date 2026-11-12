@echo off
rem Windows: build the launcher if needed (no disc needed for that) and open it. Double-click me.
rem launcher.cmd --clean: start from nothing first (keeps your saves; docs\BUILDING.md).
rem The launcher picks your ISO and the options, then Runs or Builds the game (docs\BUILDING.md).
cd /d "%~dp0"
call build.cmd --launcher %*
if errorlevel 1 pause

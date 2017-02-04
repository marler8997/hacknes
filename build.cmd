
@set HACKNES_OPTIONS=
@set HACKNES_SOURCE=cartridge.cpp ppu.cpp cpu.cpp hacknes.cpp
@set HACKNES_LIBS=

@rem Setup graphics library
@if "%1"=="" goto USAGE_EXIT
@if "%1"=="opengl" (
  set HACKNES_SOURCE=graphics\opengl.cpp %HACKNES_SOURCE%
) else (
if "%1"=="gdi" (
  set HACKNES_SOURCE=graphics\gdi.cpp %HACKNES_SOURCE%
  set HACKNES_LIBS=%HACKNES_LIBS% user32.lib gdi32.lib
) else (
  echo Error: unknown backend "%1"
  goto EXIT
))

cl /Fehacknes /I. %HACKNES_OPTIONS% %HACKNES_SOURCE% %HACKNES_LIBS%
@goto EXIT

:USAGE_EXIT
@echo Usage: build opengl^|gdi

:EXIT

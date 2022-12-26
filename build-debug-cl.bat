@echo off
setlocal
if not exist build mkdir build
pushd build

if "%1" equ "" (
	set project=engine_test
) else (
	set project=%1
)

cl /nologo %* "../src/engine/engine.c" "../src/engine/platform_win32.c" "../src/%project%/game.c"^
	/I"../include" /I"../src" /Zi /DDEBUG /std:c11^
	/link /OUT:game.exe /PDB:game.pdb /DEBUG

popd
endlocal
@echo on
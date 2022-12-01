@echo off
setlocal
if not exist build mkdir build
pushd build

cl /nologo %* "../src/game/engine.c" "../src/game/game.c" "../src/game/platform_win32.c"^
	/I"../include" /I"../src" /Zi /DDEBUG /std:c11^
	/link /OUT:game.exe /PDB:game.pdb /DEBUG

popd
endlocal
@echo on
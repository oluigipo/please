@echo off
setlocal
if not exist build mkdir build
pushd build

set CFLAGS=/I"../include" /I.. /I"../src" /Zi /DDEBUG /std:c11

cl /nologo "../src/gamepad_db_gen/main.c" %CFLAGS% /link /out:gamepad_db_gen.exe /PDB:gamepad_db_gen.pdb /DEBUG &&^
gamepad_db_gen ../src/ext/gamecontrollerdb.txt internal_gamepad_map_database.inc &&^
cl /nologo "../src/game/engine.c" "../src/game/game.c" "../src/game/platform_win32.c" %CFLAGS% /link /OUT:game.exe /PDB:game.pdb /DEBUG

popd
endlocal
@echo on
cl "src/game/engine.c" "src/game/game.c" "src/game/platform_win32.c"^
	/Iinclude /Isrc /Zi /DDEBUG /std:c11^
	/link /OUT:game.exe /PDB:game.pdb /DEBUG

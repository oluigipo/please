cl "src/engine.c" "src/game.c" "src/platform_win32.c"^
	/Iinclude /Zi /DDEBUG^
	/link /OUT:game.exe /PDB:game.pdb /DEBUG

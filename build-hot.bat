@echo off
setlocal

make objs CUSTOMC="-DINTERNAL_ENABLE_HOT -Wno-dll-attribute-on-redeclaration" DEBUG="-g -DDEBUG"
IF "%ERRORLEVEL%" NEQ "0" GOTO :ende

set FLAGS=-Wl,/NOENTRY,/DLL -lkernel32 -fuse-ld=lld -llibvcruntime -llibucrt -g

IF EXIST build\game.exe COPY /B build\game.exe+NUL build\game.exe >NUL
IF "%ERRORLEVEL%" EQU "0" (
	clang build/engine.o build/platform_win32.o -o build/engine.dll %FLAGS%
	clang empty.c build/engine.lib -o build/game.exe -Wl,/subsystem:windows -fuse-ld=lld -g
)
clang build/engine.lib build/game.o -o build/game.dll %FLAGS%

:ende
endlocal
@echo on

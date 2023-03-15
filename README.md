# please
made to be simple, easy to build, and mainly to learn. basically my experimentation playground.

## License (Public Domain)
Everyone is free to use my code to do whatever they want.
The license note in the file LICENSE is valid just for some of the stuff in `src`.
External code (inside `src/ext`) might have other licenses.

## Build
Just do `cl build.c` or `clang build.c -o build.exe`. It will generate a `build.exe` file which you can run to build the projects. Notice that the build system will use the same compiler you did to build it.
To build the project `game_test` with debug info and UBSan, you would do: `build game_test -g -ubsan`.
A release build would look like: `build game_test -O2 -ndebug -rc`.

Run `build --help` for help.

### tools required to build:
* Clang or MSVC: C & C++ Compiler;

### optional tools:
* `llvm-rc` or `rc`: Resource (`.rc`) files compiler. Needed if passing the `-rc` flag;
* `fxc`: HLSL compiler. Needed if you change any of the shaders;

## Folder Structure
* `build`: The default build directory. It is automatically generated;
* `include`: Where we place generated or third-party headers (such as `discord_game_sdk.h`);
* `src`: All the source code for the game, dependencies, and tools needed. It is added to the include folder list. Each subfolder is its own "subproject";
* `src/game_*`: game code;
* `src/ext`: External code distributed under it's own license;
* `src/gamepad_db_gen`: A simple tool to parse SDL's `gamecontrollerdb.txt` and generate a `gamepad_map_database.inc`;

## APIs Used
* Graphics: OpenGL 3.3, Direct3D 11 (FT 9_1 to 11_0);
* Gamepad Input: DirectInput, XInput;
* Audio: WASAPI, ALSA;
* System: Win32, Linux (X11);

## External
Dependencies:
* [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h) (src/ext/stb_image.h)
* [stb_truetype](https://github.com/nothings/stb/blob/master/stb_truetype.h) (src/ext/stb_truetype.h)
* [stb_vorbis](https://github.com/nothings/stb/blob/master/stb_vorbis.c) (src/ext/stb_vorbis.h)
* [CGLM](https://github.com/recp/cglm) (src/ext/cglm)

Others:
* [GLFW](https://github.com/glfw/glfw): Some of the code in this repository was borrowed and modified from GLFW (see file `src/ext/guid_utils.h`). Though GLFW in general is a really nice reference.
* [SDL_GameControllerDB](https://github.com/gabomdq/SDL_GameControllerDB): It was used to generate the `include/internal_gamepad_map_database.inc` file.
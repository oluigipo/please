# please
made to be simple, easy to build, and mainly to learn.

## License
Everyone is free to use my code to do whatever they want.
The license note in the file LICENSE is valid just for some of the stuff in `src`.
External code (inside `src/ext`) might have other licenses.

## Build
Simplest way: If you are on Windows, simply run `build-debug-cl.bat` through VS's Developer Command Prompt x64 and it should compile normally. If you are on Linux, then run `build-debug-gcc.sh` instead. Both of these scripts have only 1 command. You can read them to understand what is needed to compile.

There's also the `build.ninja` file. If you want to use it, make sure you have Clang installed on your Windows machine.

#### Config macros
Some macros you can `-D` or `-U` to enable/disable stuff. Check out the `src/internal_config.h` file. It has the default options and conditions for each of them.
* `INTERNAL_ENABLE_OPENGL`: Enables the OpenGL 3.3 backend. For now it won't compile with this disabled;
* `INTERNAL_ENABLE_D3D11`: Enables the ~~not working~~ Direct3D 11 backend;

## Folder Structure
* `build`: The default build directory. It is automatically generated;
* `include`: Where we place generated or third-party headers (such as `discord_game_sdk.h`);
* `src`: All the source code for the game, dependencies, and tools needed. It is added to the include folder list. Each subfolder is its own "subproject". Common stuff such as basic types, arena, etc. are all shared through the `common_*.h` headers;
* `src/game`: The main game folder containing the platform layer, basic rendering and audio mixing, and the game code;
* `src/ext`: External code distributed under it's own license;
* `src/gamepad_db_gen`: A simple tool to parse SDL's `gamecontrollerdb.txt` and generate a `gamepad_map_database.inc`;

## APIs
* Graphics: OpenGL 3.3;
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
* [GLFW](https://github.com/glfw/glfw): Some of the code in this repository was borrowed and modified from GLFW (see file `include/guid_utils.h`). Though GLFW in general is a really nice reference.
* [SDL_GameControllerDB](https://github.com/gabomdq/SDL_GameControllerDB): It was used to generate the `include/internal_gamepad_map_database.inc` file.
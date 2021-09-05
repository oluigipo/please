# please
made to be simple, easy to build, and mainly to learn.

## License
Everyone is free to use my code to do whatever they want.
The license note in the file LICENSE is valid just for the stuff in the `src` folder.
Everything in the `src` folder is either mine, or is available in the public domain.

The code inside the `include` folder may be redistributed under it's own license (generally found at the top of the file or in a `LICENSE` file).

## Build
If you are on Windows, simply run `build-debug-cl.bat` through VS's Developer Command Prompt and it should compile normally. If you are on Linux, then run `build-debug-gcc.sh` instead.
Both of those scripts have only 1 command. You can read it to understand what is needed to compile.

You may also have noticed the `Makefile` file. If you want to use it, make sure you have Clang installed on your machine.
Here are some variables you can set (all of them are optional):
* `CUSTOMC` and `CUSTOMLD`: Custom flags for the compiler and the linker respectivelly;
* `OPT`: Optimization flags. You can also set it to `-g -DDEBUG` to generate debugging symbols for example;
* `BUILDDIR`: The build directory where the object files and executable are going to be placed at. Defaults to `build`;
* `PLATFORM`: The platform you want to build to. It's worth noting that you should setup your own environment for cross-compilation;
* `CC` and `LD`: C99 Compiler and linker respectivelly.

## APIs
* Graphics: OpenGL 3.3;
* Gamepad Input: DirectInput, XInput;
* Audio: WASAPI, ALSA;
* Platforms: Win32, X11;

## External
Dependencies (my hope is that I replace them with my own implementation eventually... surely not in the near future):
* [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h) (src/ext/stb_image.h)
* [stb_truetype](https://github.com/nothings/stb/blob/master/stb_truetype.h) (src/ext/stb_truetype.h)
* [stb_vorbis](https://github.com/nothings/stb/blob/master/stb_vorbis.c) (src/ext/stb_vorbis.h)
* [CGLM](https://github.com/recp/cglm) (include/cglm)

Others:
* [GLFW](https://github.com/glfw/glfw): Some of the code in this repository was borrowed and modified from GLFW (see file `include/guid_utils.h`). Though GLFW in general is a really nice reference.
* [SDL_GameControllerDB](https://github.com/gabomdq/SDL_GameControllerDB): It was used to generate the `include/internal_gamepad_map_database.inc` file.
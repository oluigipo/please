# please
no description yet

## License
Everyone is free to use my code to do whatever they want.
The license note in the file LICENSE is valid just for some of the stuff in the `src` folder.

Exclusions:
- Pieces of code I've directly borrowed from GLFW's source code in the file `src/platform_win32_input.c`. All of them have a comment specifying where and which license that code is under.

Everything else in the `src` folder is either mine, or is available in the public domain.

#### Little Note
If you have any problems about this license, or if I am redistributing your code without permission, please make an issue or contact me directly!

## Build
Clang is required, but you can use GCC if you want to. Just run `make` and you should be good to go.
If you need to, here are some variables you can set:
* `MODE=debug`: will define the macro `DEBUG` and generate debugging symbols;
* `ARCH=`: can be `x86` or `x64`, though the default is `x64`;

Example: `make MODE=debug ARCH=x86 -j`

## APIs
* Graphics: OpenGL;
* Gamepad Input: DirectInput, XInput;

## External
* [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h)
* [stb_truetype](https://github.com/nothings/stb/blob/master/stb_truetype.h)
* [CGLM](https://github.com/recp/cglm)
* [GLFW](https://github.com/glfw/glfw): Some of the code in this repository was borrowed and modified from GLFW. Though GLFW in general is a really nice reference.
* [SDL_GameControllerDB](https://github.com/gabomdq/SDL_GameControllerDB): It was used to generate the `include/internal_gamepad_map_database.inc` file.
# please
no description yet

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
* [GLFW](https://github.com/glfw/glfw): Some of the code in this repository was borrowed and modified from GLFW. Though GLFW in general is a really nice reference.
* [SDL_GameControllerDB](https://github.com/gabomdq/SDL_GameControllerDB): It was used to generate the `src/internal_gamepad_map_database.inc` file.
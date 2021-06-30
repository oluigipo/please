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

## External Libraries
* [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h)
* [stb_truetype](https://github.com/nothings/stb/blob/master/stb_truetype.h)
* [GLFW](https://github.com/glfw/glfw): Some of the code in this repository was borrowed and modified from GLFW.
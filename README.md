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

For an Android build, you'll need some more stuff, such as the Android SDK (`platforms;android-26`), Android build-tools, Android NDK (I use r23c), and a JDK. Please read `tools/build_android.c` to check what `ENV_` defines you'll need to build in your environment. Defining `ENV__DEFAULT` uses my personal configuration.
After all that, you can run `clang tools/build_android.c -o build-android.exe -DENV_ANDROID_SDK=... -DENV_ANDROID_NDK=...`, and then `build-android -embed -install`.

## Folder Structure
* `build`: The default build directory. It is automatically generated;
* `include`: Where we place generated or third-party headers (such as `discord_game_sdk.h`);
* `src`: A pool of TUs needed by one or more projects. Each TU is defined by a `tuname.c` file and a bunch of `tuname_*.c` subfiles;
* `src/game_*`: game code;
* `src/ext`: External code distributed under it's own license;
* `src/gamepad_db_gen`: A simple tool to parse SDL's `gamecontrollerdb.txt` and generate a `gamepad_map_database.inc`;
* `tools/`: Miscellaneous utilities and manifest files;

## APIs Used
### Windows:
* OpenGL 3.3;
* Direct3D 11 (FT 9_1 to 11_0);
* DirectInput;
* XInput;
* WASAPI;
### Android:
* OpenGL ES 3.0;
* AAudio;
* android-native-app-glue;
### Desktop Linux (currently not working):
* OpenGL 3.3;
* X11;
* ALSA;

## External
Dependencies:
* [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h): src/ext/stb_image.h
* [stb_truetype](https://github.com/nothings/stb/blob/master/stb_truetype.h): src/ext/stb_truetype.h
* [stb_vorbis](https://github.com/nothings/stb/blob/master/stb_vorbis.c): src/ext/stb_vorbis.h
* [CGLM](https://github.com/recp/cglm): src/ext/cglm/

Others:
* [GLFW](https://github.com/glfw/glfw): Some of the code in this repository was borrowed and modified from GLFW (see file `src/ext/guid_utils.h`). Though GLFW in general is a really nice reference.
* [SDL_GameControllerDB](https://github.com/gabomdq/SDL_GameControllerDB): It was used to generate the `include/internal_gamepad_map_database.inc` file.
* [Android Native Example](https://github.com/mmozeiko/android-native-example): Used as a starting point for the Android port.
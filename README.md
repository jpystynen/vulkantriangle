vulkantriangle
=========

Simple triangle with Vulkan on Windows. Tested only with Nvidia GTX 970.
Debug has Vulkan validation layer enabled.

![alt text](screenshots/vulkantriangle.png?raw=true)

Building
--------

The application uses Window classes for window creation. Does not compile on other platforms.

### Dependencies

* [Vulkan SDK][vksdk]: LunarG Vulkan SDK for vulkan.
* [CMake][cmake]: For generating compilation targets.
* [Visual Studio][vstudio]: For compiling (tested with community).

### Build steps

```sh
git clone https://github.com/jpystynen/vulkantriangle.git
cd vulkantriangle
.\combile_shaders.bat
```
Run cmake.
Build with VS.
Set working dir to project root and run project.

[vksdk]: https://www.lunarg.com/vulkan-sdk/
[cmake]: https://cmake.org/
[vstudio]: https://www.visualstudio.com/vs/community/

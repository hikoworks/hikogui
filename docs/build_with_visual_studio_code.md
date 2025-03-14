Building with Visual Studio Code
================================

Install requirements
--------------------
For the best experience install Visual Studio before Visual Studio Code.

 - The latest **Microsoft Visual Studio (Preview)** from <https://visualstudio.microsoft.com/>
   with the following options installed:
   - C++ core desktop features
   - C++ CMake tools
   - Windows 10 (or newer) SDK
 - **git** from: <https://git-scm.com/>
 - The **Vulkan SDK** from: <https://www.lunarg.com/vulkan-sdk/>
   with the following options installed:
   - The Vulkan SDK Core (Always Installed)
   - Vulkan Memory Allocator header.
 - optional: **RenderDoc** (for Vulkan debugging) from: <https://renderdoc.org/>
 - optional: **Doxygen** (for documentation generation) from: <https://www.doxygen.nl/>
 - **Visual Studio Code** from <https://code.visualstudio.com/>
   with the following extensions installed:
   - C/C++ (Microsoft)
   - C/C++ Extension Pack (Microsoft)
   - C/C++ Themes (Microsoft)
   - C/C++ TestMate Legacy (Mate Pek)
   - CMake (twxs)
   - CMake Tools (Microsoft)
   - Test Adapter Convert (Microsoft)
   - Test Explorer UI (Holger Benl)

Setup Visual Studio
-------------------

We offer a curated list of required Visual Studio components in `.github\.vsconfig`.
You can import this file using the Visual Studio Installer.

Setup VSCode
------------

We offer a curated list of C++ project extensions in `.vscode\extensions.json`.
To activate automatic installation of these recommendations,
modify `"extensions.ignoreRecommendations"` to `false` in your `.vscode\settings.json`.
This will allow receiving our extension suggestions.
After installation, you might want to set it back to `true` to avoid
continuous recommendation notifications during development.

Clone the HikoGUI project from github
-------------------------------------
Clone the HikoGUI repository on your machine:

```bash
git clone git@github.com:hikogui/hikogui.git
```

Building and running
--------------------
You can then open the `hikogui` directory with **Open Folder...**
inside Visual Studio Code.

To build:
 1. In the bottom status bar select `[MSVC-x64-Debug]`, `[Library + Demo]` and
    `[hikogui_demo]`.
 2. In the bottom status bar press the `Build` option to build.
 3. Press the little turtle or play button from the bottom status bar to debug
    or run the demo.

_Note: A "Window Security Alert" may show up, this is due to the RenderDoc API
creating a network server so that the RenderDoc application can remotely
communicate to the hikogui_demo application._

Other configurations that are possible to build:
 - `[MSVC-x64-Debug]` Make a debug build
 - `[MSVC-x64-RelWithDebInfo]` Make an fully optimized build
 - `[MSVC-x64-Release]` Make a fully optimized build and build all examples and
   demos.
 - `[MSVC-Analysis]` Build with static-analysis.

Common build problems:
 - If you get "permission denied", this may be due to a still running
   process like a hung hikogui_test.exe. Terminate that process using the
   Task Manager.
 - You may also get "permission denied" when during building a compiler-crash
   causes a file to be written without any permissions associated at all.
   This can be solved by rebooting the computer and followed by deleting the
   CMake cache.
 - There are two **Test Explorer** due to two test extensions being installed.
   Only the **Test Explorer** with more details will be able to debug
   individual tests.

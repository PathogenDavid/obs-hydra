To avoid dealing with getting a build environment set up for OBS, I've included `obs.lib` directly in this repository. (Although it'd be really nice if it was included with OBS installations - it's only 300 KB.)

Here's how I built OBS:
* Download the [dependency bundle from OBS](https://obsproject.com/downloads/dependencies2017.zip).
* Extract it somewhere.
* Open the CMake GUI (At the time of writing, I used CMake 3.11.0).
* Select the `lib/obs-studio` folder for the source code.
* Select `lib/obs-studio/build` for the binaries.
* Add a `PATH` entry called `DepsPath` pointing to your extracted `dependencies2017/win64/include` folder.
* Add a `BOOL` entry called `DISABLE_UI` and enable it.
* Click `Configure`
* Allow CMake to make the output directory if it asks
* Select `Visual Studio 15 2017 Win64` as the generator, use default native compilers, and click `Finish`.
* After configuration is complete, click `Generate` then `Open Project`
* Build `libobs` (If you build the whole solution, some of the projects we don't care about are going to fail.)
* Finally, copy the `obs.lib` file out of `lib/obs-studio/build/libobs/Debug` to this folder.

(It does not matter that we are using the debug build since the library file is the same between configurations.)

For the sake of convenience I've included obs.lib in this repository. (It'd be really nice if it was included with official OBS builds since it's only 300 KB, but w/e.)

It was built by following [the official build instructions](https://github.com/obsproject/obs-studio/wiki/build-instructions-for-windows) using `CI/build-windows.ps1`. (At glance this does not seem to pollute the environment or anything untoward.)

After building, copy `obs-studio/build64/libobs/RelWithDebInfo/obs.lib` to this directory.

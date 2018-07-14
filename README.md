Quake BSP map viewer with Oculus Rift support
================

This is a BSP tree OpenGL renderer written in C++. It handles basic geometry and curved patch rendering but with no support for game-specific shaders, entities etc. It implement PVS and frustum culling so performance is optimal. At the moment only Quake III Arena maps are supported but an interface is provided for adding other BSP versions in the future.

A separate <code>no_vr</code> branch contains code without any OculusVR dependencies if you're just interested in focusing solely on OpenGL renderer.

Youtube demo:

[![Screenshot](http://kondrak.info/images/q3vr_youtube.png?raw=true)](https://www.youtube.com/watch?v=pAGLW82ryBc)

Screenshots:

![Screenshot](http://kondrak.info/images/qbsp/qbsp1.png?raw=true)
![Screenshot](http://kondrak.info/images/qbsp/qbsp3.png?raw=true)
![Screenshot](http://kondrak.info/images/qbsp/q3vr1.png?raw=true)
![Screenshot](http://kondrak.info/images/qbsp/q3vr2.png?raw=true)
Usage
-----
Running the viewer:

<code>QuakeBspViewerVR.exe &lt;path-to-bsp-file&gt; </code>

Running the viewer in VR:

<code>QuakeBspViewerVR.exe &lt;path-to-bsp-file&gt; -vr</code>

In non-VR mode, use tilde key (~) to toggle statistics menu on/off. In VR mode, toggle between statistics, VR debug data and IR tracking camera frustum rendering (if camera is available). SPACE key will recenter your tracking position. Press M to toggle between different mirror modes. Note that you must have Quake III Arena textures and models unpacked in the root directory if you want to see proper texturing. To move around use the WASD keys. RF keys lift you up/down and QE keys let you do the barrel roll (in non-VR mode only).

Dependencies
-------
This project uses following external libraries:

- GLEW extension library
- [stb_image](https://github.com/nothings/stb) library for texture loading
- SDL2 library for window/input 
- VR support requires OculusVR SDK 1.3+ and Oculus Home installed

References
-------
This viewer was made using following resources:
- http://www.mralligator.com/q3/
- http://graphics.cs.brown.edu/games/quake/quake3.html

Known issues
-------
Certain BSP leaves containing doors, moving platforms etc. may not render correctly with PVS enabled. This is because they are handled differently by the original game.

Todo
----
Here's a list of ideas that could be fun to implement over time:

- add support for older bsp formats (ie. Quake1, Quake2)
- render map entities (weapons, armor, other pickups)
- support loading data from pk3/pak files
- add support for rendering Quake 3 shaders
- bsp patch tesselation on the fly

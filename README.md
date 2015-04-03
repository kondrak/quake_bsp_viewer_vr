Quake BSP map viewer with Oculus DK2 support
================

This is a rudimentary map viewer for Quake proof-of-concept project. It handles the very basic polygon/curved patch rendering (ie. no support for game-specific shaders, entity rendering etc.). It implement PVS and frustum culling so you shouldn't run into any performance issues. At the moment the Quake III Arena map format but an interface is provided for other BSP version support.

![Screenshot](http://kondrak.info/images/qbsp/qbsp1.png?raw=true)
![Screenshot](http://kondrak.info/images/qbsp/qbsp3.png?raw=true)
![Screenshot](http://kondrak.info/images/qbsp/q3vr1.png?raw=true)
![Screenshot](http://kondrak.info/images/qbsp/q3vr2.png?raw=true)
Usage
-----
To run the application, supply the bsp filename as a cmd line argument:

<code>QuakeBspViewer.exe maps/ntkjidm2.bsp</code>

Launching the viewer in VR mode (full screen Direct Mode):

<code>QuakeBspViewer.exe maps/ntkjidm2.bsp -vr</code>

Launching the viewer in VR mode (windowed Direct Mode):

<code>QuakeBspViewer.exe maps/ntkjidm2.bsp -vr -window</code>

In non-VR mode, use tilde key (~) to toggle statistics menu on/off. In VR mode, toggle between statistics, VR debug data and IR tracking camera frustum rendering (if camera is available).

Note that you must have Quake III Arena textures and models unpacked in the root directory if you want to see proper texturing on screen, otherwise you'll just see a bunch of lightmapped, colored checker-board textures filling the screen which is less than impressive.

To move around use the WASD keys. RF keys lift you up/down and QE keys let you do the barrel roll (in non-VR mode only).


Dependencies
-------
This project uses following external libraries (included but I feel I have to mention it here):

- GLEW extension library
- stb_image library for image handling (c) Sean Barret
- SDL2 library for window/input 
- OculusVR SDK 0.5.0.1 and corrsesponding runtime for VR support

References
-------
This viewer was made using resources on following websites:
- http://www.mralligator.com/q3/
- http://graphics.cs.brown.edu/games/quake/quake3.html

And yes, I know that Q3 sources are open now but I felt like taking up a challenge ;)


Known issues
-------
You may run into is wrong handling of bsp leaves that contain doors, moving platforms etc. This is because they are handled differently by the original game. For this reason, you'll notice that doors may not render or appear suddenly when the camera reaches certain arbitrary position on the map (q3ctf1 is a good example where some leaves don't render at the right moment if PVS is enabled). 

Having the Oculus Rift in extended desktop will very likely result in heavy judder even if you have no FPS drops. At this time OpenGL support is still less than optimal in OculusSDK.

Todo
----
Here's a list of ideas that could be fun to implement over time:

- add support for older bsp formats (ie. Quake1, Quake2)
- render map entities (weapons, armor, other pickups)
- support loading data from pk3/pak files
- add support for rendering Quake 3 shaders (if you're REALLY bored)
- bsp patch tesselation on the fly - you give the level, we rework it!

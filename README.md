Quake BSP map viewer
================

This is a rudimentary map viewer for Quake, written as a fun-time project over a single weekend. For that reason it only handles the very basic polygon/curved patch rendering (ie. no support for game-specific shaders, entity rendering etc.). It does, however, implement PVS and frustum culling so you shouldn't run into any performance issues. At the moment the Quake III Arena map format is the only one you can view but I might extend it over time to support earlier bsp versions as well.

![Screenshot](http://kondrak.info/images/qbsp/qbsp1.png?raw=true)
![Screenshot](http://kondrak.info/images/qbsp/qbsp3.png?raw=true)

Usage
-----
To run/debug the application, supply the bsp filename as a cmd line argument:

<code>QuakeBspViewer.exe maps/ntkjidm2.bsp</code>

If the file is a valid bsp/not corrupted it will load up the map in a 1024x768 window. For a quick demo launch the <code>run.bat</code> script.

Note that you must have Quake III Arena textures and models unpacked in the root directory if you want to see proper texturing on screen, otherwise you'll just see a bunch of lightmapped, colored checker-board textures filling the screen which is less impressive.

To move around use the WASD keys. RF keys lift you up/down and QE keys let you do the barrel roll.


Dependencies
-------
This project uses following external libraries (included but I feel I have to mention it here):

- GLee library for extensions (c)2011 Ben Woodhouse
- GLFont library for rendering text (c)1998 Brad Fish, (c)2002 Henri Kyrki
- stb_image library for image handling (c) Sean Barret
- SDL2 library for window/input 


References
-------
This viewer was made using resources on following websites:
- http://www.mralligator.com/q3/
- http://graphics.cs.brown.edu/games/quake/quake3.html

And yes, I know that Q3 sources are open now but I felt like taking up a challenge ;)


Known issues
-------
The only noticeable you may run into is wrong handling of bsp leaves that contain doors, moving platforms etc. This is because they are handled differently by the original game. For this reason, you'll notice that doors may not render or appear suddenly when the camera reaches certain arbitrary position on the map (q3ctf1 is a good example where some leaves don't render at the right moment if PVS is enabled). 


Todo
----
Here's a list of ideas that could be fun to implement over time:

- rewrite the thing so that it doesn't use OpenGL immediate mode for rendering
- add support for older bsp formats (ie. Quake1, Quake2)
- render map entities (weapons, armor, other pickups)
- support loading data from pk3/pak files
- add support for rendering Quake 3 shaders (if you're REALLY bored)
- bsp patch tesselation on the fly - you give the level, we rework it!

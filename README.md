<h1>PolyVR - A Shortcut To Virtual Reality</h1>

PolyVR is a scene authoring system for engineering applications.
It packs common virtual reality features like clustering, tracking and scenegraph operations as well as advanced features like interactive offscreen websites on any geometry using the chromium embedded framework, the bullet physics engine, world generation algorithms with a OpenStreetMap data interface (in development), mechanics simulation, a molecule generator (from string definitions) and much more. PolyVR comes with a graphical user interface to ease the authoring of complex virtual scenes and worlds. Logic can be scripted using Python and shader written in GLSL. The authoring process is dynamic, no need to stop the scene, every change applies instantly. This is also true for the hardware configuration dialogs which allows for easy deployment and calibration of distributed visualization setups. No need for complex config files and system restart for every change.

<h2>Requirements</h2>

Ubuntu 18.04 LTS and derived distributions (dropping soon)

Ubuntu 20.04 LTS and derived distributions

Ubuntu 22.04 LTS and derived distributions

There are no special hardware requirements for basic usage.
A decent graphics card from any main vendor is recommended.
Some specialized modules require OpenGL 4.X to work properly.
PolyVR is regularly tested on NVIDIA QUADDRO, NVIDIA GTX and ATI RADEON cards.

<h2>Install instructions</h2>

clone this repository

 git clone https://github.com/Victor-Haefner/polyvr.git

 cd polyvr

 sudo ./install


build the project in codeblocks with the project file polyvr/PolyVR_XX.04.cbp (XX is 18/20/22 corresponding to the ubuntu LTS version).
Set the number of compilation processes to 5 or more depending on your CPU.
The option is in Settings > Compiler... > Global compiler settings > Build options.

PolyVR does not yet run with Wayland wich comes with 22.04.
To switch to X edit (with sudo) /etc/gdm3/custom.conf and uncomment the line with WaylandEnable=false


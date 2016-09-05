# Unreal Terrain Snow Simulation
Snow simulation for large terrains which runs in real-time on the GPU. The simulation is based on the paper [Geospecific rendering of alpine terrain](https://www.cs.utah.edu/~thompson/publications/Premoze:1999:GRA.pdf) by Simon Premoze et al. with several changes. 

# How to Use

The main Actor used by the simulation is the 

## Digital Elevation Model
The first step is importing a digital elevation model. An easy way to generate an artificial digital is to use a tool such as ![World Machine](http://www.world-machine.com/). See https://wiki.unrealengine.com/World_Machine_to_Unreal_Engine_4_-_In_Depth_Guide for a guide on how to import a digital elevation model into the Unreal Engine.

## Weather Data

# Implementation Details

There are two implementations available of the simulation. The GPU implementation which uses a Compute Shader and an implementation which runs on the CPU. The simulations write the amount of snow into a texture which is then used by the Landscape rendering Blueprint.

# TODO

1. Fix GPU bugs with rendering target
2. Improve statistical weather generator

   The statistical weather generator is only able to generate weather data for a single location. It should be updated to be able to generate weather data for larger scales. An approach for this

# Screenshots

![snow comparison](http://i.imgur.com/fou2hOm.jpg)

![mountain without snow](http://i.imgur.com/C9BNX9N.jpg)

![mountain with snow](http://i.imgur.com/Muv39YE.jpg)

![mountain without snow 2](http://i.imgur.com/cHsONvN.jpg)

![mountain with snow 2](http://i.imgur.com/PVhWQIK.jpg)

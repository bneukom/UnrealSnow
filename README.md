# Unreal Terrain Snow Simulation
Snow simulation for large terrains which runs in real-time on the GPU. The simulation is based on the paper [Geospecific rendering of alpine terrain](https://www.cs.utah.edu/~thompson/publications/Premoze:1999:GRA.pdf) by Simon Premoze et al. with several changes.

There are two implementations of the simulation available. The GPU implementation which uses a Compute Shader and an implementation which runs on the CPU. The simulations write the amount of snow into a texture which is then used by the landscape rendering Blueprint.

# How to Use

The main Actor used by the simulation is the [SnowSimulationActor](https://github.com/bneukom/snowsimulation/blob/master/Source/SnowSimulation/Simulation/SnowSimulationActor.cpp).

## Digital Elevation Model
The first step is importing a digital elevation model. An easy way to generate an artificial digital is to use a tool such as [World Machine](http://www.world-machine.com/). See https://wiki.unrealengine.com/World_Machine_to_Unreal_Engine_4_-_In_Depth_Guide for a guide on how to import a digital elevation model into the Unreal Engine.

## Climate Data

Climate data is globally available and comes in various data formats. Currently supported sources are:

1. MeteoSwiss
2. WorldClim
3. Stochastic Weather Generator
   The stochastic weather generator uses a two state markov chain to simulate temperature and precipitation as described in *Stochastic simulation of daily precipitation, temperature, and solar radiation* by Richardson.

If additional climate data imports are needed the interface [USimulationWeatherDataProviderBase](https://github.com/bneukom/snowsimulation/blob/master/Source/SnowSimulation/Simulation/Data/SimulationWeatherDataProviderBase.h) can be extended.



# TODO

1. Fix GPU bugs with rendering target
2. Improve statistical weather generator

   The statistical weather generator is only able to generate weather data for a single site. It should be updated to be able to generate weather data for multiple sites. This could be achieved by using 
   
3. Fix interpolation for multiple climate data sites

# Screenshots

Some results of the simulations:

![snow comparison](http://i.imgur.com/fou2hOm.jpg)

![mountain without snow](http://i.imgur.com/C9BNX9N.jpg)

![mountain with snow](http://i.imgur.com/Muv39YE.jpg)

![mountain without snow 2](http://i.imgur.com/cHsONvN.jpg)

![mountain with snow 2](http://i.imgur.com/PVhWQIK.jpg)

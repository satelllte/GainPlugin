# GainPlugin

A basic VST/AU audio plugin example based on [JUCE](https://github.com/juce-framework/JUCE) framework.

## Cloning repository

```bash
git clone --recurse-submodules git@github.com:satelllte/GainPlugin.git
```

## Third party submodules

| Name | Version | About |
|---|---|---|
| JUCE | [6.1.6](https://github.com/juce-framework/JUCE/releases/tag/6.1.6) | To know more about what JUCE is check out its [docs](https://github.com/juce-framework/JUCE#readme). The reason it's used here as a submodule is because all of its modules linked via relative paths in [GainPlugin.jucer](./GainPlugin.jucer) configuration file. |

## Getting started

### 1. Generating project via Projucer

[Projucer](https://github.com/juce-framework/JUCE/tree/6.1.6#the-projucer) is a project-configuration tool provided by JUCE. 

To get the executable file you can:

1. Build it from source for your platform. Check out [/JUCE/extras/Projucer/Builds/](/JUCE/extras/Projucer/Builds/) folder.
2. Download pre-built executables. Check out the archive files of [JUCE 6.1.6 release](https://github.com/juce-framework/JUCE/releases/tag/6.1.6).

Now, when Projucer executable is installed and available to run, `GainPlugin.jucer` configuration file can be opened in it and the project for Xcode or Visual Studio (depending on operating system) can be generated. 

For more info on Projucer, check out [Tutorial: Projucer Part 1: Getting started with the Projucer](https://docs.juce.com/master/tutorial_new_projucer_project.html).

### 2. Develop & Build

Check out [Tutorial: Projucer Part 2: Manage your Projucer projects](https://docs.juce.com/master/tutorial_manage_projucer_project.html).
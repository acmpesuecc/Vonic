# Vonic
### By Vortex

![Inshalla this image is rendered properly](assets/image.png)

- Audio Plugin Created using the [JUCE](https://github.com/juce-framework/JUCE) Plugin.
- Performs Attenuation and Amplification of lowband and high band frequencies with optional adjustment of Gain.
- Future Prospects : Audio Visualizer and a more comprehensive UI (As soon as i get some time).

## Build Instructions :

### Getting your template running : 

- Follow [JUCE Build Instructions](https://github.com/juce-framework/JUCE) to build Projucer(GUI tool to configure your JUCE projects).
- Create New Project and choose defaults as-per your Operating System.
- Make Sure to enable the `juce_dsp` module in the modules section.
- Once Project is created, Build to ensure that project environment is set up correctly.
- If a window with `Hello World` is displayed, it means that you've successfully setup your JUCE project.

### Cloning This project : 

- Replace the `/your/project/folder/Source` with the `Source` folder from this project.
- Build the project once again and you should have a working replica.
### Debugging Your Builds : 

- Visit `/your/path/JUCE/extras/AudioPluginHost/Builds`.
- Build as per your operating system.
    - XCode for OSX.
    - Visual Studio for Windows
    - Make files for linux :)
- After this run `AudioPluginHost` and scan for VST3/AU plugins.
- Ideally your build should appear on the list of identified plugins.
- Drag and drop the plugin onto the debug area, make connections and test the build!

#### Narayan :grimacing:

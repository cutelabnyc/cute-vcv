# Cute Modules

Hardware modules made by [CuteLab](https://cutelab.nyc/)

## VCV Rack

VCV Rack organizes program extensions into Plugins, each of which can contain several Modules. We build a single plugin, CuteLab, which contains all of our individual modules. 

### Creating a new VCV Rack Module

Creating a new VCV Rack module starts with designing the panel. Panel designs can be found in the `panels` directory, with a different subfolder for each module.

#### Prerequisites

1. Download and install the VCV Rack SDK.
2. Make sure the path to the SDK is available in RACK_SDK

#### Making a new plugin from scratch

1. Create an Adobe Illustrator file containing the module. Document dimensions should be in millimeters, with 128.5mm height and width in a multiple of 5.08mm (1 HP)
2. Size the artboard to be the same size as your panel
3. You'll need at least two layers, one for your panel design and one for the "components", including jacks, pots, and LEDs. You can find more details in the official VCV Rack documentation https://vcvrack.com/manual/Panel. Make sure that each of your basic shapes in the "components" layer has a name, which you can set in the Illustrator "Layers" view.
4. Remember that VCV Rack cannot render text from an SVG file, so you should convert all text to outlines before exporting.
5. When you like your panel, export the panel to an SVG file. The suffix to the SVG file should by `.in.svg`, for a full name like `mypanel.in.svg`.
6. Run `npm run vcv:format`. This will make small modifications to the SVG file, converting it to a format that will render correctly in VCV Rack. The result will be placed into a new `.svg` file, converting `mypanel.in.svg` to `mypanel.svg`.
7. Copy this file to `vcv-rack/res`. This is now the input template that the VCV Rack scripts will use to create a new panel from your template.
8. From the `vcv-rack` directory, run `$RACK_DIR/helper.py createmodule <new-module-name> res/<new-module-name>.svg src/<new-module-name>.cpp`.
9. Now the `.cpp` file `<new-module-name>.cpp` should define the view of your new module. Looking at the file text, you should see function calls to do things like create and position knobs, and to read the values of parameter and signal inputs.
10. In order to include the module in the plugin, be sure to modify `plugin.hpp` and `plugin.cpp` to include the new module.
11. From the vcv-rack directory, run `make && make install` to build the plugin and install it to VCV Rack plugin directory. On Mac, this path will be `~/Library/Application Support/Rack2/plugins-mac-arm64`. 

### Installing a plugin directly from a release

Place the `.vcvplugin` file into `~/Library/Application Support/Rack2/plugins-mac-arm64`. It must be top level. Upon loading the VCV Rack application, the file will be unpacked, creating a CuteLab folder with the appropriate files. More info [here](https://vcvrack.com/manual/Installing#Installing-plugins-not-available-on-the-VCV-Library).

### Debugging a plugin

When you open VCV Rack, your new module should appear among modules from CuteLab. If you don't see your module, then it's likely that something went wrong when trying to load the plugin. Check the log file at `~/Library/Application Support/Rack2/log.txt` to see what might have gone wrong when trying to load the plugin.

To debug a plugin as it runs, you can use the `(lldb) Launch VCV` launch configuration in the `.vscode` directory. You will of course need to be running VSCode for this to work.

You will also need to make sure that debug symbols are not stripped from the CuteLab plugin when building. The way that the Rack SDK makefile is configured, there doesn't seem to be an easy way to prevent it from invoking `strip` on the output plugin. However, you can "trick" the bundler into leaving sympols intact by overriding the `STRIP` variable with "true".

```
make && make install STRIP=true
```

This will (counterintuitively) compile the plugin without stripping debug symbols. Now, when you attach to VCV Rack, you should be able to set breakpoints in your code and debug the output.

### Updating the VCV Rack Plugin Version

The folder `vcv-rack` contains a file `plugin.json`, which defines metadata about the CuteLab plugin as well as the plugin version. Importantly, this plugin version must always have a major version of 2, in order to be compatible with VCV Rack version 2. 

In order to publish a new version of the plugin, you should call `npm run vcv:version` to update the plugin version. Generally, you should not modify the version in `vcv-rack/src/plugin.json` directly. Calling this script will do three things:

1. Make sure the version is valid semve
2. Update the version in vcv-rack/src/plugin.json
3. Create a new git commit and tag for the version change

If you push the commit and tag, it should automatically trigger a GitHub action that will build and upload the CuteLab VCV Rack plugin to releases.

```
npm run vcv:version -- --version=2.0.3
```

This will set the version to 2.0.3

```
npm run vcv:version -- --version=2.0.4-momjeans_alpha.0
```

Set the version to a prerelease revision "momjeans_alpha" at revision 0.

```
npm run vcv:version -- prerelease
```

Bump the prerelease version number, e.g. from version=2.0.4-momjeans_alpha.0 to version=2.0.4-momjeans_alpha.1

```
npm run vcv:version -- patch
```

Bump the patch version number, e.g. from 2.0.4 to 2.0.5

```
npm run vcv:version -- patch --dry-run
```

Bump the patch version number, e.g. from 2.0.4 to 2.0.5, but don't actually change any files or make any git tags, just show what would happen.

### Publishing a new VCV Rack Plugin

As mentioned, anytime you push a new version of the plugin, GitHub should automatically build a new release. You can also trigger a new build manually from the actions page https://github.com/cutelabnyc/cute-modules/actions/workflows/publish-vcvrack.yml.

The build script will generate a changelog automatically from git commits, but you can add additional notes by leaving a markdown file in `vcv-rack/release-notes`. Notes should be simple markdown, with something like the following form:

```
# Notes for v2.0.3

Initial internal release for development. Includes an implementation of Mom Jeans, both as a gen export and in plain C.

## Included Modules

- Mom Jeans (Pulsar Synthesis)
```

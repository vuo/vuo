# Vuo SDK
This is the Vuo SDK, which you can use to develop add-ons to Vuo, and to embed Vuo in your applications.  You can find the latest version at https://vuo.org/download .

The Vuo SDK includes:

## The Vuo Framework
In the `framework` folder:

   - `Vuo.framework` for macOS 32-bit and 64-bit apps — including the compiler, runner, and nodes

   - The Vuo command-line tools:
      - `vuo-compile`
      - `vuo-debug`
      - `vuo-export`
      - `vuo-link`
      - `vuo-render`
      - Run each command from Terminal (e.g., `./framework/vuo-compile`) for information on what it does and how to use it
      - See http://doc.vuo.org/latest/manual/the-command-line-tools.xhtml for more information
      - (If you want to move these command-line tools to another folder, note that they all require `Vuo.framework` to be in the same folder as them, and `vuo-export` and `vuo-render` require the Qt frameworks in the `resources` subfolder.  Alternatively, you can run the command-line tools from anywhere by adding the `framework` folder to your shell's `PATH`.)

   - In the `resources` folder:
      - The Qt frameworks (required for `vuo-export` and `vuo-render`)
      - The fonts used on the Vuo Editor canvas (required by `vuo-render`)
      - The `inputEditorWidgets` folder, containing the C++ headers and library required to create input editors for custom types — see http://api.vuo.org/latest/group__DevelopingInputEditors.html for more information

## Examples
In the `example` folder:

   - In the `node` folder, example projects for creating add-ons to Vuo:
      - `stateless` — a very simple node implementation — see http://api.vuo.org/latest/group__DevelopingNodeClasses.html for more information
      - `stateful` — an example node that keeps track of data — see http://api.vuo.org/latest/group__DevelopingNodeClasses.html for more information
      - `imageFilter` — an example node that processes an image on the GPU — see http://api.vuo.org/latest/group__WorkingWithGraphics.html for more information
      - `customType` — an example type and input editor implementation — see http://api.vuo.org/latest/group__DevelopingTypes.html for more information

   - In the `runner` folder, example projects for using Vuo inside another application:
      - For Vuo's Cocoa API: `VuoPluginApp`
      - For Vuo's C++ API: `CompileAndRunInCurrentProcess`, `CompileAndRunInNewProcess`, `ListPublishedPorts`, `RunImageFilter-GLFW`, `RunImageFilter-Qt`
      - See http://api.vuo.org/latest/group__DevelopingApplications.html for more information

To build, install, and use each example:

   - For the `VuoPluginApp` example, open `VuoPluginApp.xcodeproj` in Xcode, and click the Run button
   - For the other examples:
      - In Terminal, change to the example's folder — e.g., `cd example/node/stateless`
      - Run `qmake` to generate the makefile
      - Run `make` to build and install the example
      - For examples in the `node` folder, (re-)launch Vuo Editor so it loads the example node or type/input editor
      - For examples in the `runner` folder:
         - Launch `CompileAndRunInCurrentProcess`, `CompileAndRunInNewProcess`, and `ListPublishedPorts` from Terminal by entering the binary's filename preceded by `./` (e.g., `./ListPublishedPorts`), and press Return
         - Launch `RunImageFilter-GLFW` and `RunImageFilter-Qt` by double-clicking the app icon in Finder

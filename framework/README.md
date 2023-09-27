# Vuo SDK
This is the Vuo SDK, which you can use to develop add-ons to Vuo, and to embed Vuo in your applications.  You can find the latest version at <https://vuo.org/download>.

The Vuo SDK includes:

## The Vuo Framework
In the `/Library/Developer/Vuo/framework` folder:

   - `Vuo.framework` enables macOS 64-bit apps to compile, launch, and control compositions
   - `VuoRunner.framework` enables macOS 64-bit apps to launch and control precompiled compositions
      - See https://api.vuo.org/latest/group__DevelopingApplications.html for more information about choosing between the two framework options

   - The Vuo command-line tools:
      - `vuo-compile`
      - `vuo-debug`
      - `vuo-export`
      - `vuo-link`
      - Run each command from Terminal (e.g., `./vuo-compile`) for information on what it does and how to use it
      - See https://doc.vuo.org/latest/manual/the-command-line-tools.xhtml for more information
      - (If you want to move these command-line tools to another folder, note that they all require `Vuo.framework` to be in the same folder as them, and `vuo-export` requires the Qt frameworks in the `resources` subfolder.  Alternatively, you can run the command-line tools from anywhere by adding the `framework` folder to your shell's `PATH`.)

   - In the `resources` folder:
      - The Qt frameworks (required for `vuo-export`)
      - The fonts used on the Vuo Editor canvas (required by `vuo-export`)
      - The `inputEditorWidgets` folder, containing the headers and library required to create input editors for custom types — see https://api.vuo.org/latest/group__DevelopingInputEditors.html for more information

## Examples
In the `/Library/Developer/Vuo/example` folder:

   - In the `node` folder, example projects for creating add-ons to Vuo:
      - `stateless` — a very simple node implementation — see https://api.vuo.org/latest/group__DevelopingNodeClasses.html for more information
      - `stateful` — an example node that keeps track of data — see https://api.vuo.org/latest/group__DevelopingNodeClasses.html for more information
      - `imageFilterGLSL` — an example node that processes an image on the GPU using a GLSL fragment shader — see https://api.vuo.org/latest/group__WorkingWithGraphics.html for more information
      - `imageFilterMetal` — an example node that processes an image on the GPU using Apple Metal
      - `imageFilterCoreImage` — an example node that processes an image on the GPU using Apple Core Image
      - `customType` — an example type and input editor implementation — see https://api.vuo.org/latest/group__DevelopingTypes.html for more information

   - In the `runner` folder, example C++ projects for using Vuo inside another application:
      - `CompileAndRunInCurrentProcess`
      - `CompileAndRunInNewProcess`
      - `ListPublishedPorts`
      - `RunImageFilter-GLFW`
      - `RunImageFilter-Qt`
      - See https://api.vuo.org/latest/group__DevelopingApplications.html for more information

To build, install, and use each example:

   - Copy the example's folder to your Desktop or some other folder you have write access to
   - In Terminal, change to the example's folder — e.g., `cd stateless`
   - Create a build folder — `mkdir build`
   - Change to the build folder — `cd build`
   - Run `cmake ..` to generate the makefile
   - Run `make` to build and install the example
   - Launch examples in the `runner` folder from Terminal by entering the binary's filename preceded by `./` (e.g., `./ListPublishedPorts`) and pressing Return

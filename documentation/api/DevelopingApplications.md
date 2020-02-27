@addtogroup DevelopingApplications

If you're developing an application, library, or other project, your code can build and run Vuo compositions. One way is by invoking the Vuo command-line tools (see the [Vuo Manual](https://vuo.org/manual.pdf)), but you can also interact with Vuo programmatically through its API.

To learn how to develop an application that uses Vuo, see: 

   - This API documentation
   - The example projects found in the `/Library/Developer/Vuo/example/runner` folder after installing the Vuo SDK:
      - For Vuo's Cocoa API: `VuoPluginApp`
      - For Vuo's C++ API: `CompileAndRunInCurrentProcess`, `CompileAndRunInNewProcess`, `ListPublishedPorts`, `RunImageFilter-GLFW`, `RunImageFilter-Qt`
   - The source code for the Vuo command-line tools


## Choosing an API

### Cocoa API

If you plan to use Vuo within a Cocoa application, and the compositions you'll be using it with conform to the Image Generator or Image Filter protocol, then consider using the Cocoa API. It provides a simple Objective-C interface for retrieving images from compositions (VuoRunnerCocoa.hh) and methods for converting between Vuo types and Cocoa types (VuoRunnerCocoa+Conversion.hh).

### C++ API

Otherwise, you should use the C++ API (VuoRunner.hh). It offers much more flexibility for running and interacting with compositions.

### 64-bit

The Cocoa API and the C++ API both support 64-bit mode.  (Up through Vuo 1.2.x, the Vuo SDK included 32-bit support, but we removed it in Vuo 2.0.)


## Setting up your application

To use Vuo, your application needs to link to Vuo.framework, which comes with the Vuo SDK. 


### Xcode 11

If using the Cocoa API or the C++ API:

  - Create a new project:
    - Choose template macOS > Application > App.
    - Choose language Objective-C.
  - Add Vuo.framework to the project:
    - In Finder, move or copy Vuo.framework into the project's folder.
    - In Xcode, File > Add Files to your project. Select Vuo.framework from the project's folder.
      - Vuo.framework should now appear under Build Phases > Link Binary With Libraries.
  - Modify the target's Build Phases:
    - Editor > Add Build Phase > Add Copy Files Build Phase
      - Destination: Frameworks
      - Add Vuo.framework here. (You can drag and drop it from the Project Navigator.)
  - Modify the target's Build Settings: 
    - Go to Editor > Add Build Setting > Add User-Defined Setting, add `REMOVE_HEADERS_FROM_EMBEDDED_BUNDLES`, and set it to `NO`.
  - Set up one or more of your project's source files to be able to call Vuo API functions:
    - If using the C++ API, name the source file with extension ".mm" (for Objective-C++) instead of ".m".
    - Add @code{cpp} #include <Vuo/Vuo.h> @endcode


### Qt Creator

See the example CMake projects included with the Vuo SDK.


## Using the Cocoa API

Depending on whether the Vuo compositions to be run by your application are Image Filters or Image Generators, you should use either the VuoImageFilter or VuoImageGenerator class. With these classes, you can set the composition's published input port values, filter or generate images based on those inputs, and retrieve the output images as NSImages or GL textures. These classes provide a high-level interface that does all of the necessary composition setup (compiling, linking, and running in a separate 64-bit process) behind the scenes.


## Using the C++ API

### Compiling and linking a Vuo composition

If you want to run a Vuo composition, you first have to compile and link it. 

The easiest way to do that is with the factory methods VuoCompiler::newSeparateProcessRunnerFromCompositionFile and VuoCompiler::newCurrentProcessRunnerFromCompositionFile. These compile and link your composition and return a VuoRunner that's ready to run it. 

The VuoCompiler class also provides functions for separately compiling a composition, linking a composition, and compiling a node class, port type, or library module. 

### Running a Vuo composition

Using the VuoRunner class, you can run and interact with a composition. The composition can run in the same process as the VuoRunner or in a separate process. 

The VuoRunner::start and VuoRunner::stop functions allow you to start and stop a composition. While the composition is running, you can use other VuoRunner functions to control it and query it (such such pausing the composition, setting the values of published input ports, and getting the values of published output ports). 

You can receive notifications from the running composition (such as when a published output port receives an event) by creating a derived class of VuoRunnerDelegate or VuoRunnerDelegateAdapter. Use VuoRunner::setDelegate to connect your class to the VuoRunner. Your class's VuoRunnerDelegate functions will be called whenever the VuoRunner receives messages from the running composition. 


## A mini framework for running Vuo compositions

If your application only needs to run Vuo compositions that are already compiled and linked, there's an alternative to Vuo.framework that has a much smaller file size: VuoRunner.framework.

VuoRunner.framework contains the C++ API functions for running a composition. It does not contain the Cocoa API functions or any C++ API functions for compiling or linking a composition.  This could be useful if your application has built-in, non-user-modifiable compositions.  (But if your application needs to run _any_ composition, such as custom compositions created by end-users, you'll need the compiler and thus you should use the full-size Vuo.framework.)

VuoRunner.framework doesn't come with any headers or supporting libraries of its own. Your application will need to refer to Vuo.framework instead of VuoRunner.framework for its header files. You'll need to copy the frameworks and dylibs that your application makes use of from the Frameworks and Modules folders in Vuo.framework into the corresponding folders in VuoRunner.framework.  An easy way to find out which frameworks and dylibs a particular composition uses is to open it in Vuo editor, export it as an app, then look in the exported app bundle's `Contents/Frameworks/VuoRunner.framework/Frameworks` and `Contents/Frameworks/VuoRunner.framework/Modules` folders.

&nbsp;                                             | Vuo.framework | VuoRunner.framework
-------------------------------------------------- | ------------- | --------------------------------------------------------------------------------
Framework size, uncompressed                       | about 170 MB  | about 5 MB + any frameworks and dylibs required by your precompiled compositions
Run precompiled compositions (@ref VuoRunner)      | ✅            | ✅
Compile and run any composition (@ref VuoCompiler) | ✅            | ❌
Cocoa API (@ref VuoRunnerCocoa)                    | ✅            | ❌


## The Vuo framework contains encryption

The Vuo framework contains [OpenSSL](https://www.openssl.org/) libraries, which are needed by nodes that support downloading files via HTTPS. If you plan to distribute an application that uses the Vuo framework, your application may be subject to export regulations. For more information, see [How do export regulations impact the applications I create with Vuo?](https://vuo.org/node/511).

@addtogroup DevelopingApplications

If you're developing an application, library, or other project, your code can build and run Vuo compositions. One way is by invoking the Vuo command-line tools (see the [Vuo Manual](http://vuo.org/manual.pdf)), but you can also interact with Vuo programmatically through its API. 

To learn how to develop an application that uses Vuo, see: 

   - This API documentation
   - The example projects found in the `example/runner` folder of the Vuo SDK:
      - For Vuo's Cocoa API: `VuoPluginApp`
      - For Vuo's C++ API: `CompileAndRunInCurrentProcess`, `CompileAndRunInNewProcess`, `ListPublishedPorts`, `RunImageFilter-GLFW`, `RunImageFilter-Qt`
   - The source code for the Vuo command-line tools


## Choosing an API

### Cocoa API

If you plan to use Vuo within a Cocoa application, and the compositions you'll be using it with conform to the Image Generator or Image Filter protocol, then consider using the Cocoa API. It provides a simple Objective-C interface for retrieving images from compositions (VuoRunnerCocoa.hh) and methods for converting between Vuo types and Cocoa types (VuoRunnerCocoa+Conversion.hh).

### C++ API

Otherwise, you should use the C++ API (VuoRunner.hh). It offers much more flexibility for running and interacting with compositions.

### 32-bit and 64-bit

The Cocoa API and the C++ API can be used in both 32-bit and 64-bit applications. The Cocoa API is available in its entirety on both architectures. In the C++ API, some classes and functions are available only in 64-bit mode — specifically, classes whose names begin with `VuoCompiler`, and VuoRunner functions that involve running a composition in the current process.

When running a composition from a 32-bit application, the composition always runs in a separate (64-bit) process. When running a composition from a 64-bit application with the C++ API, you have the option to run it in the current process or a separate process.


## Setting up your application

To use Vuo, your application needs to link to Vuo.framework, which comes with the Vuo SDK. 


### Xcode 4, 5, or 6

If using the Cocoa API or the C++ API:

  - Create a new project:
    - Choose template OS X > Application > Cocoa Application. 
  - Modify the target's Build Phases: 
    - Link Binary With Libraries
      - Use the + button and "Add Other..." to add Vuo.framework. 
    - Editor > Add Build Phase > Add Copy Files
      - Destination: Frameworks
      - Uncheck "Copy only when installing"
      - Drag Vuo.framework from the Project Navigator to this Build Phase. (The + button doesn't work.) 
  - Modify the target's Build Settings: 
    - All > Linking > Runpath Search Paths: `@loader_path/../Frameworks`
    - If using Xcode 6.1.1 or later, go to Editor > Add Build Setting > Add User-Defined Setting, add `REMOVE_HEADERS_FROM_EMBEDDED_BUNDLES`, and set it to `NO`.
  - Modify &lt;your project&gt;-Prefix.pch: 
    - Just before @code{cpp} #import <Cocoa/Cocoa.h> @endcode add @code{cpp}#define __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES 0 @endcode (This is a workaround for a bug in one of Vuo's dependencies that prevents Cocoa projects from compiling.)

If using the Cocoa API:

  - Set up one or more of your project's source files to be able to call Vuo API functions: 
    - Add @code{cpp} #include <Vuo/Vuo.h> @endcode

If using the C++ API:

  - Modify the target's Build Settings: 
    - All > Apple LLVM [version] - Language - C++ > C++ Language Dialect: C++98
    - All > Apple LLVM [version] - Language - C++ > C++ Standard Library: libstdc++
  - Set up one or more of your project's source files to be able to call Vuo API functions: 
    - Name the source file with extension ".mm" (for Objective-C++) instead of ".m". 
    - Add @code{cpp} #include <Vuo/Vuo.h> @endcode


### Qt Creator

See the example Qt projects included with the Vuo SDK. 


### Extra libraries

Vuo.framework includes the basic functions you need to build and run a Vuo composition. For additional functionality, you can optionally link in these dynamic libraries: 

   - For @ref ManagingMemory "memory management" when working with port types, use the functions declared in VuoHeap.h and defined in `Vuo.framework/Modules/libVuoHeap.dylib`. 
   - For working with OpenGL contexts, use the functions declared in VuoGlContext.h and defined in `Vuo.framework/Modules/libVuoGlContext.dylib`. 

See the RunImageFilter example projects (included with the Vuo SDK) for examples of using these libraries. 


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


## The Vuo framework contains encryption

The Vuo framework contains [OpenSSL](https://www.openssl.org/) libraries, which are needed by nodes that support downloading files via HTTPS. If you plan to distribute an application that uses the Vuo framework, your application may be subject to export regulations. For more information, see [How do export regulations impact the applications I create with Vuo?](https://vuo.org/node/511).

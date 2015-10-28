@addtogroup DevelopingApplications

If you're developing an application, library, or other project, your code can build and run Vuo compositions. One way is by invoking the Vuo command-line tools (see the [Vuo Manual](http://vuo.org/manual.pdf)), but you can also interact with Vuo programmatically through its API. 

To learn how to develop an application that uses Vuo, see: 

   - This API documentation
   - The example Xcode project (`example/api/VuoPluginApp` included with the Vuo SDK) for using Vuo's Cocoa API
   - The example Qt projects (`example/api/RunImageFilter` included with the Vuo SDK) for using Vuo's C++ API
   - The source code for the Vuo command-line tools



## Setting up your application

To use Vuo, your application needs to link to Vuo.framework, which comes with the Vuo SDK. 


### Xcode 4 and 5

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
  - Modify &lt;your project&gt;-Prefix.pch: 
    - Just before @code{cpp} #import <Cocoa/Cocoa.h> @endcode add @code{cpp}#define __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES 0 @endcode (This is a workaround for a bug in one of Vuo's dependencies that prevents Cocoa projects from compiling.)

If using the Cocoa API:

  - Set up one or more of your project's source files to be able to call Vuo API functions: 
    - Add @code{cpp} #include <Vuo/Vuo.h> @endcode

If using the C++ API:

  - Modify the target's Build Settings: 
    - All > Apple LLVM 4.2 (5.0 for Xcode 5) - Language - C++ > C++ Language Dialect: C++98
    - All > Apple LLVM 4.2 (5.0 for Xcode 5) - Language - C++ > C++ Standard Library: libstdc++
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


## The Cocoa API

Using the VuoImageFilter and VuoImageGenerator classes, you can compile, run, interact with, and get information about a composition.  The composition runs in a separate 64-bit process.


## The C++ API

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

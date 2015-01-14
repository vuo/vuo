@addtogroup DevelopingApplications

If you're developing an application, library, or other project, your code can build and run Vuo compositions. One way is by invoking the Vuo command-line tools (see the [Vuo Manual](http://vuo.org/support)), but you can also interact with Vuo programmatically through its C++ API. 

To learn how to develop an application that uses Vuo, see: 

   - This API documentation
   - The example code under "Modules" below
   - The example Qt projects for using Vuo inside an application
   - The source code for the Vuo command-line tools



## Setting up your application

To use Vuo, your application needs to link to Vuo.framework, which comes with the Vuo SDK. 


### Qt Creator

See the example Qt projects for using Vuo inside an application. 


### Xcode 4

  - Create a new project:
    - Choose template OS X > Application > Cocoa Application. 
  - Modify the target's Build Phases: 
    - Link Binary With Libraries
      - Use the + button and "Add Other..." to add Vuo.framework. 
    - Add Build Phase > Add Copy Files
      - Destination: Frameworks
      - Uncheck "Copy only when installing"
      - Drag Vuo.framework from the Project Navigator to this Build Phase. (The + button doesn't work.) 
  - Modify the target's Build Settings: 
    - C++ Language Dialect: C++98
    - C++ Standard Library: libstdc++
    - Runpath Search Paths: `@loader_path/../Frameworks`
  - Modify &lt;your project&gt;-Prefix.pch: 
    - Just before @code #import <Cocoa/Cocoa.h> @endcode add @code #define __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES 0 @endcode (This is a workaround for a bug in one of Vuo's dependencies that prevents Cocoa projects from compiling.)
  - Set up one or more of your project's source files to be able to call Vuo API functions: 
    - Name the source file with extension ".mm" (for Objective-C++) instead of ".m". 
    - Add @code #include <Vuo/Vuo.h> @endcode


### Xcode 5

@todo (https://b33p.net/kosada/node/6211)


## Compiling and linking a Vuo composition

If you want to run a Vuo composition, you first have to compile and link it. 

The easiest way to do that is with the factory methods VuoCompiler::newSeparateProcessRunnerFromCompositionFile and VuoCompiler::newCurrentProcessRunnerFromCompositionFile. These compile and link your composition and return a VuoRunner that's ready to run it. 

The VuoCompiler class also provides functions for separately compiling a composition, linking a composition, and compiling a node class, port type, or library module. 



## Running a Vuo composition

Using the VuoRunner class, you can run and interact with a composition. The composition can run in the same process as the VuoRunner or in a separate process. 

The VuoRunner::start and VuoRunner::stop functions allow you to start and stop a composition. While the composition is running, you can use other VuoRunner functions to control it and query it (such such pausing the composition, setting the values of published input ports, and getting the values of published output ports). 

You can receive notifications from the running composition (such as when a published output port receives an event) by creating a derived class of VuoRunnerDelegate or VuoRunnerDelegateAdapter. Use VuoRunner::setDelegate to connect your class to the VuoRunner. Your class's VuoRunnerDelegate functions will be called whenever the VuoRunner receives messages from the running composition. 

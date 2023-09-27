@addtogroup DevelopingApplications

If you're developing an application, library, or other project, your code can build and run Vuo compositions. One way is by invoking the Vuo command-line tools (see the [Vuo Manual](https://doc.vuo.org/@vuoVersion/manual/the-command-line-tools.xhtml)), but you can also interact with Vuo programmatically through its API.

To learn how to develop an application that uses Vuo, see:

   - This API documentation
   - The example C++ projects found in the `/Library/Developer/Vuo/example/runner` folder after installing the Vuo SDK:
      - `CompileAndRunInCurrentProcess`
      - `CompileAndRunInNewProcess`
      - `ListPublishedPorts`
      - `RunImageFilter-GLFW`
      - `RunImageFilter-Qt`
   - The source code for the Vuo command-line tools


## Setting up your application

To use Vuo, your application needs to link to Vuo.framework, which comes with the Vuo SDK.


### Xcode 13

   - Create a new project:
      - Choose template macOS > Application > App.
      - Choose language Objective-C.
   - Add Vuo.framework to the project:
      - File > Add Files to your project.
      - Select `Vuo.framework` from the `/Library/Developer/Vuo/framework` folder.
      - Uncheck the "Copy items if needed" checkbox.
      - Vuo.framework should now appear under Build Phases > Link Binary With Libraries.
   - Modify the target's Build Phases:
      - Editor > Add Build Phase > Add Copy Files Build Phase
         - Destination: Frameworks
         - Add Vuo.framework here. (You can drag and drop it from the Project Navigator.)
         - Uncheck the "Code Sign On Copy" checkbox. (Vuo.framework is already code-signed and notarized; you should not re-sign it.)
   - Modify the target's Build Settings:
      - Editor > Add Build Setting > Add User-Defined Setting
         - Add `REMOVE_HEADERS_FROM_EMBEDDED_BUNDLES`, and set it to `NO`.
      - Set "Quoted Include In Framework Header" to `NO`.
      - Under "Framework Search Paths", add the `/Library/Developer/Vuo/framework` folder.
   - In the Signing & Capabilities tab:
      - In the top-right of the App Sandbox pane, click the ✕ to disable the Sandbox. (Vuo includes a compiler, which is not permitted by Apple's App Sandbox.)
      - In the Hardened Runtime pane, check the following checkboxes:
         - Disable Library Validation — This enables Vuo to run compositions in the current process
         - Audio Input — If you would like to allow Vuo compositions to use `Receive Live Audio`
         - Camera — If you would like to allow Vuo compositions to use `Receive Live Video`
   - Set up one or more of your project's source files to be able to call Vuo API functions:
      - Name the source file with extension ".mm" (for Objective-C++) in order to use Vuo's C++ API.
      - Add @code{cpp} #import <Vuo/Vuo.h> @endcode


### Qt Creator

See the example CMake projects included with the Vuo SDK.


## Using the API

### Compiling and linking a Vuo composition

If you want to run a Vuo composition, you first have to compile and link it.

The easiest way to do that is with the factory methods VuoCompiler::newSeparateProcessRunnerFromCompositionFile and VuoCompiler::newCurrentProcessRunnerFromCompositionFile. These compile and link your composition and return a VuoRunner that's ready to run it.

If you plan to distribute the executable built from a composition, then instead of the factory methods you should use VuoCompiler::linkCompositionToCreateExecutable and pass VuoCompiler::Optimization_NoModuleCaches (so that the executable doesn't depend on module cache dylibs on your system) and a run-path search path such as `@loader_path/../Frameworks` (so that the executable can find the Vuo framework).

### Running a Vuo composition

Using the VuoRunner class, you can run and interact with a composition. The composition can run in the same process as the VuoRunner or in a separate process.

The VuoRunner::start and VuoRunner::stop functions allow you to start and stop a composition. While the composition is running, you can use other VuoRunner functions to control it and query it (such such pausing the composition, setting the values of published input ports, and getting the values of published output ports).

You can receive notifications from the running composition (such as when a published output port receives an event) by creating a derived class of VuoRunnerDelegate or VuoRunnerDelegateAdapter. Use VuoRunner::setDelegate to connect your class to the VuoRunner. Your class's VuoRunnerDelegate functions will be called whenever the VuoRunner receives messages from the running composition.


## A mini framework for running Vuo compositions

If your application only needs to run Vuo compositions that are already compiled and linked, there's an alternative to Vuo.framework that has a much smaller file size: VuoRunner.framework.

VuoRunner.framework contains the API functions for running a composition. It does not contain any functions for compiling or linking a composition.  This could be useful if your application has built-in, non-user-modifiable compositions.  (But if your application needs to run _any_ composition, such as custom compositions created by end-users, you'll need the compiler and thus you should use the full-size Vuo.framework.)

VuoRunner.framework doesn't come with any headers or supporting libraries of its own. Your application will need to refer to Vuo.framework instead of VuoRunner.framework for its header files. You'll need to copy the frameworks and dylibs that your application makes use of from the Frameworks and Modules folders in Vuo.framework into the corresponding folders in VuoRunner.framework.  An easy way to find out which frameworks and dylibs a particular composition uses is to open it in Vuo editor, export it as an app, then look in the exported app bundle's `Contents/Frameworks/VuoRunner.framework/Frameworks` and `Contents/Frameworks/VuoRunner.framework/Modules` folders.

&nbsp;                                             | Vuo.framework | VuoRunner.framework
-------------------------------------------------- | ------------- | --------------------------------------------------------------------------------
Framework size, uncompressed                       | about 500 MB  | about 15 MB + any frameworks and dylibs required by your precompiled compositions
Run precompiled compositions (@ref VuoRunner)      | ✅            | ✅
Compile and run any composition (@ref VuoCompiler) | ✅            | ❌


## The Vuo framework contains encryption

The Vuo framework contains [OpenSSL](https://www.openssl.org/) libraries, which are needed by nodes that support downloading files via HTTPS. If you plan to distribute an application that uses the Vuo framework, your application may be subject to export regulations. For more information, see [How do export regulations impact the applications I create with Vuo?](https://vuo.org/node/511).

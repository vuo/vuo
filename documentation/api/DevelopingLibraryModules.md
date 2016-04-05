@addtogroup DevelopingLibraryModules

In @ref DevelopingTypes, we saw that a port type allows multiple node classes to share a data type and a set of related functions. Another way that node classes can share functions is by defining those functions in a @term{library module}. 

A Vuo library module is a special kind of library. Although node classes can link to any library that exports C symbols, a Vuo library module works especially well with Vuo because it provides metadata about itself. In particular, it can list other libraries and frameworks as dependencies (see @ref ManagingDependencies), and it can specify its compatibility with various operating systems (see @ref VuoModuleMetadata). Vuo uses this information when linking a composition. 

A library module can be implemented in C, C++, Objective-C, or another language, as long as it exports C symbols. 

A library module can be used to manage global resources. For example, the @ref VuoGlPool.cc library module manages the pool of available OpenGL objects across node classes and port types. 



## Writing a library module

To implement a library module, you need to: 

   - Create the library module source files. 
   - Add <code>\#include "module.h"</code>. 
   - Call @ref VuoModuleMetadata to define the library module's metadata. 
   - Declare and implement your functions in the library module. 


### The file name

When you implement a library module, the first thing you have to decide is the module name. Every library module has two names: a module name and a title. These can be the same. The module name is how node classes and port types refer to your library module. When you create a library module, the file name is the module name. 


### The metadata

Here's an example of metadata for a library module: 

@code{.c}
VuoModuleMetadata({
					 "title" : "VuoGradientNoiseCommon"
				 });
@endcode

The @ref VuoModuleMetadata macro takes a [JSON-formatted](http://www.json.org/) argument. The argument may include a title and other information about the library module. 

For more information, see the documentation for @ref VuoModuleMetadata. 



## Compiling a library module

Before you can install your library module, you need to compile it to a `.bc` (LLVM bitcode) file. 

If your library module is written in C, you can compile it with the `vuo-compile` command-line tool that comes with the Vuo SDK. To learn how to use `vuo-compile`, see the [Vuo Manual](http://vuo.org/manual.pdf) or run `vuo-compile --help`. 

If your library module is written in another language, you can compile it with [Clang](http://clang.llvm.org/). For example: 
@code
clang -cc1 -triple x86_64-apple-macosx10.7.0 -emit-llvm-bc MyModule.c -o MyModule.bc
@endcode

If your library module is written in C++, the @ref VuoModuleMetadata call and any exported functions need to be enclosed by `extern "C" { ... }` so that symbols are exported with C names (instead of C++ mangled names). 



## Installing a library module

The final step is to place your compiled library module in the correct folder, so that it will be detected by the Vuo framework and the Vuo command-line tools. You can place it in either `~/Library/Application Support/Vuo/Modules/` or `/Library/Application Support/Vuo/Modules/`. For more information about these folders, see the [Vuo Manual](http://vuo.org/manual.pdf). 

After that, you should be able to use your library module as a dependency of a node class or port type. See @ref ManagingDependencies. 



## Naming library modules and functions

**Please do not begin your library module's name with "Vuo".** This is reserved for library modules distributed by Team Vuo / Kosada. Please use your own company or personal name for your port types so that Vuo users can appreciate your work (and not be confused). 

Built-in Vuo library modules follow a set of naming conventions. If you develop library modules to share with other Vuo developers, we encourage you to follow these conventions, too, to make your library modules easier to use. 

   - A library module's name should:
      - Use upper camel case (`VuoSceneRenderer`). 
      - Capitalize only the first letter of abbreviations (`VuoUrl`, not `VuoURL`). 
   - A library module function's name should: 
      - Be prefixed with the module name and an underscore (`VuoImageRenderer_draw()`). 
      - For the constructor function, be called "make" (`VuoImageRenderer_make()`). 


## Managing global resources

@todo (https://b33p.net/kosada/node/5252)

@addtogroup ManagingDependencies

Node classes, port types, and library modules can use functions and data types defined in port types, library modules, and other libraries. This enables you to take existing code (such as an open-source library) and wrap it in a Vuo node class, and to share functions between node classes. (See @ref DevelopingTypes and @ref DevelopingLibraryModules.)

A library that you link to can be implemented in C, C++, Objective-C, or another language, as long as it exports C symbols. 

To allow a module (node class, port type, or library module) to use a library, you need to: 

   - List the library in the module's metadata. 
   - Make sure the library's header files can be found when compiling the module. 
   - Make sure the library can be found when linking a composition that uses the module. 

Read on for details. 


## When dependencies are added for you

When developing a node class, you usually don't need to list port types as dependencies. For each port on the node class, the Vuo compiler automatically adds its type as a dependency. (However, you do need to list a port type as a dependency if it's only used within the node implementation and not on any ports.)


## Adding a library to the module's metadata

To specify that a module depends on a library, list it in the module's @ref VuoModuleMetadata in the "dependencies" array. 

For example, a node class that depends on a static library called `libassimp.a`, a dynamic library called `libicuuc.dylib`, a framework called `AppKit.framework`, and a Vuo library module called `VuoUrl.bc` would have this metadata: 

@code{.cc}
VuoModuleMetadata({
					 "title" : "...",
					 "description" : "...",
					 "keywords" : [ ... ],
					 "version" : ... ,
					 "dependencies" : [ "assimp", "icuuc", "AppKit.framework", "VuoUrl" ],
					 "node" : { ... }
				 });
@endcode

Notice that the static and dynamic library names leave off the "lib" prefix, and the file extension is optional; `libassimp.a` becomes `assimp` or `assimp.a`, and `libicuuc.dylib` becomes `icuuc` or `icuuc.dylib`. The Vuo library module name leaves off the file extension; `VuoUrl.bc` becomes `VuoUrl`. The framework keeps its file extension.



## Using or adding to the Vuo compiler's header search paths

When you compile your module, the library's header files need to be in a location where the compiler can find them. 

If the library's header files are not in one of the folders that `vuo-compile` searches by default, you can specify additional folders with the `--header-search-path` argument. 



## Using or adding to the Vuo linker's library search paths

When you link a composition that uses your module, the library needs to be in a location where the Vuo linker can find it. 

If the library is not in one of the folders that `vuo-link` searches by default, you can place the library in `~/Library/Application Support/Vuo/Modules/` or `/Library/Application Support/Vuo/Modules/`. Alternatively, you can specify additional folders to search with the `--library-search-path` and `--framework-search-path` arguments. 

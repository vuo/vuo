@addtogroup DevelopingLibraryModules

A @term{library module} is a way to provide functionality that multiple node classes and types can use.

This documentation assumes you're familiar with @ref DevelopingTypes. Like a type, a library module can contain functions and data structures. Unlike a type, a library module's data structures are not used for storing port data. A library module doesn't have to implement any Vuo API functions; it just has to specify its metadata.

One reason you might want to develop a library module is to wrap a (non-Vuo) library or framework. For more information, see @ref ManagingDependencies.

## Writing a library module

To implement a library module, you need to: 

   - Create the library module's header file and implementation file.
   - Call @ref VuoModuleMetadata to define the library module's metadata. 
   - Declare and implement your functions in the library module. 

### The file name

A library module's file name consists of the machine name followed by a file extension. The machine name is how node classes and types refer to your module when listing it as a dependency.

Library modules can be implemented in C/C++/Objective-C. The file extension for both the header file and the implementation file should reflect the language.

### The metadata

As with node classes and types, you need to specify the metadata for a library module by calling @ref VuoModuleMetadata. Here's an example:

@code{.c}
VuoModuleMetadata({
    "title" : "VuoGradientNoiseCommon",
    "dependencies" : [
        "VuoReal",
        "VuoPoint2d",
        "VuoPoint3d",
        "VuoPoint4d"
    ]
});
@endcode

## Compiling and installing a library module

To be able to use your library module within Vuo, you'll need to:

   - Use the `vuo-compile` command to compile the library module to a `.bc` file.
   - Package the library module into a node set that contains the `.bc` file and any dynamic libraries on which the library module depends.
   - Place the node set (`.vuonode`) file in one of Vuo's [Modules folders](https://doc.vuo.org/@vuoVersion/manual/installing-a-node.xhtml).

After that, you should be able to use your library module as a dependency of a node class or data type. See @ref ManagingDependencies.

## Naming library modules and functions

**Please do not begin your library module's name with "Vuo".** This is reserved for library modules distributed by Team Vuo / Kosada. Please use your own company or personal name for your library modules so that Vuo users can appreciate your work (and not be confused).

Built-in Vuo library modules follow a set of naming conventions. If you develop library modules to share with other Vuo developers, we encourage you to follow these conventions, too, to make your library modules easier to use. 

   - A library module's name should:
      - Use upper camel case (`VuoSceneRenderer`). 
      - Capitalize only the first letter of abbreviations (`VuoUrl`, not `VuoURL`). 
   - A library module function's name should: 
      - Be prefixed with the module name and an underscore (`VuoImageRenderer_draw()`). 
      - For the constructor function, be called "make" (`VuoImageRenderer_make()`). 

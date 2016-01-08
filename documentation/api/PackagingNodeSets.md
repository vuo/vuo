@addtogroup PackagingNodeSets

When you distribute node classes to other Vuo users, you'll probably want to package them in a format that's easy to download and install. Along with node classes, you may also want to bundle port types, library modules, and example compositions. You can accomplish this by packaging the files into a @term{node set}. 

A node set is a zip file that contains a set of related node classes, their supporting port types and library modules, and optionally a set of example compositions. Like a compiled node class, a node set needs to have the file extension `.vuonode`. 

To package files into a node set: 

   - Create a folder for your node set. 
   - Place the node classes and other files in the proper location in that folder. (See the rest of this section for details.) 
   - Zip the node set folder. 
   - Rename the resulting zip file to consist of your node set name plus the `.vuonode` extension (e.g. `mycompany.speechSynthesis.vuonode`). 


## Node classes, port types, and library modules

Place the compiled node class (`.vuonode`) files and compiled port type and library module (`.bc`) files in the top level of the node set folder. 

If any node class uses generic types, place the node class's source code (`.c`) file and any included header files (except for header files like @c node.h provided in Vuo.framework) in the top level of the node set folder. 


## Node set description

If you'd like to provide documentation that gives an overview of the node set, create a folder called `descriptions` in the top level of the node set folder, and create a file with the node set name plus the `.md` extension (e.g. `mycompany.speechSynthesis.md`) in the top level of that folder. Write the documentation in that file using [Markdown formatting](http://daringfireball.net/projects/markdown/). 

The Vuo Editor displays a link to to the node set description in the node class documentation that pops up when you click on a node class in the Node Library. 


## Node class descriptions

As an alternative to the "description" key in @ref VuoModuleMetadata, you can place a node class's description in a separate [Markdown-formatted](http://daringfireball.net/projects/markdown/) file. Create a folder called `descriptions` in the top level of the node set folder, and add a file whose name is the node class name plus the `.md` extension. For example, a node class whose source file is `mycompany.math/mycompany.math.add.c` would have documentation in `mycompany.math/descriptions/mycompany.math.add.md`. This file is only used if the node class's @ref VuoModuleMetadata does not specify a description. 


## Example compositions

If you have any example compositions, create a folder called `examples` in the top level of the node set folder. Place the composition (`.vuo`) files in the top level of that folder. 

The Vuo Editor lists the example compositions for each node set in the File > Examples menu. 

The Vuo Editor can also display example compositions that pertain to a specific node class, if you set this up in the node class definition. In @ref VuoModuleMetadata, you can use the "exampleCompositions" key to provide a list of compositions. (See the documentation for @ref VuoModuleMetadata for details.) The Vuo Editor will display this list of compositions in the node class documentation that pops up when you click on the node class in the Node Library. 

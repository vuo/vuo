@addtogroup DevelopingNodeClassesTypes

When you create a Vuo composition in the Vuo editor, you drag nodes from the Node Library onto your composition. Each item in the Node Library is a @term{node class}, or type of node. When you add a node to your composition, you're creating an instance of a node class. This documentation explains how to develop new node classes, which you can add to your Node Library and share with other Vuo users.

To learn how to develop node classes, see: 

   - This API documentation
      - @ref DevelopingNodeClasses — How to create a node class
      - @ref DevelopingTypes — How to create a data type that can be used by node classes
      - @ref DevelopingLibraryModules — How to create a library of shared code that can be used by node classes and types
      - @ref ManagingDependencies — How to use libraries and frameworks in node classes, types, and library modules
      - @ref ManagingMemory — How to prevent memory leaks and corruption in node classes, types, and library modules
      - @ref ManagingConcurrency — How to be thread-safe in node classes, types, and library modules
      - @ref WorkingWithGraphics — How to work with images, scenes, and shaders
      - @ref PackagingNodeSets — How to bundle node classes and supporting files into a node set
   - The example projects found in the `/Library/Developer/Vuo/example/node` folder after installing the Vuo SDK
   - The source code for Vuo's built-in node classes, types, and library modules

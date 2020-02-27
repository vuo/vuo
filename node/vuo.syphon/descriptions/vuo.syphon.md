Nodes for sharing video and images with other applications in real time through Syphon.

Using [Syphon](http://syphon.v002.info/), you can share video and images between a Vuo composition and another application, between two Vuo compositions, or between different parts of the same Vuo composition. Because video and images are shared directly on the graphics card, HD video can be transferred at a high framerate.

With Syphon, one application (the Syphon **server**) can send video or still images to one or more other applications (the Syphon **clients**). The server publishes a series of images, which are received by each client connected to the server. 

Each Syphon server has a **UUID** (unique ID) assigned by Syphon and, optionally, a **name** assigned by the application running the server. 

A Syphon client can get a list of all available Syphon servers and pick one to connect to. Or the client can look up a specific server by the name of the server or the name of the application running the server. On macOS, you can look up an application's name in the Activity Monitor application.

To receive images into a Vuo composition from a Syphon server, use the [Receive Syphon Video](vuo-node://vuo.syphon.receive) node.

To send images from a Vuo composition to Syphon clients, use the [Send Syphon Video](vuo-node://vuo.syphon.send) node.

To see the available Syphon servers, use the [List Syphon Servers](vuo-node://vuo.syphon.listServers) node. 

Syphon is developed by [Tom Butterworth](http://kriss.cx/tom) and [Anton Marini](http://vade.info/).

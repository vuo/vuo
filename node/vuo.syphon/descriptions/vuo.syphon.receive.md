Fires an event each time this Syphon client receives an image from a Syphon server. 

You can use this node to feed the output of another app into your Vuo composition.  (If the app doesn't support Syphon, you could alternatively use Vuo's [Capture Image of Screen](vuo-node://vuo.screen.capture) node.)

When the composition starts, when this node is added to a running composition, or when this node receives an event, it tries to connect to the server. If the server is not yet available, this node will automatically connect to the server when it becomes available.

How this node chooses the server:

   - If the `Server` port has a constant value:
      - With "First available server", this node chooses arbitrarily among the running servers.
      - With the other menu items, this node chooses one of the servers running when the input editor was opened.
   - If the `Server` port has an incoming cable:
	  - This node chooses arbitrarily among the servers that match the server name (if provided) and application name (if provided). An exact match is not required; a connected node such as [Specify Syphon Server](vuo-node://vuo.syphon.make.serverDescription2) can specify just part of the name.
      - If an empty server is provided (such as the output of [Get First Item in List](vuo-node://vuo.list.get.first) for an empty list), this node chooses arbitrarily among the running servers.

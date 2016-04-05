Fires an event each time this Syphon client receives an image from a Syphon server. 

You can use this node to feed the output of another app into your Vuo composition.  (If the app doesn't support Syphon, you could alternatively use Vuo's `Capture Screen Region` node.)

When searching for the server, this node will restrict its search to those that match the server name (if provided) and application name (if provided) in `Server Description`. An exact match is not required; `Server Description` can specify just part of the name. If no `Server Description` is provided, then this node will search among all available servers. If multiple servers are found, then one of them is arbitrarily chosen. 

When the composition starts, when this node is added to a running composition, or when this node receives an event, it tries to connect to the server. If the server is not yet available, this node will automatically connect to the server when it becomes available. 

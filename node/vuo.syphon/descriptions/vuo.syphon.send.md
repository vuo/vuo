Creates a Syphon server and sends frames to any connected Syphon clients. 

When the composition starts or this node is added to a running composition, it creates a Syphon server that clients can connect to. 

   - `serverName` — A name to identify this server, allowing clients to look up the server by name. This is optional. 
   - `sendImage` - When this port receives an event, its image is sent to all connected clients. 

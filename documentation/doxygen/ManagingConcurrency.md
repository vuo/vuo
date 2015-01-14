@addtogroup ManagingConcurrency

Vuo compositions are multithreaded to improve performance. When a composition is compiled, the Vuo compiler (a parallelizing compiler) generates code that allows independent parts of the composition to run concurrently. The creator of a composition can control some of the composition's threading behavior, such as allowing a long-running task to run in the background (see the [Vuo Manual](http://vuo.org/manual.pdf)). 

The Vuo compiler handles most thread synchronization automatically. It guarantees that each node's functions (@ref VuoNodeMethodsStateless and @ref VuoNodeMethodsStateful) will be called sequentially (one at a time), not concurrently. For example, two calls to nodeEvent(), or a call to nodeInstanceEvent() and a call to nodeInstanceTriggerStop(), will never overlap for a node (although they may overlap for different *instances* of a node *class*). A node class's functions don't need to be mutually thread-safe or re-entrant. 
   
There are some cases where you need to handle synchronization when implementing node classes, port types, and library modules: 

   - If resources are shared between multiple nodes, they need to be thread-safe. (For example, see @ref VuoGlPool.cc.) 
   - If a node class fires events on a background thread or timer, it may only do so between the time when nodeInstanceTriggerStart() is called and the time when nodeInstanceTriggerStop() returns. 

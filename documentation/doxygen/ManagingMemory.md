@addtogroup ManagingMemory

When a node allocates memory on the heap and sends the pointer through an output port, the pointer travels to other nodes. The other nodes can hold onto that pointer for any amount of time. There's no guarantee of which node will be the last to use the pointer. Who owns that heap-allocated data? Who is responsible for deallocating it? 

The answer is that the Vuo runtime takes ownership of the data as soon as it leaves the node, and is responsible for deallocating it. The Vuo runtime uses [reference counting](http://en.wikipedia.org/wiki/Reference_counting) to keep track of heap-allocated memory and deallocate it when all nodes are finished using it. 

You need to use Vuo's reference-counting system to manage memory if: 

   - You're implementing a port type that has heap-allocated data. 
   - You're implementing a stateful node class. 

Read on to learn how. 



## The reference-counting functions

Vuo provides three functions for reference-counting: 

  - VuoRegister() informs the Vuo runtime that the data should be reference counted. When the reference count of the data is decremented back to zero (its original value), the data will be deallocated. This function should be called once on the data. 
  - VuoRetain() increments the data's reference count, informing the Vuo runtime that someone has started using the data. The VuoRegister() function must be called on the data before the first call to VuoRetain(). 
  - VuoRelease() decrements the data's reference count, informing the Vuo runtime that someone has finished using the data. The VuoRetain() function must have been called at least *N* times on the data before the *N*-th call to VuoRelease(). 

The Vuo compiler automatically inserts most of the necessary calls to VuoRegister(), VuoRetain(), and VuoRelease(). If you're implementing a stateless node class, you never need to call these functions. If you're implementing a stateful node class or a port type, read on to learn when to call these functions. 



## Automatically-inserted calls to the reference-counting functions

In many cases, the Vuo compiler automatically inserts calls to VuoRetain() and VuoRelease() where needed: 

   - After a call to nodeInstanceInit(), the returned instance data is retained. 
   - After a call to nodeEvent() or nodeInstanceEvent(), for each output port and the instance data, the old data is released and the new data is retained. 
   - After a call to nodeInstanceFini(), the instance data is released. 
   - When port's value changes, the old data is released and the new data is retained. 
   - When a composition is stopped, for each port, the data is released. 

If the port type or instance data type is a pointer, the Vuo compiler calls VuoRetain() or VuoRelease() on the data itself. If the port type or instance data type is a struct, the Vuo compiler recursively traverses the struct and calls VuoRetain() or VuoRelease() on each field that is a pointer. 



## Managing memory for a port type

This section describes how to use reference-counting for port types that use heap-allocated data in various ways. 


### Port type is a heap-allocated type

If you're defining a port type that's a pointer to heap-allocated data, then your port type is responsible for registering that data. For example: 

@code{.cc}
typedef char * MyString;

MyString MyString_valueFromJson(json_object * js)
{
	const char *s = "";
	if (json_object_get_type(js) == json_type_string)
		*s = json_object_get_string(js);
	MyString *m = strdup(s);
	VuoRegister(m, free);
	return m;
}
@endcode


### Port type is a struct containing a heap-allocated type

If you're defining a port type that's a struct, and the struct contains pointers to heap-allocated data, then your port type is responsible for registering that data. For example: 

@code{.cc}
typedef struct {
	float *elements;
	size_t elementCount;
} ArrayOfFloats;

ArrayOfFloats ArrayOfFloats_valueFromJson(json_object * js)
{
	ArrayOfFloats a;
	a.elementCount = ...;
	a.elements = (float *)calloc(elementCount, sizeof(float));
	VuoRegister(a.elements, free);
	...;
	return a;
}
@endcode


### Port type is an opaque container for a heap-allocated type

If you're defining a port type that's some kind of container than a struct, and it contains pointers to heap-allocated data, then you have to do a little more work. Unlike a struct, the Vuo compiler can't analyze your container to automatically insert calls to VuoRetain() and VuoRelease(). Your port type is responsible for registering, retaining, and releasing the items in the container, as well as registering the container itself. For example: 

@code{.cc}
typedef VuoText * PairOfStrings;

PairOfStrings PairOfStrings_valueFromJson(json_object * js)
{
	PairOfStrings pair = (PairOfStrings)calloc(2, sizeof(VuoText));
	VuoRegister(pair, PairOfStrings_destroy);
	return pair;
}

void PairOfStrings_destroy(void *p)
{
	PairOfStrings pair = (PairOfStrings)p;
	VuoRelease(pair[0]);
	VuoRelease(pair[1]);	
	free(pair);
}

void PairOfStrings_replaceFirst(PairOfStrings pair, VuoText newFirst)
{
	VuoRetain(newFirst);
	VuoRelease(pair[0]);
	pair[0] = newFirst;
}
@endcode

If instead of @ref VuoText this container held a struct port type such as @ref VuoSceneObject, then you would replace each call to VuoRetain() or VuoRelease() with a call to MyType_retain() or MyType_release(). In fact, you can call MyType_retain() or MyType_release() for any type. These functions are automatically generated by the Vuo compiler (though need to be declared in `MyType.h`). Depending on the underlying data type of `MyType`, they call VuoRetain() and VuoRelease() as needed. 



## Managing memory for node instance data

A stateful node class needs to register its instance data before returning it from nodeInstanceInit(). Use VuoRegister(), just like in the examples above for MyType_valueFromJson() in port types. 

If the instance data is a container for heap-allocated items, then it needs to retain items that it stores and and release items that it no longer stores. Use VuoRetain() and VuoRelease(), just like in the example above for the opaque container port type. 

/**
 * @file
 * VuoPool interface and implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOPOOL_HH
#define VUOPOOL_HH

#include <string>
#include <map>
#include <dispatch/dispatch.h>

/**
 * A pool of shared objects — one object per key.
 *
 * Each shared object is released immediately when its last user calls @ref VuoRelease.
 *
 * It's safe for multiple threads to call @ref getSharedInstance() on the same pool.
 */
template<typename KeyType, typename InstanceType>
class VuoKeyedPool
{
public:
	/// A function that allocates @c InstanceType for a given key.
	typedef InstanceType (*AllocateFunctionType)(KeyType);

	VuoKeyedPool(std::string instanceTypeString, AllocateFunctionType allocate);
	InstanceType getSharedInstance(KeyType key);
	void removeSharedInstance(KeyType key);
	void visit(void (^b)(KeyType,InstanceType));
	unsigned int size(void);

private:
	std::string instanceTypeString;	///< The name of the data type this pool holds (for debugging).
	AllocateFunctionType allocate;	///< To be called when a new shared instance is needed.
	dispatch_queue_t queue;			///< Protects access to a pool instance's ivars.
	typedef std::map<KeyType,InstanceType> PoolType;
	PoolType pool;
};

/**
 * Declares a VuoKeyedPool instance.
 */
#define VUOKEYEDPOOL(keyType,instanceType) extern VuoKeyedPool<keyType, instanceType> * instanceType ## Pool

/**
 * Defines a VuoKeyedPool instance.
 *
 * @param keyType The C or C++ type to be used as this pool's key.
 * @param instanceType The C or C++ type for this pool's objects.
 * @param allocate A callback function to allocate and initialize a shared object. The deallocation function (passed to @ref VuoRegister in the allocate function) should call @c removeSharedInstance before deinitializing and deallocating the object.
 */
#define VUOKEYEDPOOL_DEFINE(keyType,instanceType,allocate) VuoKeyedPool<keyType, instanceType> * instanceType ## Pool = new VuoKeyedPool<keyType, instanceType>(#instanceType, allocate)

/**
 * See @ref VUOKEYEDPOOL_DEFINE.
 */
template<typename KeyType, typename InstanceType>
VuoKeyedPool<KeyType,InstanceType>::VuoKeyedPool(std::string instanceTypeString, AllocateFunctionType allocate)
{
//	VLog("%s",instanceTypeString.c_str());
	this->instanceTypeString = instanceTypeString;
	this->allocate = allocate;
	this->queue = dispatch_queue_create(instanceTypeString.c_str(), NULL);
}

/**
 * Returns a shared object for the specified @c key, allocating it if necessary.
 *
 * @threadAny
 */
template<typename KeyType, typename InstanceType>
InstanceType VuoKeyedPool<KeyType,InstanceType>::getSharedInstance(KeyType key)
{
//	VLog("%s key=%d",instanceTypeString.c_str(),key);
	__block InstanceType instance;
	dispatch_sync(queue, ^{
		typename PoolType::iterator it = pool.find(key);
		if (it != pool.end())
			instance = it->second;
		else
		{
			instance = allocate(key);
			pool[key] = instance;
		}
	});
	return instance;
}

/**
 * Removes @c key from the pool.
 * This should only be called by objects' deallocate functions (as passed to @c VuoRegister) prior to deinitializing and deallocating the object.
 *
 * @threadAny
 */
template<typename KeyType, typename InstanceType>
void VuoKeyedPool<KeyType,InstanceType>::removeSharedInstance(KeyType key)
{
//	VLog("%s key=%d",instanceTypeString.c_str(),key);
	dispatch_sync(queue, ^{
		pool.erase(key);
	});
}

/**
 * Invokes `block` on each item in the pool.
 *
 * @threadAny
 */
template<typename KeyType, typename InstanceType>
void VuoKeyedPool<KeyType,InstanceType>::visit(void (^b)(KeyType,InstanceType))
{
	dispatch_sync(queue, ^{
					  for (typename PoolType::iterator i = pool.begin(); i != pool.end(); ++i)
						  b(i->first, i->second);
	});
}

/**
 * Returns the number of items in the pool.
 *
 * @threadAny
 */
template<typename KeyType, typename InstanceType>
unsigned int VuoKeyedPool<KeyType,InstanceType>::size(void)
{
	__block unsigned int c;
	dispatch_sync(queue, ^{
		c = pool.size();
	});
	return c;
}

#endif // VUOPOOL_HH

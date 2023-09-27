/**
 * @file
 * VuoPool interface and implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include <string>
#include <map>
#include <dispatch/dispatch.h>

/**
 * A pool of shared objects — one object per key.
 *
 * A shared object is allocated when it's first requested by @ref useSharedInstance,
 * and released once all users call @ref disuseSharedInstance
 * (that is, when the `useSharedInstance` and `disuseSharedInstance` calls are balanced).
 *
 * It's safe for multiple threads to simultaneously call any public methods on the same pool.
 */
template<typename KeyType, typename InstanceType>
class VuoKeyedPool
{
public:
	/// A function that allocates @c InstanceType for a given key.
	typedef InstanceType (*AllocateFunctionType)(KeyType);

	VuoKeyedPool(std::string instanceTypeString, AllocateFunctionType allocate);
	InstanceType useSharedInstance(KeyType key);
	void disuseSharedInstance(InstanceType instance);
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
 * @param allocate A callback function that should allocate, @ref VuoRegister, and initialize a shared object.
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
 * Returns a shared object for the specified key, allocating it if necessary.
 *
 * The caller must invoke `disuseShared` when it's done using it.
 *
 * The caller may invoke @ref VuoRetain and @ref VuoRelease, but it is not required,
 * and any such calls must be balanced before invoking `disuseShared`.
 *
 * @threadAny
 */
template<typename KeyType, typename InstanceType>
InstanceType VuoKeyedPool<KeyType,InstanceType>::useSharedInstance(KeyType key)
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

		VuoRetain(instance);
	});
	return instance;
}

/**
 * Indicates that the caller is done using the shared object.
 *
 * @threadAny
 */
template<typename KeyType, typename InstanceType>
void VuoKeyedPool<KeyType,InstanceType>::disuseSharedInstance(InstanceType instance)
{
	if (!instance)
		return;

//	VLog("%s key=%d",instanceTypeString.c_str(),key);
	dispatch_sync(queue, ^{
		for (auto it = pool.begin(); it != pool.end(); ++it)
			if (it->second == instance)
			{
				int referenceCount = VuoRelease(it->second);
				if (referenceCount == 0)
				{
					pool.erase(it);
					break;
				}
			}
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

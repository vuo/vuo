/**
 * @file
 * VuoTriggerSet interface and implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include <set>
#include <dispatch/dispatch.h>

/**
 * Manages a set of callbacks for nodes' trigger ports.
 *
 * It's safe for multiple threads to call @ref addTrigger(), @ref removeTrigger(), @ref size(), and @ref fire() on the same set.
 */
template<typename TriggerDataType, typename TriggerContextType = void *>
class VuoTriggerSet
{
public:
	/// A node's trigger method.
	typedef void (*TriggerFunctionType)(TriggerDataType);

	VuoTriggerSet();
	~VuoTriggerSet();

	void addTrigger(TriggerFunctionType trigger);
	void addTrigger(TriggerFunctionType trigger, TriggerContextType context);
	void removeTrigger(TriggerFunctionType trigger);
	unsigned int size(void);

	void fire(TriggerDataType data);
	void fire(void (^)(TriggerFunctionType trigger, TriggerContextType context));

private:
	dispatch_queue_t queue;	///< Serializes access to the following trigger method.
	std::set<TriggerFunctionType> triggers;
	std::set< std::pair<TriggerFunctionType, TriggerContextType> > triggersWithContext;
};

/**
 * Creates a new trigger set.
 */
template<typename TriggerDataType, typename TriggerContextType>
VuoTriggerSet<TriggerDataType, TriggerContextType>::VuoTriggerSet()
{
	this->queue = dispatch_queue_create("VuoTriggerSet", NULL);
}

/**
 * Destroys the trigger set.
 */
template<typename TriggerDataType, typename TriggerContextType>
VuoTriggerSet<TriggerDataType, TriggerContextType>::~VuoTriggerSet()
{
#if !__has_feature(objc_arc)
	dispatch_release(this->queue);
#endif
}

/**
 * Adds a trigger method to the trigger set.
 *
 * @threadAny
 */
template<typename TriggerDataType, typename TriggerContextType>
void VuoTriggerSet<TriggerDataType, TriggerContextType>::addTrigger(TriggerFunctionType trigger)
{
	dispatch_sync(queue, ^{
		triggers.insert(trigger);
	});
}

/**
 * Adds a trigger method, with caller-defined context data, to the trigger set.
 *
 * @threadAny
 */
template<typename TriggerDataType, typename TriggerContextType>
void VuoTriggerSet<TriggerDataType, TriggerContextType>::addTrigger(TriggerFunctionType trigger, TriggerContextType context)
{
	dispatch_sync(queue, ^{
		triggersWithContext.insert(std::make_pair(trigger, context));
	});
}

/**
 * Removes a trigger method from the trigger set.
 *
 * @threadAny
 */
template<typename TriggerDataType, typename TriggerContextType>
void VuoTriggerSet<TriggerDataType, TriggerContextType>::removeTrigger(TriggerFunctionType trigger)
{
	dispatch_sync(queue, ^{
		triggers.erase(trigger);

		for (typename std::set< std::pair<TriggerFunctionType, TriggerContextType> >::iterator it = triggersWithContext.begin(); it != triggersWithContext.end(); )
			if (it->first == trigger)
				triggersWithContext.erase(it++);
			else
				++it;
	});
}

/**
 * Returns the number of triggers in the trigger set.
 *
 * @threadAny
 */
template<typename TriggerDataType, typename TriggerContextType>
unsigned int VuoTriggerSet<TriggerDataType, TriggerContextType>::size(void)
{
	__block unsigned int size;
	dispatch_sync(queue, ^{
		size = triggers.size() + triggersWithContext.size();
	});
	return size;
}

/**
 * Fires the contextless triggers in the trigger set, passing them @c data.
 *
 * @threadAny
 */
template<typename TriggerDataType, typename TriggerContextType>
void VuoTriggerSet<TriggerDataType, TriggerContextType>::fire(TriggerDataType data)
{
	dispatch_sync(queue, ^{
					   for (typename std::set<TriggerFunctionType>::iterator it = triggers.begin(); it != triggers.end(); ++it)
						   (*it)(data);
				   });
}

/**
 * Fires the context-having triggers in the trigger set, passing them @c data.
 *
 * @threadAny
 */
template<typename TriggerDataType, typename TriggerContextType>
void VuoTriggerSet<TriggerDataType, TriggerContextType>::fire(void (^block)(TriggerFunctionType trigger, TriggerContextType context))
{
	dispatch_sync(queue, ^{
		for (typename std::set< std::pair<TriggerFunctionType, TriggerContextType> >::iterator it = triggersWithContext.begin(); it != triggersWithContext.end(); ++it)
			block(it->first, it->second);
	});
}

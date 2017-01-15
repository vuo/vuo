/**
 * @file
 * VuoTriggerSet interface and implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOTRIGGERSET_HH
#define VUOTRIGGERSET_HH

#include <set>
#include <dispatch/dispatch.h>

/**
 * Manages a set of callbacks for nodes' trigger ports.
 *
 * It's safe for multiple threads to call @ref addTrigger(), @ref removeTrigger(), @ref size(), and @ref fire() on the same set.
 */
template<typename TriggerDataType>
class VuoTriggerSet
{
public:
	/// A node's trigger method.
	typedef void (*TriggerFunctionType)(TriggerDataType);

	VuoTriggerSet();
	~VuoTriggerSet();

	void addTrigger(TriggerFunctionType trigger);
	void removeTrigger(TriggerFunctionType trigger);
	unsigned int size(void);

	void fire(TriggerDataType data);

private:
	dispatch_queue_t queue;	///< Serializes access to the following trigger method.
	std::set<TriggerFunctionType> triggers;
};

/**
 * Creates a new trigger set.
 */
template<typename TriggerDataType>
VuoTriggerSet<TriggerDataType>::VuoTriggerSet()
{
	this->queue = dispatch_queue_create("VuoTriggerSet", NULL);
}

/**
 * Destroys the trigger set.
 */
template<typename TriggerDataType>
VuoTriggerSet<TriggerDataType>::~VuoTriggerSet()
{
	dispatch_release(this->queue);
}

/**
 * Adds a trigger method to the trigger set.
 *
 * @threadAny
 */
template<typename TriggerDataType>
void VuoTriggerSet<TriggerDataType>::addTrigger(TriggerFunctionType trigger)
{
	dispatch_sync(queue, ^{
					  triggers.insert(trigger);
				  });
}

/**
 * Removes a trigger method from the trigger set.
 *
 * @threadAny
 */
template<typename TriggerDataType>
void VuoTriggerSet<TriggerDataType>::removeTrigger(TriggerFunctionType trigger)
{
	dispatch_sync(queue, ^{
					  triggers.erase(trigger);
				  });
}

/**
 * Returns the number of triggers in the trigger set.
 *
 * @threadAny
 */
template<typename TriggerDataType>
unsigned int VuoTriggerSet<TriggerDataType>::size(void)
{
	__block unsigned int size;
	dispatch_sync(queue, ^{
					  size = triggers.size();
				  });
	return size;
}

/**
 * Fires all the triggers in the trigger set, passing them @c data.
 *
 * @threadAny
 */
template<typename TriggerDataType>
void VuoTriggerSet<TriggerDataType>::fire(TriggerDataType data)
{
	dispatch_sync(queue, ^{
					   for (typename std::set<TriggerFunctionType>::iterator it = triggers.begin(); it != triggers.end(); ++it)
						   (*it)(data);
				   });
}

#endif // VUOTRIGGERSET_HH

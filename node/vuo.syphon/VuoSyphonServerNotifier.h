/**
 * @file
 * VuoSyphonServerNotifier interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOSYPHONSERVERNOTIFIER_H
#define VUOSYPHONSERVERNOTIFIER_H

#include "node.h"
#include "VuoSyphonServerDescription.h"
#include "VuoList_VuoSyphonServerDescription.h"
#include <objc/runtime.h>

/**
 * Keeps track of notifications when the set of available Syphon servers changes.
 */
typedef void * VuoSyphonServerNotifier;

VuoSyphonServerNotifier VuoSyphonServerNotifier_make(void);
void VuoSyphonServerNotifier_setNotificationFunction(VuoSyphonServerNotifier serverNotifier,
													 VuoOutputTrigger(serversChanged, VuoList_VuoSyphonServerDescription));
void VuoSyphonServerNotifier_setNotificationMethod(VuoSyphonServerNotifier serverNotifier,
												   id object, SEL method);
void VuoSyphonServerNotifier_start(VuoSyphonServerNotifier serverNotifier);
void VuoSyphonServerNotifier_stop(VuoSyphonServerNotifier serverNotifier);

#endif

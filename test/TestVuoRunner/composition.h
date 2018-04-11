/**
 * @file
 * composition interface.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoRuntime.h"

//@{
/**
 * Normally defined in the composition's generated code. Defined here to prevent link errors.
 */

const char *vuoTopLevelCompositionIdentifier = "Top";

int VuoRelease(void *heapPointer)
{
	return 0;
}
void vuoSetup(void)
{
}
void vuoCleanup(void)
{
}
void vuoInstanceInit(void)
{
}
void vuoInstanceTriggerStart(void)
{
}
void vuoInstanceTriggerStop(void)
{
}
char * vuoGetPortValue(const char *portIdentifier, int serializationType)
{
	return NULL;
}
void vuoSetInputPortValue(const char *portIdentifier, char *valueAsString)
{
}
void fireTriggerPortEvent(const char *portIdentifier)
{
}
unsigned int getPublishedInputPortCount(void)
{
	return 0;
}
unsigned int getPublishedOutputPortCount(void)
{
	return 0;
}
char ** getPublishedInputPortNames(void)
{
	return NULL;
}
char ** getPublishedOutputPortNames(void)
{
	return NULL;
}
char ** getPublishedInputPortTypes(void)
{
	return NULL;
}
char ** getPublishedOutputPortTypes(void)
{
	return NULL;
}
char ** getPublishedInputPortDetails(void)
{
	return NULL;
}
char ** getPublishedOutputPortDetails(void)
{
	return NULL;
}
int getPublishedInputPortConnectedIdentifierCount(const char *name)
{
	return 0;
}
int getPublishedOutputPortConnectedIdentifierCount(const char *name)
{
	return 0;
}
char ** getPublishedInputPortConnectedIdentifiers(const char *name)
{
	return NULL;
}
char ** getPublishedOutputPortConnectedIdentifiers(const char *name)
{
	return NULL;
}
void firePublishedInputPortEvent(const char *name)
{
}
void setPublishedInputPortValue(const char *portIdentifier, const char *valueAsString)
{
}
char * getPublishedInputPortValue(const char *portIdentifier, int shouldUseInterprocessSerialization)
{
	return NULL;
}
char * getPublishedOutputPortValue(const char *portIdentifier, int shouldUseInterprocessSerialization)
{
	return NULL;
}

//@}

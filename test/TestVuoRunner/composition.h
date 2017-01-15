/**
 * @file
 * composition interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoRuntime.h"

extern bool isStopped;

//@{
/**
 * Normally defined in VuoRuntimeHelper. Defined here to prevent link errors.
 */
bool isPaused = false;
char *compositionDiff = NULL;
//@}

//@{
/**
 * Normally defined in the composition's generated code. Defined here to prevent link errors.
 */
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
char * vuoGetPortValue(char *portIdentifier, int serializationType)
{
	return NULL;
}
void vuoSetInputPortValue(char *portIdentifier, char *valueAsString)
{
}
void fireTriggerPortEvent(char *portIdentifier)
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
int getPublishedInputPortConnectedIdentifierCount(char *name)
{
	return 0;
}
int getPublishedOutputPortConnectedIdentifierCount(char *name)
{
	return 0;
}
char ** getPublishedInputPortConnectedIdentifiers(char *name)
{
	return NULL;
}
char ** getPublishedOutputPortConnectedIdentifiers(char *name)
{
	return NULL;
}
void firePublishedInputPortEvent(char *name)
{
}
void setPublishedInputPortValue(char *portIdentifier, char *valueAsString)
{
}
char * getPublishedInputPortValue(char *portIdentifier, int shouldUseInterprocessSerialization)
{
	return NULL;
}
char * getPublishedOutputPortValue(char *portIdentifier, int shouldUseInterprocessSerialization)
{
	return NULL;
}

//@}

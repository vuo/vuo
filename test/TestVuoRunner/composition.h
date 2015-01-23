/**
 * @file
 * composition interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoRuntime.h"

extern bool isStopped;

//@{
/**
 * Normally defined in the composition's generated code. Defined here to prevent link errors.
 */
int VuoRelease(void *heapPointer)
{
	return 0;
}
void setup(void)
{
}
void cleanup(void)
{
}
void nodeInstanceInit(void)
{
}
void nodeInstanceTriggerStart(void)
{
}
void nodeInstanceTriggerStop(void)
{
}
char * getInputPortValue(char *portIdentifier)
{
	return NULL;
}
char * getOutputPortValue(char *portIdentifier)
{
	return NULL;
}
char * getInputPortSummary(char *portIdentifier)
{
	return NULL;
}
char * getOutputPortSummary(char *portIdentifier)
{
	return NULL;
}
void setInputPortValue(char *portIdentifier, char *valueAsString, int shouldUpdateCallbacks)
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
int isPaused = 0;
//@}

/**
 * @file
 * VuoCompositionStub implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

//@{
/**
 * Normally defined in a composition's generated code.
 * Defined here to prevent link errors when linking in VuoRuntime without also linking in a composition.
 */
void setup(void)
{
}
void cleanup(void)
{
}
void nodeInstanceInit(void)
{
}
void nodeInstanceFini(void)
{
}
void nodeInstanceTriggerStart(void)
{
}
void nodeInstanceTriggerStop(void)
{
}
char * getPortValue(char *portIdentifier, bool isInput, bool isThreadSafe, int serializationType)
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

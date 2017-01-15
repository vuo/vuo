/**
 * @file
 * VuoCompilerNodeArgumentClass implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerNodeArgumentClass.hh"

#include "VuoPortClass.hh"

/**
 * Creates a parameter for calling a node's event or init function, and creates its corresponding base @c VuoPortClass.
 */
VuoCompilerNodeArgumentClass::VuoCompilerNodeArgumentClass(string name, enum VuoPortClass::PortType portType, Type *type)
	: VuoBaseDetail<VuoPortClass>("VuoPortClass", new VuoPortClass(name,portType))
{
	getBase()->setCompiler(this);

	this->type = type;

	inEventFunction = false;
	inInitFunction = false;
	inCallbackStartFunction = false;
	inCallbackUpdateFunction = false;
	inCallbackStopFunction = false;
}

/**
 * Destructor.
 */
VuoCompilerNodeArgumentClass::~VuoCompilerNodeArgumentClass(void)
{
}

/**
 * Returns the type of this argument.
 */
Type * VuoCompilerNodeArgumentClass::getType(void)
{
	return type;
}

/**
 * Returns true if this is known to be one of the parameters in its node class's event function.
 */
bool VuoCompilerNodeArgumentClass::isInEventFunction(void)
{
	return inEventFunction;
}

/**
 * Returns the index of this port's parameter in its node class's event function.
 *
 * Assumes this port is known to be one of the parameters in its node class's event function.
 */
size_t VuoCompilerNodeArgumentClass::getIndexInEventFunction(void)
{
	return indexInEventFunction;
}

/**
 * Indicates that this is the parameter at index @c indexInEventFunction in its node class's event function.
 */
void VuoCompilerNodeArgumentClass::setIndexInEventFunction(size_t indexInEventFunction)
{
	this->indexInEventFunction = indexInEventFunction;
	inEventFunction = true;
}

/**
 * Returns true if this is known to be one of the parameters in its node class's init function.
 */
bool VuoCompilerNodeArgumentClass::isInInitFunction(void)
{
	return inInitFunction;
}

/**
 * Returns the index of this port's parameter in its node class's init function.
 *
 * Assumes this port is known to be one of the parameters in its node class's init function.
 */
size_t VuoCompilerNodeArgumentClass::getIndexInInitFunction(void)
{
	return indexInInitFunction;
}

/**
 * Indicates that this is the parameter at index @c indexInInitFunction in its node class's init function.
 */
void VuoCompilerNodeArgumentClass::setIndexInInitFunction(size_t indexInInitFunction)
{
	this->indexInInitFunction = indexInInitFunction;
	inInitFunction = true;
}

/**
 * Returns true if this is known to be one of the parameters in its node class's callback start function.
 */
bool VuoCompilerNodeArgumentClass::isInCallbackStartFunction(void)
{
	return inCallbackStartFunction;
}

/**
 * Returns the index of this port's parameter in its node class's callback start function.
 *
 * Assumes this port is known to be one of the parameters in its node class's callback start function.
 */
size_t VuoCompilerNodeArgumentClass::getIndexInCallbackStartFunction(void)
{
	return indexInCallbackStartFunction;
}

/**
 * Indicates that this is the parameter at index @c indexInCallbackStartFunction in its node class's callback start function.
 */
void VuoCompilerNodeArgumentClass::setIndexInCallbackStartFunction(size_t indexInCallbackStartFunction)
{
	this->indexInCallbackStartFunction = indexInCallbackStartFunction;
	inCallbackStartFunction = true;
}

/**
 * Returns true if this is known to be one of the parameters in its node class's callback update function.
 */
bool VuoCompilerNodeArgumentClass::isInCallbackUpdateFunction(void)
{
	return inCallbackUpdateFunction;
}

/**
 * Returns the index of this port's parameter in its node class's callback update function.
 *
 * Assumes this port is known to be one of the parameters in its node class's callback update function.
 */
size_t VuoCompilerNodeArgumentClass::getIndexInCallbackUpdateFunction(void)
{
	return indexInCallbackUpdateFunction;
}

/**
 * Indicates that this is the parameter at index @c indexInCallbackUpdateFunction in its node class's callback update function.
 */
void VuoCompilerNodeArgumentClass::setIndexInCallbackUpdateFunction(size_t indexInCallbackUpdateFunction)
{
	this->indexInCallbackUpdateFunction = indexInCallbackUpdateFunction;
	inCallbackUpdateFunction = true;
}

/**
 * Returns true if this is known to be one of the parameters in its node class's callback stop function.
 */
bool VuoCompilerNodeArgumentClass::isInCallbackStopFunction(void)
{
	return inCallbackStopFunction;
}

/**
 * Returns the index of this port's parameter in its node class's callback stop function.
 *
 * Assumes this port is known to be one of the parameters in its node class's callback stop function.
 */
size_t VuoCompilerNodeArgumentClass::getIndexInCallbackStopFunction(void)
{
	return indexInCallbackStopFunction;
}

/**
 * Indicates that this is the parameter at index @c indexInCallbackStopFunction in its node class's callback stop function.
 */
void VuoCompilerNodeArgumentClass::setIndexInCallbackStopFunction(size_t indexInCallbackStopFunction)
{
	this->indexInCallbackStopFunction = indexInCallbackStopFunction;
	inCallbackStopFunction = true;
}

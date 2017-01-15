/**
 * @file
 * VuoCompilerNodeArgumentClass interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERNODEARGUMENTCLASS_H
#define VUOCOMPILERNODEARGUMENTCLASS_H

#include "VuoBaseDetail.hh"
#include "VuoPortClass.hh"

/**
 * A parameter to a node's event and/or init function. To create an argument to pass
 * for this parameter, instantiate a @c VuoCompilerNodeArgument.
 *
 * @see VuoPortClass
 */
class VuoCompilerNodeArgumentClass : public VuoBaseDetail<VuoPortClass>
{
protected:
	Type *type; ///< The parameter's type.
	bool inEventFunction; ///< Does this parameter appear in the node's event function?
	size_t indexInEventFunction; ///< Where this parameter appears in the node's event function's parameter list.
	bool inInitFunction; ///< Does this parameter appear in the node's init function?
	size_t indexInInitFunction; ///< Where this parameter appears in the node's init function's parameter list.
	bool inCallbackStartFunction; ///< Does this parameter appear in the node's callback start function?
	size_t indexInCallbackStartFunction; ///< Where this parameter appears in the node's callback start function's parameter list.
	bool inCallbackUpdateFunction; ///< Does this parameter appear in the node's callback update function?
	size_t indexInCallbackUpdateFunction; ///< Where this parameter appears in the node's callback update function's parameter list.
	bool inCallbackStopFunction; ///< Does this parameter appear in the node's callback stop function?
	size_t indexInCallbackStopFunction; ///< Where this parameter appears in the node's callback stop function's parameter list.

	VuoCompilerNodeArgumentClass(string name, VuoPortClass::PortType portType, Type *type);

public:
	virtual ~VuoCompilerNodeArgumentClass(void);  ///< to make this class dynamic_cast-able

	virtual Type * getType(void);
	bool isInEventFunction(void);
	size_t getIndexInEventFunction(void);
	void setIndexInEventFunction(size_t indexInEventFunction);
	bool isInInitFunction(void);
	size_t getIndexInInitFunction(void);
	void setIndexInInitFunction(size_t indexInInitFunction);
	bool isInCallbackStartFunction(void);
	size_t getIndexInCallbackStartFunction(void);
	void setIndexInCallbackStartFunction(size_t indexInCallbackStartFunction);
	bool isInCallbackUpdateFunction(void);
	size_t getIndexInCallbackUpdateFunction(void);
	void setIndexInCallbackUpdateFunction(size_t indexInCallbackUpdateFunction);
	bool isInCallbackStopFunction(void);
	size_t getIndexInCallbackStopFunction(void);
	void setIndexInCallbackStopFunction(size_t indexInCallbackStopFunction);
};

#endif

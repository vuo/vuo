/**
 * @file
 * VuoCompilerTriggerPortClass interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERTRIGGERPORTCLASS_H
#define VUOCOMPILERTRIGGERPORTCLASS_H

#include "VuoCompilerPortClass.hh"

/**
 * A trigger output port type.
 *
 * \see{VuoCompilerTriggerPort}
 */
class VuoCompilerTriggerPortClass : public VuoCompilerPortClass
{
private:
	VuoType *vuoType;

public:
	VuoCompilerTriggerPortClass(string name, PointerType *type);
	VuoCompilerPort * newPort(void);
	VuoCompilerPort * newPort(VuoPort *port);
	FunctionType * getFunctionType(void);
	VuoType * getDataVuoType(void);
	void setDataVuoType(VuoType *type);
};

#endif

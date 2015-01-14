/**
 * @file
 * VuoCompilerInputEventPortClass interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERINPUTEVENTPORTCLASS_H
#define VUOCOMPILERINPUTEVENTPORTCLASS_H

#include "VuoCompilerEventPortClass.hh"
#include "VuoCompilerInputDataClass.hh"


/**
 * An input port type, optionally with data.
 */
class VuoCompilerInputEventPortClass : public VuoCompilerEventPortClass
{
public:
	VuoCompilerInputEventPortClass(string name, Type *type);
	VuoCompilerInputEventPortClass(string name);
	VuoCompilerPort * newPort(void);
	VuoCompilerPort * newPort(VuoPort *port);
	VuoCompilerInputDataClass * getDataClass(void);
};


#endif

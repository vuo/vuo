/**
 * @file
 * VuoCompilerOutputEventPortClass interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILEROUTPUTEVENTPORTCLASS_H
#define VUOCOMPILEROUTPUTEVENTPORTCLASS_H

#include "VuoCompilerEventPortClass.hh"

class VuoCompilerOutputDataClass;
class VuoCompilerPort;
class VuoPort;

/**
 * A passive (non-trigger) output port type, optionally with data.
 *
 * \see{VuoCompilerOutputEventPort}
 */
class VuoCompilerOutputEventPortClass : public VuoCompilerEventPortClass
{
public:
	VuoCompilerOutputEventPortClass(string name, Type *type);
	VuoCompilerOutputEventPortClass(string name);
	VuoCompilerPort * newPort(void);
	VuoCompilerPort * newPort(VuoPort *port);
	VuoCompilerOutputDataClass * getDataClass(void);
};


#endif

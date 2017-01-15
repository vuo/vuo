/**
 * @file
 * VuoCompilerInstanceDataClass interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERINSTANCEDATACLASS_H
#define VUOCOMPILERINSTANCEDATACLASS_H

#include "VuoCompilerNodeArgumentClass.hh"


/**
 * A type of node instance data.
 *
 * \see{VuoCompilerInstanceData}
 */
class VuoCompilerInstanceDataClass : public VuoCompilerNodeArgumentClass
{
public:
	VuoCompilerInstanceDataClass(string name, Type *type);
};


#endif

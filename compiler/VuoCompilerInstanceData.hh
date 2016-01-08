/**
 * @file
 * VuoCompilerInstanceData interface.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERINSTANCEDATA_H
#define VUOCOMPILERINSTANCEDATA_H

#include "VuoCompilerNodeArgument.hh"
#include "VuoCompilerInstanceDataClass.hh"


/**
 * The instance data for a node.
 *
 * \see{VuoCompilerInstanceDataClass}
 */
class VuoCompilerInstanceData : public VuoCompilerNodeArgument
{
public:
	VuoCompilerInstanceData(VuoCompilerInstanceDataClass *instanceDataClass);
};


#endif

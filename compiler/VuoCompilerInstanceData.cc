/**
 * @file
 * VuoCompilerInstanceData implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerInstanceData.hh"

#include "VuoPort.hh"

/**
 * Creates instance data for a node, based on the specified @c instanceDataClass.
 */
VuoCompilerInstanceData::VuoCompilerInstanceData(VuoCompilerInstanceDataClass *instanceDataClass)
	: VuoCompilerNodeArgument(new VuoPort(instanceDataClass->getBase()))
{
}

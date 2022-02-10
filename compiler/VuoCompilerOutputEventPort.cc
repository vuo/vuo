/**
 * @file
 * VuoCompilerOutputEventPort implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompilerOutputEventPort.hh"
#include "VuoCompilerOutputEventPortClass.hh"
#include "VuoCompilerOutputDataClass.hh"

/**
 * Creates a VuoCompilerOutputEventPort along with its VuoCompilerOutputEventPortClass.
 */
VuoCompilerOutputEventPort * VuoCompilerOutputEventPort::newPort(string name, VuoType *type)
{
	VuoCompilerOutputEventPortClass *portClass = new VuoCompilerOutputEventPortClass(name);

	if (type)
	{
		VuoCompilerOutputDataClass *dataClass = new VuoCompilerOutputDataClass("");
		dataClass->setVuoType(type);
		portClass->setDataClass(dataClass);
	}

	return static_cast<VuoCompilerOutputEventPort *>( portClass->newPort() );
}

/**
 * Creates a passive (non-trigger) output port based on @c portClass.
 */
VuoCompilerOutputEventPort::VuoCompilerOutputEventPort(VuoPort * basePort)
	: VuoCompilerEventPort(basePort)
{
}

/**
 * Returns this port's data, or NULL if none.
 */
VuoCompilerOutputData * VuoCompilerOutputEventPort::getData(void)
{
	return (VuoCompilerOutputData *)data;
}

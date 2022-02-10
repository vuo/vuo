/**
 * @file
 * VuoCompilerInputEventPort implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompilerInputEventPort.hh"
#include "VuoCompilerInputEventPortClass.hh"
#include "VuoCompilerInputDataClass.hh"
#include "VuoCompilerType.hh"
#include "VuoType.hh"

/**
 * Creates a VuoCompilerInputEventPort along with its VuoCompilerInputEventPortClass.
 */
VuoCompilerInputEventPort * VuoCompilerInputEventPort::newPort(string name, VuoType *type)
{
	VuoCompilerInputEventPortClass *portClass = new VuoCompilerInputEventPortClass(name);

	if (type)
	{
		VuoCompilerInputDataClass *dataClass = new VuoCompilerInputDataClass("");
		dataClass->setVuoType(type);
		portClass->setDataClass(dataClass);
	}

	return static_cast<VuoCompilerInputEventPort *>( portClass->newPort() );
}

/**
 * Creates an input port based on the specified @c portClass.
 */
VuoCompilerInputEventPort::VuoCompilerInputEventPort(VuoPort * basePort)
	: VuoCompilerEventPort(basePort)
{
}

/**
 * Returns this port's data, or NULL if none.
 */
VuoCompilerInputData * VuoCompilerInputEventPort::getData(void)
{
	return static_cast<VuoCompilerInputData *>(data);
}

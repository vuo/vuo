/**
 * @file
 * VuoCompilerInputEventPort implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerInputEventPort.hh"


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

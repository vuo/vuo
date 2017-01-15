/**
 * @file
 * VuoCompilerOutputEventPort implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerOutputEventPort.hh"

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

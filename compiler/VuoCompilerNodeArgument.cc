/**
 * @file
 * VuoCompilerNodeArgument implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerNodeArgument.hh"
#include "VuoCompilerNodeArgumentClass.hh"
#include "VuoPort.hh"

/**
 * Creates an argument instance based on the specified @c argumentClass.
 */
VuoCompilerNodeArgument::VuoCompilerNodeArgument(VuoPort *basePort)
	: VuoBaseDetail<VuoPort>("VuoCompilerNodeArgument", basePort)
{
	getBase()->setCompiler(this);
}

/**
 * Destructor.
 */
VuoCompilerNodeArgument::~VuoCompilerNodeArgument(void)
{
}

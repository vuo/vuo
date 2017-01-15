/**
 * @file
 * VuoCompilerNodeArgument interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERNODEARGUMENT_H
#define VUOCOMPILERNODEARGUMENT_H

#include "VuoBaseDetail.hh"
#include "VuoPort.hh"

/**
 * An argument to a node's event and/or init function. For each @c VuoCompilerNodeArgumentClass,
 * there are 0 or more instances of @c VuoCompilerNodeArgument.
 *
 * @see VuoPort
 */
class VuoCompilerNodeArgument : public VuoBaseDetail<VuoPort>
{
protected:
	VuoCompilerNodeArgument(VuoPort * basePort);

public:
	virtual ~VuoCompilerNodeArgument(void);
};

#endif

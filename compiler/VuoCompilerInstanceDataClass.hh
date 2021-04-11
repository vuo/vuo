/**
 * @file
 * VuoCompilerInstanceDataClass interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

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
	Type * getType(void);

private:
	Type *type;
};

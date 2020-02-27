/**
 * @file
 * VuoCompilerInputEventPort interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCompilerEventPort.hh"
#include "VuoCompilerInputData.hh"

class VuoPort;

/**
 * An input port, optionally with data.
 */
class VuoCompilerInputEventPort : public VuoCompilerEventPort
{
public:
	static VuoCompilerInputEventPort * newPort(string name, VuoType *type);
	VuoCompilerInputEventPort(VuoPort *basePort);
	VuoCompilerInputData * getData(void);
};

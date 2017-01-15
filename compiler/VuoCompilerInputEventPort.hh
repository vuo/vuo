/**
 * @file
 * VuoCompilerInputEventPort interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERINPUTEVENTPORT_H
#define VUOCOMPILERINPUTEVENTPORT_H

#include "VuoCompilerEventPort.hh"
#include "VuoCompilerInputData.hh"

class VuoPort;

/**
 * An input port, optionally with data.
 */
class VuoCompilerInputEventPort : public VuoCompilerEventPort
{
public:
	VuoCompilerInputEventPort(VuoPort *basePort);
	VuoCompilerInputData * getData(void);
};


#endif

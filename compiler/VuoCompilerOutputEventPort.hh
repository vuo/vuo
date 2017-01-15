/**
 * @file
 * VuoCompilerOutputEventPort interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILEROUTPUTEVENTPORT_H
#define VUOCOMPILEROUTPUTEVENTPORT_H

#include "VuoCompilerEventPort.hh"
#include "VuoCompilerOutputData.hh"

class VuoPort;

/**
 * A passive (non-trigger) output port, optionally with data.
 *
 * \see{VuoCompilerOutputEventPortClass}
 */
class VuoCompilerOutputEventPort : public VuoCompilerEventPort
{
public:
	VuoCompilerOutputEventPort(VuoPort *basePort);
	VuoCompilerOutputData * getData(void);
};


#endif

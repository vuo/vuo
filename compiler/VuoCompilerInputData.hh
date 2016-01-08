/**
 * @file
 * VuoCompilerInputData interface.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERINPUTDATA_H
#define VUOCOMPILERINPUTDATA_H

#include "VuoCompilerData.hh"
#include "VuoCompilerInputDataClass.hh"


/**
 * The data for a data-and-event input port.
 */
class VuoCompilerInputData : public VuoCompilerData
{
public:
	VuoCompilerInputData(VuoCompilerInputDataClass *dataClass);
	void setInitialValue(string initialValueAsString);
	string getInitialValue(void);

private:
	string initialValueAsString;
};


#endif

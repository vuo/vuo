/**
 * @file
 * VuoCompilerInputDataClass interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERINPUTDATACLASS_H
#define VUOCOMPILERINPUTDATACLASS_H

#include "VuoCompilerDataClass.hh"

class VuoCompilerData;

/**
 * The data type for a data-and-event input port.
 */
class VuoCompilerInputDataClass : public VuoCompilerDataClass
{
private:
	bool twoParameters;  /// True if this event function parameter in the node class implementation is lowered to two parameters in the LLVM bitcode.

public:
	VuoCompilerInputDataClass(string name, Type *type, bool twoParameters);
	VuoCompilerData * newData(void);
	string getDefaultValue(void);
	bool isLoweredToTwoParameters(void);
};

#endif

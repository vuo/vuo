/**
 * @file
 * VuoCompilerInputDataClass interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERINPUTDATACLASS_H
#define VUOCOMPILERINPUTDATACLASS_H

#include "VuoCompilerDataClass.hh"

/**
 * The data type for a data-and-event input port.
 */
class VuoCompilerInputDataClass : public VuoCompilerDataClass
{
private:
	struct json_object *details;  /// The default value and other details specified in the node class implementation.
	bool twoParameters;  /// True if this event function parameter in the node class implementation is lowered to two parameters in the LLVM bitcode.

public:
	VuoCompilerInputDataClass(string name, Type *type, bool twoParameters);
	VuoCompilerData * newData(void);
	void setDetails(struct json_object *details);
	json_object * getDetails(void);
	string getDefaultValue(void);
	bool isLoweredToTwoParameters(void);
};

#endif

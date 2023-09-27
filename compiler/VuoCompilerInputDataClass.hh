/**
 * @file
 * VuoCompilerInputDataClass interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCompilerDataClass.hh"

class VuoCompilerData;

/**
 * The data type for a data-and-event input port.
 */
class VuoCompilerInputDataClass : public VuoCompilerDataClass
{
public:
	explicit VuoCompilerInputDataClass(string name);
	VuoCompilerData * newData(void);
	string getDefaultValue(void);
	string getAutoValue(void);
	bool getAutoSupersedesDefaultValue(void);

	void setUnloweredStructPointerInEventFunction(Type *firstParameterType);
	bool isUnloweredStructPointerInEventFunction(void);
	void setUnloweredStructPointerInInitFunction(Type *firstParameterType);
	bool isUnloweredStructPointerInInitFunction(void);
	void setUnloweredStructPointerInCallbackStartFunction(Type *firstParameterType);
	bool isUnloweredStructPointerInCallbackStartFunction(void);
	void setUnloweredStructPointerInCallbackUpdateFunction(Type *firstParameterType);
	bool isUnloweredStructPointerInCallbackUpdateFunction(void);
	void setUnloweredStructPointerInCallbackStopFunction(Type *firstParameterType);
	bool isUnloweredStructPointerInCallbackStopFunction(void);

private:
	void setUnloweredStructPointer(Type *firstParameterType, bool &unloweredStructPointer);

	bool unloweredStructPointerInEventFunction;
	bool unloweredStructPointerInInitFunction;
	bool unloweredStructPointerInCallbackStartFunction;
	bool unloweredStructPointerInCallbackUpdateFunction;
	bool unloweredStructPointerInCallbackStopFunction;
};

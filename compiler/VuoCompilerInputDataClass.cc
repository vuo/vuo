/**
 * @file
 * VuoCompilerInputDataClass implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompilerInputDataClass.hh"

#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerInputData.hh"
#include "VuoStringUtilities.hh"
#include "VuoType.hh"

/**
 * Creates a data type for a data-and-event input port.
 *
 * The default data value for instances of this data type is not set. Set it with @c setDefaultValue.
 */
VuoCompilerInputDataClass::VuoCompilerInputDataClass(string name) :
	VuoCompilerDataClass(name)
{
	unloweredStructPointerInEventFunction = false;
	unloweredStructPointerInInitFunction = false;
	unloweredStructPointerInCallbackStartFunction = false;
	unloweredStructPointerInCallbackUpdateFunction = false;
	unloweredStructPointerInCallbackStopFunction = false;
}

/**
 * Creates a data instance based on this data type.
 */
VuoCompilerData * VuoCompilerInputDataClass::newData(void)
{
	return new VuoCompilerInputData(this);
}

/**
 * Returns the string representation of the default initial value of port data instantiated
 * from this data type.
 */
string VuoCompilerInputDataClass::getDefaultValue(void)
{
	if (details)
	{
		json_object *value = NULL;
		if (json_object_object_get_ex(details, "default", &value))
		{
			return json_object_to_json_string_ext(value, JSON_C_TO_STRING_PLAIN);
		}
		else if (json_object_object_get_ex(details, "defaults", &value))
		{
			json_object *valueForType = NULL;
			if (json_object_object_get_ex(value, getVuoType()->getModuleKey().c_str(), &valueForType))
				return json_object_to_json_string_ext(valueForType, JSON_C_TO_STRING_PLAIN);
		}
	}

	return "";
}

/**
 * Returns the string representation of @c auto value for port data instantiated
 * from this data type.
 */
string VuoCompilerInputDataClass::getAutoValue(void)
{
	if (details)
	{
		json_object *value = NULL;
		if (json_object_object_get_ex(details, "auto", &value))
			return json_object_to_json_string_ext(value, JSON_C_TO_STRING_PLAIN);
	}

	return "";
}

/**
 * Returns the boolean @c autoSupersedesDefault value for port data instantiated
 * from this data type.
 */
bool VuoCompilerInputDataClass::getAutoSupersedesDefaultValue(void)
{
	if (details)
	{
		json_object *value = NULL;
		if (json_object_object_get_ex(details, "autoSupersedesDefault", &value))
			return json_object_get_boolean(value);
	}

	return false;
}

/**
 * Helper for `VuoCompilerInputDataClass::setUnloweredStructPointerInEventFunction` and friends.
 */
void VuoCompilerInputDataClass::setUnloweredStructPointer(Type *firstParameterType, bool &unloweredStructPointer)
{
	StructType *elementType = nullptr;
	if (VuoCompilerCodeGenUtilities::isPointerToStruct(firstParameterType, &elementType))
	{
		// Check if the LLVM type name looks like e.g. "struct.VuoRange" or "struct.VuoRange.123" (hopefully faster than regex).
		vector<string> parts = VuoStringUtilities::split(elementType->getStructName().str(), '.');
		unloweredStructPointer = (parts.size() >= 2
								  && parts[0] == "struct"
								  && parts[1] == getVuoType()->getModuleKey()
								  && std::all_of(parts.begin() + 2, parts.end(), [](const string &s) { return s.find_first_not_of("0123456789") == string::npos; }));
	}
}

/**
 * Stores whether @a parameterType — this data's first (and possibly only) parameter in the node event function
 * — is an unlowered struct passed by reference.
 *
 * The reason for storing this when a node class's module is first loaded, instead of checking it as needed, is
 * to work around an apparent bug in LLVM. When the module is linked into a composition (i.e., a clone of it is
 * passed to `llvm::Linker::linkInModule()`), the struct type names in the module's function parameters are
 * changed from the original (e.g. `struct.VuoRange`) to something different (e.g. `%"type 0x7f8ef07bce80"*`).
 * https://b33p.net/kosada/vuo/vuo/-/issues/18132
 */
void VuoCompilerInputDataClass::setUnloweredStructPointerInEventFunction(Type *firstParameterType)
{
	setUnloweredStructPointer(firstParameterType, unloweredStructPointerInEventFunction);
}

/**
 * @see VuoCompilerInputDataClass::setUnloweredStructPointerInEventFunction.
 */
bool VuoCompilerInputDataClass::isUnloweredStructPointerInEventFunction(void)
{
	return unloweredStructPointerInEventFunction;
}

/**
 * @see VuoCompilerInputDataClass::setUnloweredStructPointerInEventFunction.
 */
void VuoCompilerInputDataClass::setUnloweredStructPointerInInitFunction(Type *firstParameterType)
{
	setUnloweredStructPointer(firstParameterType, unloweredStructPointerInInitFunction);
}

/**
 * @see VuoCompilerInputDataClass::setUnloweredStructPointerInEventFunction.
 */
bool VuoCompilerInputDataClass::isUnloweredStructPointerInInitFunction(void)
{
	return unloweredStructPointerInInitFunction;
}

/**
 * @see VuoCompilerInputDataClass::setUnloweredStructPointerInEventFunction.
 */
void VuoCompilerInputDataClass::setUnloweredStructPointerInCallbackStartFunction(Type *firstParameterType)
{
	setUnloweredStructPointer(firstParameterType, unloweredStructPointerInCallbackStartFunction);
}

/**
 * @see VuoCompilerInputDataClass::setUnloweredStructPointerInEventFunction.
 */
bool VuoCompilerInputDataClass::isUnloweredStructPointerInCallbackStartFunction(void)
{
	return unloweredStructPointerInCallbackStartFunction;
}

/**
 * @see VuoCompilerInputDataClass::setUnloweredStructPointerInEventFunction.
 */
void VuoCompilerInputDataClass::setUnloweredStructPointerInCallbackUpdateFunction(Type *firstParameterType)
{
	setUnloweredStructPointer(firstParameterType, unloweredStructPointerInCallbackUpdateFunction);
}

/**
 * @see VuoCompilerInputDataClass::setUnloweredStructPointerInEventFunction.
 */
bool VuoCompilerInputDataClass::isUnloweredStructPointerInCallbackUpdateFunction(void)
{
	return unloweredStructPointerInCallbackUpdateFunction;
}

/**
 * @see VuoCompilerInputDataClass::setUnloweredStructPointerInEventFunction.
 */
void VuoCompilerInputDataClass::setUnloweredStructPointerInCallbackStopFunction(Type *firstParameterType)
{
	setUnloweredStructPointer(firstParameterType, unloweredStructPointerInCallbackStopFunction);
}

/**
 * @see VuoCompilerInputDataClass::setUnloweredStructPointerInEventFunction.
 */
bool VuoCompilerInputDataClass::isUnloweredStructPointerInCallbackStopFunction(void)
{
	return unloweredStructPointerInCallbackStopFunction;
}

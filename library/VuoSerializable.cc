/**
 * @file
 * VuoSerializable implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSerializable.hh"
#include "type.h"

#include <typeinfo>
#include <cxxabi.h>

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Serializable Object",
					 "description" : "Base class for serializable objects.",
				 });
#endif
/// @}

VuoSerializable::ConstructorMap *VuoSerializable::constructors;

std::string VuoSerializable::type = "";	///< The subtype's class name.

/**
 * Informs this base class of the presence of a subclass, so it can be instantiated from JSON.
 *
 * Instead of calling this directly, use @ref VuoSerializableRegister.
 */
bool VuoSerializable::registerSubclass(std::string _type, Constructor makeFromJson)
{
	static dispatch_once_t init = 0;
	dispatch_once(&init, ^{
		VuoSerializable::constructors = new VuoSerializable::ConstructorMap;
	});
	(*constructors)[_type] = makeFromJson;
	return true;
}

/**
 * Creates an instance of a VuoSerializable subclass from JSON.
 */
VuoSerializable *VuoSerializable::makeFromJson(json_object *js)
{
	json_object *o = NULL;
	if (!json_object_object_get_ex(js, "type", &o))
		return NULL;

	std::string typeName = json_object_get_string(o);
	VuoSerializable::ConstructorMap::iterator i = VuoSerializable::constructors->find(typeName);
	if (i != VuoSerializable::constructors->end())
		return i->second(js);

	return NULL;
}

/**
 * Serializes this instance's data.
 */
json_object *VuoSerializable::getJson()
{
	json_object *json = json_object_new_object();
	json_object_object_add(json, "type", json_object_new_string(getType().c_str()));
	return json;
}

/**
 * Returns this instance's specialized type.
 *
 * Can't simply access the `type` member, since C++'s static dereferencing means it will always return
 * the local variable's type, instead of the instance's actual specialized type.
 */
std::string VuoSerializable::getType() const
{
	int status;
	char *unmangled = abi::__cxa_demangle(typeid(*this).name(), 0, 0, &status);
	if (status == 0)
	{
		std::string unmangledS(unmangled);
		free(unmangled);
		return unmangledS;
	}
	return "";
}

/**
 * Registers an instance of a VuoSerializable subclass with VuoHeap.
 */
VuoSerializable::VuoSerializable()
{
	VuoRegister(this, VuoSerializable::destroy);
}

/**
 * Does nothing.
 */
VuoSerializable::~VuoSerializable()
{
}

/**
 * Deletes an instance of a VuoSerializable subclass.
 *
 * To be called only by VuoHeap.
 */
void VuoSerializable::destroy(void *t)
{
	delete (VuoSerializable *)t;
}

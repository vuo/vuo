/**
 * @file
 * VuoSerializable class definition.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include <string>
#include <map>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include "json-c/json.h"
#pragma clang diagnostic pop

/**
 * Base class for VuoTypes implemented as private polymorphic class sets.
 *
 * @see VuoUiThemeBase and VuoUiButtonGroup
 */
class VuoSerializable
{
public:
	typedef VuoSerializable * (*Constructor)(json_object *);	///< A makeFromJson method.
	static bool registerSubclass(std::string type, Constructor makeFromJson);
	static void destroy(void *t);
	static std::string type;
	static VuoSerializable *makeFromJson(json_object *js);
	virtual json_object *getJson();
	std::string getType() const;
	virtual char *getSummary() = 0;	///< Outputs a summary of this instance's data, to be shown in port popovers.
	virtual bool operator==(const VuoSerializable &that) = 0;	///< Returns true if this instance is equivalent to that instance.
	virtual bool operator<(const VuoSerializable &that) = 0;	///< Returns true if this instance sorts before that instance.
	VuoSerializable();
	virtual ~VuoSerializable();
private:
	typedef std::map<std::string, Constructor> ConstructorMap;
	static ConstructorMap *constructors;
};

/**
 * Informs the VuoSerializable base class of the presence of a subclass, so it can be instantiated from JSON.
 *
 * @hideinitializer
 */
#define VuoSerializableRegister(class) \
	std::string class::type = #class; \
	static bool class ## Registered = VuoSerializable::registerSubclass(class::type, class::makeFromJson);

/**
 * Creates and initializes local variable `thatSpecialized`.
 *
 * To be used in each subclass's `operator==()` implementation.
 *
 * @hideinitializer
 */
#define VuoSerializableEquals(class) \
	const class *thatSpecialized = dynamic_cast<const class *>(&that); \
	if (!thatSpecialized) \
		return false;

/**
 * Creates and initializes local variable `thatSpecialized`.
 *
 * To be used in each subclass's `operator<()` implementation.
 *
 * @hideinitializer
 */
#define VuoSerializableLessThan(class) \
	const class *thatSpecialized = dynamic_cast<const class *>(&that); \
	if (!thatSpecialized) \
		return getType() < that.getType();

/**
 * @file
 * Private VuoUiThemeGroup implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoUiThemeBase.hh"

#include "VuoUiTheme.h"
#include "VuoList_VuoUiTheme.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "UI Theme Group",
				 });
#endif
/// @}

/**
 * A collection of multiple themes (ideally one theme for each type of widget).
 */
class VuoUiThemeGroup : public VuoUiThemeBase
{
private:
	VuoList_VuoUiTheme elements;

public:
	static std::string type; ///< The subtype's class name.

	/**
	 * Creates a theme group from JSON.
	 */
	static VuoSerializable *makeFromJson(json_object *js)
	{
		return new VuoUiThemeGroup(VuoJson_getObjectValue(VuoList_VuoUiTheme, js, "elements", NULL));
	}

	/**
	 * Creates a theme group containing `_elements`.
	 */
	VuoUiThemeGroup(VuoList_VuoUiTheme _elements)
	{
		elements = _elements;
		VuoRetain(elements);
	}
	~VuoUiThemeGroup()
	{
		VuoRelease(elements);
	}

	/**
	 * Encodes the theme group as a JSON object.
	 */
	json_object *getJson()
	{
		json_object *json = VuoSerializable::getJson();
		json_object_object_add(json, "elements", VuoList_VuoUiTheme_getJson(elements));
		return json;
	}

	/**
	 * Returns a compact string representation of the theme group.
	 */
	char *getSummary()
	{
		return strdup("Widget Theme Group");
	}

	/**
	 * Returns true if both are theme groups, and if all their elements are equal.
	 */
	bool operator==(const VuoSerializable &that)
	{
		VuoSerializableEquals(VuoUiThemeGroup);
		return VuoList_VuoUiTheme_areEqual(elements, thatSpecialized->elements);
	}

	/**
	 * Returns true if this theme group sorts before `that` theme group.
	 */
	bool operator<(const VuoSerializable &that)
	{
		VuoSerializableLessThan(VuoUiThemeGroup);
		VuoType_returnInequality(VuoList_VuoUiTheme, elements, thatSpecialized->elements);
		return false;
	}

	/**
	 * Finds the first item in `elements` whose type begins with `typeToFind`.
	 */
	VuoUiThemeBase *findElement(std::string typeToFind)
	{
		size_t typeToFindLength = typeToFind.length();
		unsigned long elementCount = VuoListGetCount_VuoUiTheme(elements);
		for (unsigned long i = 1; i <= elementCount; ++i)
		{
			VuoUiTheme item = VuoListGetValue_VuoUiTheme(elements, i);
			VuoUiThemeBase *base = (VuoUiThemeBase *)item;
			if (base && base->getType().compare(0, typeToFindLength, typeToFind) == 0)
				return base;
		}

		return NULL;
	}
};

VuoSerializableRegister(VuoUiThemeGroup);	///< Register with base class

/**
 * Creates a theme group containing `elements`.
 */
VuoUiTheme VuoUiTheme_makeGroup(VuoList_VuoUiTheme elements)
{
	return reinterpret_cast<VuoUiTheme>(new VuoUiThemeGroup(elements));
}

/**
 * Returns a specialized subclass of `type`.
 *
 * - If `theme`'s type begins with `type`, returns `theme`.
 * - If `theme` is a group, returns the first item in the group whose type begins with `type`.
 * - Otherwise creates a new Rounded instance of `type`.
 *
 * For this class set, "begins with" is defined to be a synonym for "is a subclass of".
 */
VuoUiThemeBase *VuoUiTheme_getSpecificTheme(VuoUiTheme theme, std::string type)
{
	if (theme)
	{
		VuoUiThemeBase *base = (VuoUiThemeBase *)theme;

		if (base->getType().compare(0, type.length(), type) == 0)
			return base;

		VuoUiThemeGroup *group = dynamic_cast<VuoUiThemeGroup *>(base);
		if (group)
		{
			VuoUiThemeBase *found = group->findElement(type);
			if (found)
				return found;
		}
	}

	// `theme` is neither the specified type nor a group that contains the specified type, make a new instance with default settings.
	json_object *js = json_object_new_object();
	std::string roundedType = type + "Rounded";
	json_object_object_add(js, "type", json_object_new_string(roundedType.c_str()));
	VuoUiThemeBase *rounded = (VuoUiThemeBase *)VuoUiTheme_makeFromJson(js);
	json_object_put(js);
	return rounded;
}

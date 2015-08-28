/**
 * @file
 * VuoList implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#error "This module is a template; do not build it."

extern "C" {
#include "VuoHeap.h"
#include "VuoText.h"
#include "ELEMENT_TYPE.h"
#include "LIST_TYPE.h"
}
#include "type.h"
#include <string>
#include <sstream>
#include <vector>

//@{
/**
 * Ignore calls to VuoRetain and VuoRelease if the element type is not reference-counted.
 */
#if IS_ELEMENT_REFERENCE_COUNTED == 0
#define RETAIN(element)
#define RELEASE(element)
#elif IS_ELEMENT_REFERENCE_COUNTED == 1
#define RETAIN(element) VuoRetain((void *)element)
#define RELEASE(element) VuoRelease((void *)element)
#elif IS_ELEMENT_REFERENCE_COUNTED == 2
#define RETAIN(element) ELEMENT_TYPE_retain(element)
#define RELEASE(element) ELEMENT_TYPE_release(element)
#endif
//@}

extern "C" {
/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "List of ELEMENT_TYPE elements",
					 "description" : "",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "c",
						 "json"
					 ]
				 });
#endif
/// @}
}

/**
 * Destroys the list (a LIST_TYPE).
 */
void VuoListDestroy_ELEMENT_TYPE(void *list);


LIST_TYPE LIST_TYPE_valueFromJson(json_object * js)
{
	LIST_TYPE list = VuoListCreate_ELEMENT_TYPE();

	if (json_object_get_type(js) == json_type_array)
	{
		int itemCount = json_object_array_length(js);
		for (int i = 0; i < itemCount; ++i)
		{
			json_object *itemObject = json_object_array_get_idx(js, i);
			ELEMENT_TYPE item = ELEMENT_TYPE_valueFromJson(itemObject);
			VuoListAppendValue_ELEMENT_TYPE(list, item);
		}
	}

	return list;
}

json_object * LIST_TYPE_jsonFromValue(const LIST_TYPE value)
{
	json_object *listObject = json_object_new_array();

	unsigned long itemCount = VuoListGetCount_ELEMENT_TYPE(value);
	for (unsigned long i = 1; i <= itemCount; ++i)
	{
		ELEMENT_TYPE item = VuoListGetValueAtIndex_ELEMENT_TYPE(value, i);
		json_object *itemObject = ELEMENT_TYPE_jsonFromValue(item);
		json_object_array_add(listObject, itemObject);
	}

	return listObject;
}

char * LIST_TYPE_summaryFromValue(const LIST_TYPE value)
{
	if (!value)
		return strdup("(empty list)");

	const int maxItems = 20;
	const int maxCharacters = 400;

	unsigned long itemCount = VuoListGetCount_ELEMENT_TYPE(value);
	if (itemCount == 0)
		return strdup("(empty list)");

	unsigned long characterCount = 0;

	std::ostringstream summary;
	summary << "List containing " << itemCount << " item" << (itemCount == 1 ? "" : "s") << ": <ul>";
	for (unsigned long i = 1; i <= itemCount && i <= maxItems && characterCount <= maxCharacters; ++i)
	{
		ELEMENT_TYPE item = VuoListGetValueAtIndex_ELEMENT_TYPE(value, i);
		char *itemSummary = ELEMENT_TYPE_summaryFromValue(item);
		if (strlen(itemSummary))
			summary << "<li>" << itemSummary << "</li>";
		else
			summary << "<li>&nbsp;</li>";
		characterCount += strlen(itemSummary);
		free(itemSummary);
	}

	if (itemCount > maxItems || characterCount > maxCharacters)
		summary << "<li>…</li>";

	summary << "</ul>";

	return strdup(summary.str().c_str());
}

LIST_TYPE VuoListCreate_ELEMENT_TYPE(void)
{
	std::vector<ELEMENT_TYPE> * l = new std::vector<ELEMENT_TYPE>;
	VuoRegister(l, VuoListDestroy_ELEMENT_TYPE);
	return l;
}

void VuoListDestroy_ELEMENT_TYPE(void *list)
{
	VuoListRemoveAll_ELEMENT_TYPE(list);

	std::vector<ELEMENT_TYPE> * l = (std::vector<ELEMENT_TYPE> *)list;
	delete l;
}

ELEMENT_TYPE VuoListGetValueAtIndex_ELEMENT_TYPE(const LIST_TYPE list, const unsigned long index)
{
	std::vector<ELEMENT_TYPE> * l = (std::vector<ELEMENT_TYPE> *)list;

	if (l->size() == 0)
		return ELEMENT_TYPE_valueFromJson(NULL);

	if (index == 0)
		return (*l)[0];

	if (index > l->size())
		return (*l)[l->size()-1];

	return (*l)[index-1];
}

void VuoListSetValueAtIndex_ELEMENT_TYPE(const LIST_TYPE list, const ELEMENT_TYPE value, const unsigned long index)
{
	std::vector<ELEMENT_TYPE> * l = (std::vector<ELEMENT_TYPE> *)list;

	if (l->size() == 0)
		return;

	unsigned long clampedIndex = index - 1;

	if (index == 0)
		clampedIndex = 0;

	if (index > l->size())
		clampedIndex = l->size() - 1;

	ELEMENT_TYPE oldValue = (*l)[clampedIndex];
	(*l)[clampedIndex] = value;
	RETAIN(value);
	RELEASE(oldValue);
}

void VuoListAppendValue_ELEMENT_TYPE(LIST_TYPE list, const ELEMENT_TYPE value)
{
	std::vector<ELEMENT_TYPE> * l = (std::vector<ELEMENT_TYPE> *)list;
	RETAIN(value);
	l->push_back(value);
}

void VuoListRemoveFirstValue_ELEMENT_TYPE(LIST_TYPE list)
{
	std::vector<ELEMENT_TYPE> * l = (std::vector<ELEMENT_TYPE> *)list;
	RELEASE(l->front());
	l->erase(l->begin());
}

void VuoListRemoveLastValue_ELEMENT_TYPE(LIST_TYPE list)
{
	std::vector<ELEMENT_TYPE> * l = (std::vector<ELEMENT_TYPE> *)list;
	RELEASE(l->back());
	l->pop_back();
}

void VuoListRemoveAll_ELEMENT_TYPE(LIST_TYPE list)
{
	while (VuoListGetCount_ELEMENT_TYPE(list) > 0)
		VuoListRemoveLastValue_ELEMENT_TYPE(list);
}

unsigned long VuoListGetCount_ELEMENT_TYPE(const LIST_TYPE list)
{
	std::vector<ELEMENT_TYPE> * l = (std::vector<ELEMENT_TYPE> *)list;
	return l->size();
}

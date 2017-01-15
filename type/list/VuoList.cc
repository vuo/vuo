/**
 * @file
 * VuoList implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#error "This module is a template; do not build it."

extern "C" {
#include "VuoHeap.h"
#include "VuoInteger.h"
#include "VuoText.h"
#include "ELEMENT_TYPE.h"
#include "LIST_TYPE.h"
}
#include "type.h"
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

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
						"VuoInteger"
					 ]
				 });
#endif
/// @}
}

/**
 * Destroys the list (a LIST_TYPE).
 */
void VuoListDestroy_ELEMENT_TYPE(void *list);


LIST_TYPE LIST_TYPE_makeFromJson(json_object * js)
{
	LIST_TYPE list = VuoListCreate_ELEMENT_TYPE();

	if (json_object_get_type(js) == json_type_array)
	{
		int itemCount = json_object_array_length(js);
		for (int i = 0; i < itemCount; ++i)
		{
			json_object *itemObject = json_object_array_get_idx(js, i);
			ELEMENT_TYPE item = ELEMENT_TYPE_makeFromJson(itemObject);
			VuoListAppendValue_ELEMENT_TYPE(list, item);
		}
	}

	return list;
}

json_object * LIST_TYPE_getJson(const LIST_TYPE value)
{
	json_object *listObject = json_object_new_array();

	unsigned long itemCount = VuoListGetCount_ELEMENT_TYPE(value);
	for (unsigned long i = 1; i <= itemCount; ++i)
	{
		ELEMENT_TYPE item = VuoListGetValue_ELEMENT_TYPE(value, i);
		json_object *itemObject = ELEMENT_TYPE_getJson(item);
		json_object_array_add(listObject, itemObject);
	}

	return listObject;
}

char * LIST_TYPE_getSummary(const LIST_TYPE value)
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
		ELEMENT_TYPE item = VuoListGetValue_ELEMENT_TYPE(value, i);
		std::string itemSummary = ELEMENT_TYPE_getSummary(item);
		if (itemSummary.length() && itemSummary.find_first_not_of(' ') != std::string::npos)
			summary << "<li>" << itemSummary << "</li>";
		else
			summary << "<li>&nbsp;</li>";
		characterCount += itemSummary.length();
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
	return reinterpret_cast<LIST_TYPE>(l);
}

LIST_TYPE VuoListCreateWithCount_ELEMENT_TYPE(const unsigned long count, const ELEMENT_TYPE value)
{
	std::vector<ELEMENT_TYPE> * l = new std::vector<ELEMENT_TYPE>(count);
	VuoRegister(l, VuoListDestroy_ELEMENT_TYPE);
	return reinterpret_cast<LIST_TYPE>(l);
}

LIST_TYPE VuoListCopy_ELEMENT_TYPE(const LIST_TYPE list)
{
	if (!list)
		return NULL;

	std::vector<ELEMENT_TYPE> *oldList = (std::vector<ELEMENT_TYPE> *)list;

	std::vector<ELEMENT_TYPE> *newList = new std::vector<ELEMENT_TYPE>(*oldList);
	VuoRegister(newList, VuoListDestroy_ELEMENT_TYPE);

	for (std::vector<ELEMENT_TYPE>::iterator i = newList->begin(); i != newList->end(); ++i)
		RETAIN(*i);

	return reinterpret_cast<LIST_TYPE>(newList);
}

void VuoListDestroy_ELEMENT_TYPE(void *list)
{
	if (!list)
		return;

#if IS_ELEMENT_REFERENCE_COUNTED != 0
	VuoListRemoveAll_ELEMENT_TYPE(reinterpret_cast<LIST_TYPE>(list));
#endif

	std::vector<ELEMENT_TYPE> * l = (std::vector<ELEMENT_TYPE> *)list;
	delete l;
}

ELEMENT_TYPE VuoListGetValue_ELEMENT_TYPE(const LIST_TYPE list, const unsigned long index)
{
	std::vector<ELEMENT_TYPE> * l = (std::vector<ELEMENT_TYPE> *)list;

	if (!l || l->size() == 0)
		return ELEMENT_TYPE_makeFromJson(NULL);

	if (index == 0)
		return (*l)[0];

	if (index > l->size())
		return (*l)[l->size()-1];

	return (*l)[index-1];
}

ELEMENT_TYPE *VuoListGetData_ELEMENT_TYPE(const LIST_TYPE list)
{
	std::vector<ELEMENT_TYPE> * l = (std::vector<ELEMENT_TYPE> *)list;

	if (!l || l->size() == 0)
		return NULL;

	return &((*l)[0]);
}

void VuoListSetValue_ELEMENT_TYPE(const LIST_TYPE list, const ELEMENT_TYPE value, const unsigned long index)
{
	if (!list)
		return;

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

void VuoListInsertValue_ELEMENT_TYPE(const LIST_TYPE list, const ELEMENT_TYPE value, const unsigned long index)
{
	if (!list)
		return;

	std::vector<ELEMENT_TYPE> * l = (std::vector<ELEMENT_TYPE> *)list;

	unsigned long clampedIndex = index - 1;

	if (index == 0)
		clampedIndex = 0;

	if (index > l->size())
		clampedIndex = l->size() - 1;

	RETAIN(value);
	if (index <= l->size())
		l->insert(l->begin() + clampedIndex, value);
	else
		l->push_back(value);
}

void VuoListPrependValue_ELEMENT_TYPE(LIST_TYPE list, const ELEMENT_TYPE value)
{
	if (!list)
		return;

	std::vector<ELEMENT_TYPE> * l = (std::vector<ELEMENT_TYPE> *)list;
	RETAIN(value);
	l->insert(l->begin(), value);
}

void VuoListAppendValue_ELEMENT_TYPE(LIST_TYPE list, const ELEMENT_TYPE value)
{
	if (!list)
		return;

	std::vector<ELEMENT_TYPE> * l = (std::vector<ELEMENT_TYPE> *)list;
	RETAIN(value);
	l->push_back(value);
}

void VuoListExchangeValues_ELEMENT_TYPE(LIST_TYPE list, const unsigned long indexA, const unsigned long indexB)
{
	if (!list)
		return;

	std::vector<ELEMENT_TYPE> * l = (std::vector<ELEMENT_TYPE> *)list;

	size_t size = l->size();
	if (size == 0)
		return;

	unsigned long clampedIndexA = indexA - 1;
	if (indexA == 0)
		clampedIndexA = 0;
	if (indexA > size)
		clampedIndexA = size - 1;

	unsigned long clampedIndexB = indexB - 1;
	if (indexB == 0)
		clampedIndexB = 0;
	if (indexB > size)
		clampedIndexB = size - 1;

	ELEMENT_TYPE value = (*l)[clampedIndexA];
	(*l)[clampedIndexA] = (*l)[clampedIndexB];
	(*l)[clampedIndexB] = value;
}

#ifdef ELEMENT_TYPE_SUPPORTS_COMPARISON
void VuoListSort_ELEMENT_TYPE(LIST_TYPE list)
{
	if (!list)
		return;

	std::vector<ELEMENT_TYPE> * l = (std::vector<ELEMENT_TYPE> *)list;

	size_t size = l->size();
	if (size < 2)
		return;

	std::sort(l->begin(), l->end(), ELEMENT_TYPE_isLessThan);
}

bool LIST_TYPE_areEqual(const LIST_TYPE _a, const LIST_TYPE _b)
{
	if (!_a || !_b)
		return _a == _b;

	std::vector<ELEMENT_TYPE> *a = (std::vector<ELEMENT_TYPE> *)_a;
	std::vector<ELEMENT_TYPE> *b = (std::vector<ELEMENT_TYPE> *)_b;
	if (a->size() != b->size())
		return false;

	for (std::vector<ELEMENT_TYPE>::iterator ia = a->begin(), ib = b->begin(); ia != a->end(); ++ia, ++ib)
		if (!ELEMENT_TYPE_areEqual(*ia, *ib))
			return false;

	return true;
}

bool LIST_TYPE_isLessThan(const LIST_TYPE _a, const LIST_TYPE _b)
{
	if (!_a || !_b)
		return _a < _b;

	std::vector<ELEMENT_TYPE> *a = (std::vector<ELEMENT_TYPE> *)_a;
	std::vector<ELEMENT_TYPE> *b = (std::vector<ELEMENT_TYPE> *)_b;
	if (a->size() < b->size()) return true;
	if (a->size() > b->size()) return false;

	for (std::vector<ELEMENT_TYPE>::iterator ia = a->begin(), ib = b->begin(); ia != a->end(); ++ia, ++ib)
	{
		if (ELEMENT_TYPE_isLessThan(*ia, *ib)) return true;
		if (ELEMENT_TYPE_isLessThan(*ib, *ia)) return false;
	}

	return false;
}
#endif

void VuoListShuffle_ELEMENT_TYPE(LIST_TYPE list, const double chaos)
{
	if (!list)
		return;

	std::vector<ELEMENT_TYPE> * l = (std::vector<ELEMENT_TYPE> *)list;

	size_t size = l->size();
	if (size == 0)
		return;

	double clampedChaos = MIN(MAX(chaos,0),1);
	for (unsigned long i = 0; i < size * clampedChaos; ++i)
	{
		unsigned long j = VuoInteger_random(i, size-1);
		ELEMENT_TYPE value = (*l)[i];
		(*l)[i] = (*l)[j];
		(*l)[j] = value;
	}
}

void VuoListReverse_ELEMENT_TYPE(LIST_TYPE list)
{
	if (!list)
		return;

	std::vector<ELEMENT_TYPE> * l = (std::vector<ELEMENT_TYPE> *)list;
	std::reverse(l->begin(), l->end());
}

void VuoListCut_ELEMENT_TYPE(LIST_TYPE list, const signed long startIndex, const unsigned long itemCount)
{
	if (!list)
		return;

	std::vector<ELEMENT_TYPE> * l = (std::vector<ELEMENT_TYPE> *)list;

	size_t size = l->size();
	if (size == 0)
		return;

	signed long clampedStartIndex = startIndex - 1;
	signed long clampedEndIndex = clampedStartIndex + itemCount - 1;

	if (clampedStartIndex < 0)
		clampedStartIndex = 0;
	if (clampedEndIndex >= (signed long)size)
		clampedEndIndex = size - 1;

	if (clampedStartIndex > clampedEndIndex)
	{
		VuoListRemoveAll_ELEMENT_TYPE(list);
		return;
	}

	while (clampedStartIndex--)
		VuoListRemoveFirstValue_ELEMENT_TYPE(list);

	while (clampedEndIndex++ < (signed long)size-1)
		VuoListRemoveLastValue_ELEMENT_TYPE(list);
}

void VuoListRemoveFirstValue_ELEMENT_TYPE(LIST_TYPE list)
{
	if (!list)
		return;

	std::vector<ELEMENT_TYPE> * l = (std::vector<ELEMENT_TYPE> *)list;

	if (!l->size())
		return;

	RELEASE(l->front());
	l->erase(l->begin());
}

void VuoListRemoveLastValue_ELEMENT_TYPE(LIST_TYPE list)
{
	if (!list)
		return;

	std::vector<ELEMENT_TYPE> * l = (std::vector<ELEMENT_TYPE> *)list;

	if (!l->size())
		return;

	RELEASE(l->back());
	l->pop_back();
}

void VuoListRemoveAll_ELEMENT_TYPE(LIST_TYPE list)
{
	if (!list)
		return;

	while (VuoListGetCount_ELEMENT_TYPE(list) > 0)
		VuoListRemoveLastValue_ELEMENT_TYPE(list);
}

void VuoListRemoveValue_ELEMENT_TYPE(LIST_TYPE list, const unsigned long index)
{
	if (!list)
		return;

	if (index == 0)
		return;

	std::vector<ELEMENT_TYPE> * l = (std::vector<ELEMENT_TYPE> *)list;

	size_t size = l->size();
	if (size == 0 || index > size)
		return;

	l->erase(l->begin() + index - 1);
}

unsigned long VuoListGetCount_ELEMENT_TYPE(const LIST_TYPE list)
{
	if (!list)
		return 0;

	std::vector<ELEMENT_TYPE> * l = (std::vector<ELEMENT_TYPE> *)list;
	return l->size();
}

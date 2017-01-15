/**
 * @file
 * VuoList_ExampleLanguage implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "VuoHeap.h"
#include "VuoInteger.h"
#include "VuoText.h"
#include "ExampleLanguage.h"
#include "VuoList_ExampleLanguage.h"
}
#include "type.h"
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

extern "C" {
/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "List of ExampleLanguage elements",
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
 * Destroys the list (a VuoList_ExampleLanguage).
 */
void VuoListDestroy_ExampleLanguage(void *list);


VuoList_ExampleLanguage VuoList_ExampleLanguage_makeFromJson(json_object * js)
{
	VuoList_ExampleLanguage list = VuoListCreate_ExampleLanguage();

	if (json_object_get_type(js) == json_type_array)
	{
		int itemCount = json_object_array_length(js);
		for (int i = 0; i < itemCount; ++i)
		{
			json_object *itemObject = json_object_array_get_idx(js, i);
			ExampleLanguage item = ExampleLanguage_makeFromJson(itemObject);
			VuoListAppendValue_ExampleLanguage(list, item);
		}
	}

	return list;
}

json_object * VuoList_ExampleLanguage_getJson(const VuoList_ExampleLanguage value)
{
	json_object *listObject = json_object_new_array();

	unsigned long itemCount = VuoListGetCount_ExampleLanguage(value);
	for (unsigned long i = 1; i <= itemCount; ++i)
	{
		ExampleLanguage item = VuoListGetValue_ExampleLanguage(value, i);
		json_object *itemObject = ExampleLanguage_getJson(item);
		json_object_array_add(listObject, itemObject);
	}

	return listObject;
}

char * VuoList_ExampleLanguage_getSummary(const VuoList_ExampleLanguage value)
{
	if (!value)
		return strdup("(empty list)");

	const int maxItems = 20;
	const int maxCharacters = 400;

	unsigned long itemCount = VuoListGetCount_ExampleLanguage(value);
	if (itemCount == 0)
		return strdup("(empty list)");

	unsigned long characterCount = 0;

	std::ostringstream summary;
	summary << "List containing " << itemCount << " item" << (itemCount == 1 ? "" : "s") << ": <ul>";
	for (unsigned long i = 1; i <= itemCount && i <= maxItems && characterCount <= maxCharacters; ++i)
	{
		ExampleLanguage item = VuoListGetValue_ExampleLanguage(value, i);
		char *itemSummary = ExampleLanguage_getSummary(item);
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

VuoList_ExampleLanguage VuoListCreate_ExampleLanguage(void)
{
	std::vector<ExampleLanguage> * l = new std::vector<ExampleLanguage>;
	VuoRegister(l, VuoListDestroy_ExampleLanguage);
	return reinterpret_cast<VuoList_ExampleLanguage>(l);
}

VuoList_ExampleLanguage VuoListCopy_ExampleLanguage(const VuoList_ExampleLanguage list)
{
	std::vector<ExampleLanguage> *oldList = (std::vector<ExampleLanguage> *)list;

	std::vector<ExampleLanguage> *newList = new std::vector<ExampleLanguage>(*oldList);
	VuoRegister(newList, VuoListDestroy_ExampleLanguage);

	return reinterpret_cast<VuoList_ExampleLanguage>(newList);
}

void VuoListDestroy_ExampleLanguage(void *list)
{
	VuoListRemoveAll_ExampleLanguage(reinterpret_cast<VuoList_ExampleLanguage>(list));

	std::vector<ExampleLanguage> * l = (std::vector<ExampleLanguage> *)list;
	delete l;
}

ExampleLanguage VuoListGetValue_ExampleLanguage(const VuoList_ExampleLanguage list, const unsigned long index)
{
	std::vector<ExampleLanguage> * l = (std::vector<ExampleLanguage> *)list;

	if (l->size() == 0)
		return ExampleLanguage_makeFromJson(NULL);

	if (index == 0)
		return (*l)[0];

	if (index > l->size())
		return (*l)[l->size()-1];

	return (*l)[index-1];
}

void VuoListSetValue_ExampleLanguage(const VuoList_ExampleLanguage list, const ExampleLanguage value, const unsigned long index)
{
	std::vector<ExampleLanguage> * l = (std::vector<ExampleLanguage> *)list;

	if (l->size() == 0)
		return;

	unsigned long clampedIndex = index - 1;

	if (index == 0)
		clampedIndex = 0;

	if (index > l->size())
		clampedIndex = l->size() - 1;

	ExampleLanguage oldValue = (*l)[clampedIndex];
	(*l)[clampedIndex] = value;
}

void VuoListInsertValue_ExampleLanguage(const VuoList_ExampleLanguage list, const ExampleLanguage value, const unsigned long index)
{
	std::vector<ExampleLanguage> * l = (std::vector<ExampleLanguage> *)list;

	unsigned long clampedIndex = index - 1;

	if (index == 0)
		clampedIndex = 0;

	if (index > l->size())
		clampedIndex = l->size() - 1;

	if (index <= l->size())
		l->insert(l->begin() + clampedIndex, value);
	else
		l->push_back(value);
}

void VuoListPrependValue_ExampleLanguage(VuoList_ExampleLanguage list, const ExampleLanguage value)
{
	std::vector<ExampleLanguage> * l = (std::vector<ExampleLanguage> *)list;
	l->insert(l->begin(), value);
}

void VuoListAppendValue_ExampleLanguage(VuoList_ExampleLanguage list, const ExampleLanguage value)
{
	std::vector<ExampleLanguage> * l = (std::vector<ExampleLanguage> *)list;
	l->push_back(value);
}

void VuoListExchangeValues_ExampleLanguage(VuoList_ExampleLanguage list, const unsigned long indexA, const unsigned long indexB)
{
	std::vector<ExampleLanguage> * l = (std::vector<ExampleLanguage> *)list;

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

	ExampleLanguage value = (*l)[clampedIndexA];
	(*l)[clampedIndexA] = (*l)[clampedIndexB];
	(*l)[clampedIndexB] = value;
}

void VuoListShuffle_ExampleLanguage(VuoList_ExampleLanguage list, const double chaos)
{
	std::vector<ExampleLanguage> * l = (std::vector<ExampleLanguage> *)list;

	size_t size = l->size();
	if (size == 0)
		return;

	double clampedChaos = MIN(MAX(chaos,0),1);
	for (unsigned long i = 0; i < size * clampedChaos; ++i)
	{
		unsigned long j = VuoInteger_random(i, size-1);
		ExampleLanguage value = (*l)[i];
		(*l)[i] = (*l)[j];
		(*l)[j] = value;
	}
}

void VuoListReverse_ExampleLanguage(VuoList_ExampleLanguage list)
{
	std::vector<ExampleLanguage> * l = (std::vector<ExampleLanguage> *)list;
	std::reverse(l->begin(), l->end());
}

void VuoListCut_ExampleLanguage(VuoList_ExampleLanguage list, const signed long startIndex, const unsigned long itemCount)
{
	std::vector<ExampleLanguage> * l = (std::vector<ExampleLanguage> *)list;

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
		VuoListRemoveAll_ExampleLanguage(list);
		return;
	}

	while (clampedStartIndex--)
		VuoListRemoveFirstValue_ExampleLanguage(list);

	while (clampedEndIndex++ < (signed long)size-1)
		VuoListRemoveLastValue_ExampleLanguage(list);
}

void VuoListRemoveFirstValue_ExampleLanguage(VuoList_ExampleLanguage list)
{
	std::vector<ExampleLanguage> * l = (std::vector<ExampleLanguage> *)list;

	if (!l->size())
		return;

	l->erase(l->begin());
}

void VuoListRemoveLastValue_ExampleLanguage(VuoList_ExampleLanguage list)
{
	std::vector<ExampleLanguage> * l = (std::vector<ExampleLanguage> *)list;

	if (!l->size())
		return;

	l->pop_back();
}

void VuoListRemoveAll_ExampleLanguage(VuoList_ExampleLanguage list)
{
	while (VuoListGetCount_ExampleLanguage(list) > 0)
		VuoListRemoveLastValue_ExampleLanguage(list);
}

void VuoListRemoveValue_ExampleLanguage(VuoList_ExampleLanguage list, const unsigned long index)
{
	if (index == 0)
		return;

	std::vector<ExampleLanguage> * l = (std::vector<ExampleLanguage> *)list;

	size_t size = l->size();
	if (size == 0 || index > size)
		return;

	l->erase(l->begin() + index - 1);
}

unsigned long VuoListGetCount_ExampleLanguage(const VuoList_ExampleLanguage list)
{
	std::vector<ExampleLanguage> * l = (std::vector<ExampleLanguage> *)list;
	return l->size();
}

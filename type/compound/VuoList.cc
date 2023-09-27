/**
 * @file
 * VuoList implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <algorithm>
#include <string>
#include <sstream>
#include <vector>
#include "VuoInteger.h"
#include "VuoText.h"

extern VuoList_VuoGenericType1 VuoListCreate_VuoGenericType1(void);
extern VuoGenericType1 VuoListGetValue_VuoGenericType1(const VuoList_VuoGenericType1 list, const unsigned long index);
extern void VuoListAppendValue_VuoGenericType1(VuoList_VuoGenericType1 list, const VuoGenericType1 value);
extern void VuoListRemoveAll_VuoGenericType1(VuoList_VuoGenericType1 list);
extern unsigned long VuoListGetCount_VuoGenericType1(const VuoList_VuoGenericType1 list);
extern VuoGenericType1 VuoGenericType1_makeFromJson(struct json_object *js);
extern struct json_object * VuoGenericType1_getJson(const VuoGenericType1 value);
#ifdef VuoGenericType1_OVERRIDES_INTERPROCESS_SERIALIZATION
extern struct json_object * VuoGenericType1_getInterprocessJson(const VuoGenericType1 value);
#endif
extern char * VuoGenericType1_getSummary(const VuoGenericType1 value);
extern void VuoGenericType1_retain(VuoGenericType1 value);
extern void VuoGenericType1_release(VuoGenericType1 value);
#ifdef VuoGenericType1_SUPPORTS_COMPARISON
extern bool VuoGenericType1_isLessThan(const VuoGenericType1 a, const VuoGenericType1 b);
extern bool VuoGenericType1_areEqual(const VuoGenericType1 a, const VuoGenericType1 b);
#endif

extern "C" {
/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoGenericType1 List",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoInteger"
					 ],
					 "genericTypes" : {
						"VuoGenericType1" : {}
					 }
				 });
#endif
/// @}
}

/**
 * Destroys the list (a `VuoList_VuoGenericType1`).
 */
void VuoListDestroy_VuoGenericType1(void *list);


VuoList_VuoGenericType1 VuoList_VuoGenericType1_makeFromJson(json_object *js)
{
	if (!js)
		return nullptr;

	VuoList_VuoGenericType1 list = VuoListCreate_VuoGenericType1();

	if (json_object_get_type(js) == json_type_array)
	{
		int itemCount = json_object_array_length(js);
		for (int i = 0; i < itemCount; ++i)
		{
			json_object *itemObject = json_object_array_get_idx(js, i);
			VuoGenericType1 item = VuoGenericType1_makeFromJson(itemObject);
			VuoListAppendValue_VuoGenericType1(list, item);
		}
	}

	return list;
}

json_object * VuoList_VuoGenericType1_getJson(const VuoList_VuoGenericType1 value)
{
	if (!value)
		return nullptr;

	json_object *listObject = json_object_new_array();

	unsigned long itemCount = VuoListGetCount_VuoGenericType1(value);
	for (unsigned long i = 1; i <= itemCount; ++i)
	{
		VuoGenericType1 item = VuoListGetValue_VuoGenericType1(value, i);
		json_object *itemObject = VuoGenericType1_getJson(item);
		json_object_array_add(listObject, itemObject);
	}

	return listObject;
}

#ifdef VuoGenericType1_OVERRIDES_INTERPROCESS_SERIALIZATION
struct json_object * VuoList_VuoGenericType1_getInterprocessJson(const VuoList_VuoGenericType1 value)
{
	json_object *listObject = json_object_new_array();

	unsigned long itemCount = VuoListGetCount_VuoGenericType1(value);
	for (unsigned long i = 1; i <= itemCount; ++i)
	{
		VuoGenericType1 item = VuoListGetValue_VuoGenericType1(value, i);
		json_object *itemObject = VuoGenericType1_getInterprocessJson(item);
		json_object_array_add(listObject, itemObject);
	}

	return listObject;
}
#endif

char * VuoList_VuoGenericType1_getSummary(const VuoList_VuoGenericType1 value)
{
	const int maxItems = 20;
	const int maxCharacters = 400;

	unsigned long itemCount = VuoListGetCount_VuoGenericType1(value);
	if (itemCount == 0)
		return strdup("Empty list");

	unsigned long characterCount = 0;

	std::ostringstream summary;
	summary << "List containing " << itemCount << " item" << (itemCount == 1 ? "" : "s") << ": <ul>";
	unsigned long i;
	for (i = 1; i <= itemCount && i <= maxItems && characterCount <= maxCharacters; ++i)
	{
		VuoGenericType1 item = VuoListGetValue_VuoGenericType1(value, i);
		char *itemSummaryCstr = VuoGenericType1_getSummary(item);
		std::string itemSummary = itemSummaryCstr;
		free(itemSummaryCstr);
		if (itemSummary.length() && itemSummary.find_first_not_of(' ') != std::string::npos)
			summary << "\n<li>" << itemSummary << "</li>";
		else
			summary << "\n<li>&nbsp;</li>";
		characterCount += itemSummary.length();
	}

	if (i <= itemCount)
		summary << "\n<li>…</li>";

	summary << "</ul>";

	return strdup(summary.str().c_str());
}

VuoList_VuoGenericType1 VuoListCreate_VuoGenericType1(void)
{
	std::vector<VuoGenericType1> * l = new std::vector<VuoGenericType1>;
	VuoRegister(l, VuoListDestroy_VuoGenericType1);
	return reinterpret_cast<VuoList_VuoGenericType1>(l);
}

VuoList_VuoGenericType1 VuoListCreateWithCount_VuoGenericType1(const unsigned long count, const VuoGenericType1 value)
{
	std::vector<VuoGenericType1> * l = new std::vector<VuoGenericType1>(count, value);
	VuoRegister(l, VuoListDestroy_VuoGenericType1);

	for (unsigned long i = 0; i < count; ++i)
		VuoGenericType1_retain(value);

	return reinterpret_cast<VuoList_VuoGenericType1>(l);
}

VuoList_VuoGenericType1 VuoListCreateWithValueArray_VuoGenericType1(const VuoGenericType1 *values, const unsigned long valueCount)
{
	std::vector<VuoGenericType1> *l = new std::vector<VuoGenericType1>(values, values + valueCount);
	VuoRegister(l, VuoListDestroy_VuoGenericType1);

	for (unsigned long i = 0; i < valueCount; ++i)
		VuoGenericType1_retain(values[i]);

	return reinterpret_cast<VuoList_VuoGenericType1>(l);
}

VuoList_VuoGenericType1 VuoListCopy_VuoGenericType1(const VuoList_VuoGenericType1 list)
{
	if (!list)
		return NULL;

	std::vector<VuoGenericType1> *oldList = (std::vector<VuoGenericType1> *)list;

	std::vector<VuoGenericType1> *newList = new std::vector<VuoGenericType1>(*oldList);
	VuoRegister(newList, VuoListDestroy_VuoGenericType1);

	for (std::vector<VuoGenericType1>::iterator i = newList->begin(); i != newList->end(); ++i)
		VuoGenericType1_retain(*i);

	return reinterpret_cast<VuoList_VuoGenericType1>(newList);
}

void VuoListDestroy_VuoGenericType1(void *list)
{
	if (!list)
		return;

	VuoListRemoveAll_VuoGenericType1(reinterpret_cast<VuoList_VuoGenericType1>(list));

	std::vector<VuoGenericType1> * l = (std::vector<VuoGenericType1> *)list;
	delete l;
}

VuoGenericType1 VuoListGetValue_VuoGenericType1(const VuoList_VuoGenericType1 list, const unsigned long index)
{
	std::vector<VuoGenericType1> * l = (std::vector<VuoGenericType1> *)list;

	if (!l || l->size() == 0)
		return VuoGenericType1_makeFromJson(NULL);

	if (index == 0)
		return (*l)[0];

	if (index > l->size())
		return (*l)[l->size()-1];

	return (*l)[index-1];
}

VuoGenericType1 * VuoListGetData_VuoGenericType1(const VuoList_VuoGenericType1 list)
{
	std::vector<VuoGenericType1> * l = (std::vector<VuoGenericType1> *)list;

	if (!l || l->size() == 0)
		return NULL;

	return &((*l)[0]);
}

void VuoListForeach_VuoGenericType1(const VuoList_VuoGenericType1 list, bool (^function)(const VuoGenericType1 value))
{
	auto l = reinterpret_cast<const std::vector<VuoGenericType1> *>(list);

	if (!l || l->size() == 0)
		return;

	for (auto item : *l)
		if (!function(item))
			break;
}

void VuoListSetValue_VuoGenericType1(const VuoList_VuoGenericType1 list, const VuoGenericType1 value, const unsigned long index, bool expandListIfNeeded)
{
	if (!list)
		return;

	std::vector<VuoGenericType1> * l = (std::vector<VuoGenericType1> *)list;

	if (!expandListIfNeeded && l->size() == 0)
		return;

	unsigned long clampedIndex = index - 1;

	if (index == 0)
		clampedIndex = 0;

	if (expandListIfNeeded && clampedIndex >= l->size())
	{
		l->resize(clampedIndex + 1);
		(*l)[clampedIndex] = value;
		VuoGenericType1_retain(value);
	}
	else
	{
		if (index > l->size())
			clampedIndex = l->size() - 1;

		VuoGenericType1 oldValue = (*l)[clampedIndex];
		(*l)[clampedIndex] = value;
		VuoGenericType1_retain(value);
		VuoGenericType1_release(oldValue);
	}
}

void VuoListInsertValue_VuoGenericType1(const VuoList_VuoGenericType1 list, const VuoGenericType1 value, const unsigned long index)
{
	if (!list)
		return;

	std::vector<VuoGenericType1> * l = (std::vector<VuoGenericType1> *)list;

	unsigned long clampedIndex = index - 1;

	if (index == 0)
		clampedIndex = 0;

	if (index > l->size())
		clampedIndex = l->size() - 1;

	VuoGenericType1_retain(value);
	if (index <= l->size())
		l->insert(l->begin() + clampedIndex, value);
	else
		l->push_back(value);
}

void VuoListPrependValue_VuoGenericType1(VuoList_VuoGenericType1 list, const VuoGenericType1 value)
{
	if (!list)
		return;

	std::vector<VuoGenericType1> * l = (std::vector<VuoGenericType1> *)list;
	VuoGenericType1_retain(value);
	l->insert(l->begin(), value);
}

void VuoListAppendValue_VuoGenericType1(VuoList_VuoGenericType1 list, const VuoGenericType1 value)
{
	if (!list)
		return;

	std::vector<VuoGenericType1> * l = (std::vector<VuoGenericType1> *)list;
	VuoGenericType1_retain(value);
	l->push_back(value);
}

void VuoListExchangeValues_VuoGenericType1(VuoList_VuoGenericType1 list, const unsigned long indexA, const unsigned long indexB)
{
	if (!list)
		return;

	std::vector<VuoGenericType1> * l = (std::vector<VuoGenericType1> *)list;

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

	VuoGenericType1 value = (*l)[clampedIndexA];
	(*l)[clampedIndexA] = (*l)[clampedIndexB];
	(*l)[clampedIndexB] = value;
}

#ifdef VuoGenericType1_SUPPORTS_COMPARISON
void VuoListSort_VuoGenericType1(VuoList_VuoGenericType1 list)
{
	if (!list)
		return;

	std::vector<VuoGenericType1> * l = (std::vector<VuoGenericType1> *)list;

	size_t size = l->size();
	if (size < 2)
		return;

	std::sort(l->begin(), l->end(), VuoGenericType1_isLessThan);
}

bool VuoList_VuoGenericType1_areEqual(const VuoList_VuoGenericType1 _a, const VuoList_VuoGenericType1 _b)
{
	if (_a == _b)
		return true;

	if (!_a || !_b)
		return false;

	std::vector<VuoGenericType1> *a = (std::vector<VuoGenericType1> *)_a;
	std::vector<VuoGenericType1> *b = (std::vector<VuoGenericType1> *)_b;
	if (a->size() != b->size())
		return false;

	for (std::vector<VuoGenericType1>::iterator ia = a->begin(), ib = b->begin(); ia != a->end(); ++ia, ++ib)
		if (!VuoGenericType1_areEqual(*ia, *ib))
			return false;

	return true;
}

bool VuoList_VuoGenericType1_isLessThan(const VuoList_VuoGenericType1 _a, const VuoList_VuoGenericType1 _b)
{
	if (!_a || !_b)
		return _a < _b;

	std::vector<VuoGenericType1> *a = (std::vector<VuoGenericType1> *)_a;
	std::vector<VuoGenericType1> *b = (std::vector<VuoGenericType1> *)_b;
	if (a->size() < b->size()) return true;
	if (a->size() > b->size()) return false;

	for (std::vector<VuoGenericType1>::iterator ia = a->begin(), ib = b->begin(); ia != a->end(); ++ia, ++ib)
	{
		if (VuoGenericType1_isLessThan(*ia, *ib)) return true;
		if (VuoGenericType1_isLessThan(*ib, *ia)) return false;
	}

	return false;
}
#endif

void VuoListShuffle_VuoGenericType1(VuoList_VuoGenericType1 list, const double chaos)
{
	if (!list)
		return;

	std::vector<VuoGenericType1> * l = (std::vector<VuoGenericType1> *)list;

	size_t size = l->size();
	if (size == 0)
		return;

	double clampedChaos = MIN(MAX(chaos,0),1);
	for (unsigned long i = 0; i < size * clampedChaos; ++i)
	{
		unsigned long j = VuoInteger_random(i, size-1);
		VuoGenericType1 value = (*l)[i];
		(*l)[i] = (*l)[j];
		(*l)[j] = value;
	}
}

void VuoListReverse_VuoGenericType1(VuoList_VuoGenericType1 list)
{
	if (!list)
		return;

	std::vector<VuoGenericType1> * l = (std::vector<VuoGenericType1> *)list;
	std::reverse(l->begin(), l->end());
}

VuoList_VuoGenericType1 VuoListSubset_VuoGenericType1(VuoList_VuoGenericType1 list, const signed long startIndex, const unsigned long itemCount)
{
	if (!list)
		return NULL;

	std::vector<VuoGenericType1> * l = (std::vector<VuoGenericType1> *)list;

	size_t size = l->size();
	if (size == 0)
		return NULL;

	signed long clampedStartIndex = startIndex - 1;
	signed long clampedEndIndex = clampedStartIndex + itemCount - 1;

	if (clampedStartIndex < 0)
		clampedStartIndex = 0;
	if (clampedEndIndex >= (signed long)size)
		clampedEndIndex = size - 1;

	if (clampedStartIndex > clampedEndIndex)
		return NULL;

	std::vector<VuoGenericType1> *newList = new std::vector<VuoGenericType1>(
				l->begin() + clampedStartIndex,
				l->begin() + clampedEndIndex + 1);
	VuoRegister(newList, VuoListDestroy_VuoGenericType1);

	for (std::vector<VuoGenericType1>::iterator i = newList->begin(); i != newList->end(); ++i)
		VuoGenericType1_retain(*i);

	return reinterpret_cast<VuoList_VuoGenericType1>(newList);
}

#ifdef VuoGenericType1_SUPPORTS_COMPARISON
VuoList_VuoGenericType1 VuoListRemoveDuplicates_VuoGenericType1(VuoList_VuoGenericType1 list)
{
	if (!list)
		return NULL;

	auto *l = (std::vector<VuoGenericType1> *)list;

	size_t size = l->size();
	if (size == 0)
		return NULL;

	auto *newList = new std::vector<VuoGenericType1>;
	VuoRegister(newList, VuoListDestroy_VuoGenericType1);

	for (auto i = l->begin(); i != l->end(); ++i)
	{
		bool found = false;
		for (auto j = newList->begin(); j != newList->end(); ++j)
			if (VuoGenericType1_areEqual(*i, *j))
			{
				found = true;
				break;
			}
		if (!found)
			newList->push_back(*i);
	}

	for (auto i = newList->begin(); i != newList->end(); ++i)
		VuoGenericType1_retain(*i);

	return reinterpret_cast<VuoList_VuoGenericType1>(newList);
}
#endif

void VuoListRemoveFirstValue_VuoGenericType1(VuoList_VuoGenericType1 list)
{
	if (!list)
		return;

	std::vector<VuoGenericType1> * l = (std::vector<VuoGenericType1> *)list;

	if (!l->size())
		return;

	VuoGenericType1_release(l->front());
	l->erase(l->begin());
}

void VuoListRemoveLastValue_VuoGenericType1(VuoList_VuoGenericType1 list)
{
	if (!list)
		return;

	std::vector<VuoGenericType1> * l = (std::vector<VuoGenericType1> *)list;

	if (!l->size())
		return;

	VuoGenericType1_release(l->back());
	l->pop_back();
}

void VuoListRemoveAll_VuoGenericType1(VuoList_VuoGenericType1 list)
{
	if (!list)
		return;

	while (VuoListGetCount_VuoGenericType1(list) > 0)
		VuoListRemoveLastValue_VuoGenericType1(list);
}

void VuoListRemoveValue_VuoGenericType1(VuoList_VuoGenericType1 list, const unsigned long index)
{
	if (!list)
		return;

	if (index == 0)
		return;

	std::vector<VuoGenericType1> * l = (std::vector<VuoGenericType1> *)list;

	size_t size = l->size();
	if (size == 0 || index > size)
		return;

	l->erase(l->begin() + index - 1);
}

unsigned long VuoListGetCount_VuoGenericType1(const VuoList_VuoGenericType1 list)
{
	if (!list)
		return 0;

	std::vector<VuoGenericType1> * l = (std::vector<VuoGenericType1> *)list;
	return l->size();
}

void VuoList_VuoGenericType1_retain(VuoList_VuoGenericType1 value)
{
	VuoRetain(value);
}

void VuoList_VuoGenericType1_release(VuoList_VuoGenericType1 value)
{
	VuoRelease(value);
}

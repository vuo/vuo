/**
 * @file
 * VuoTable C type definition.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#include "VuoListPosition.h"
#include "VuoTableFormat.h"
#include "VuoTextSort.h"
#include "VuoSortOrder.h"
#include "VuoList_VuoText.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoTable VuoTable
 * Information structured in rows and columns.
 *
 * @{
 */

/**
 * Information structured in rows and columns.
 */
typedef struct
{
	void *data;  ///< The data items (a `vector< vector<VuoText> >` with outer vectors as rows and inner vectors as columns), including headers.
	size_t rowCount;  ///< The number of rows. Same as `(*data).size()`.
	size_t columnCount;  ///< The number of columns. The maximum of `(*data)[i].size()`.
} VuoTable;

VuoTable VuoTable_makeFromJson(struct json_object *js);
struct json_object * VuoTable_getJson(const VuoTable value);
struct json_object * VuoTable_getInterprocessJson(const VuoTable value);
char * VuoTable_getSummary(const VuoTable value);

VuoTable VuoTable_makeEmpty(void);
VuoTable VuoTable_makeFromText(VuoText text, VuoTableFormat format);
VuoText VuoTable_serialize(VuoTable table, VuoTableFormat format);
VuoTable VuoTable_sort_VuoInteger(VuoTable table, VuoInteger columnIndex, VuoTextSort sortType, VuoSortOrder sortOrder, bool firstRowIsHeader);
VuoTable VuoTable_sort_VuoText(VuoTable table, VuoText columnHeader, VuoTextSort sortType, VuoSortOrder sortOrder, bool firstRowIsHeader);
VuoTable VuoTable_transpose(VuoTable table);
VuoList_VuoText VuoTable_getRow_VuoInteger(VuoTable table, VuoInteger rowIndex, bool includeHeader);
VuoList_VuoText VuoTable_getRow_VuoText(VuoTable table, VuoText rowHeader, bool includeHeader);
VuoList_VuoText VuoTable_getColumn_VuoInteger(VuoTable table, VuoInteger columnIndex, bool includeHeader);
VuoList_VuoText VuoTable_getColumn_VuoText(VuoTable table, VuoText columnHeader, bool includeHeader);
VuoText VuoTable_getItem_VuoInteger_VuoInteger(VuoTable table, VuoInteger rowIndex, VuoInteger columnIndex);
VuoText VuoTable_getItem_VuoInteger_VuoText(VuoTable table, VuoInteger rowIndex, VuoText columnHeader);
VuoText VuoTable_getItem_VuoText_VuoInteger(VuoTable table, VuoText rowHeader, VuoInteger columnIndex);
VuoText VuoTable_getItem_VuoText_VuoText(VuoTable table, VuoText rowHeader, VuoText columnHeader);
VuoTable VuoTable_addRow(VuoTable table, VuoListPosition position, VuoList_VuoText values);
VuoTable VuoTable_addColumn(VuoTable table, VuoListPosition position, VuoList_VuoText values);
VuoTable VuoTable_changeRow_VuoInteger(VuoTable table, VuoInteger rowIndex, VuoList_VuoText newValues, bool preserveHeader);
VuoTable VuoTable_changeRow_VuoText(VuoTable table, VuoText rowHeader, VuoList_VuoText newValues, bool preserveHeader);
VuoTable VuoTable_changeColumn_VuoInteger(VuoTable table, VuoInteger columnIndex, VuoList_VuoText newValues, bool preserveHeader);
VuoTable VuoTable_changeColumn_VuoText(VuoTable table, VuoText columnHeader, VuoList_VuoText newValues, bool preserveHeader);
VuoTable VuoTable_changeItem_VuoInteger_VuoInteger(VuoTable table, VuoInteger rowIndex, VuoInteger columnIndex, VuoText newValue);
VuoTable VuoTable_changeItem_VuoInteger_VuoText(VuoTable table, VuoInteger rowIndex, VuoText columnHeader, VuoText newValue);
VuoTable VuoTable_changeItem_VuoText_VuoInteger(VuoTable table, VuoText rowHeader, VuoInteger columnIndex, VuoText newValue);
VuoTable VuoTable_changeItem_VuoText_VuoText(VuoTable table, VuoText rowHeader, VuoText columnHeader, VuoText newValue);
VuoTable VuoTable_removeRow(VuoTable table, VuoListPosition position);
VuoTable VuoTable_removeColumn(VuoTable table, VuoListPosition position);

void VuoTable_retain(VuoTable value);
void VuoTable_release(VuoTable value);

///@{
/**
 * Automatically generated function.
 */
VuoTable VuoTable_makeFromString(const char *str);
char * VuoTable_getString(const VuoTable value);
char * VuoTable_getInterprocessString(const VuoTable value);
///@}

/**
 * @}
 */

/**
 * @file
 * VuoTree C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoTree_h
#define VuoTree_h

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoDictionary_VuoText_VuoText.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoTree VuoTree
 * Hierarchically structured information.
 *
 * @{
 */

/**
 * Hierarchically structured information.
 */
typedef struct
{
	void *rootXmlNode;  ///< XML representation of the tree — an `xmlNode *` of type element node.
	struct json_object *rootJson;  ///< JSON representation of the tree, if it was parsed from JSON.
	void *children;  ///< Children of the tree, if it was constructed without parsing from text — a `VuoList_VuoTree`.
} VuoTree;

#define VuoTree_OVERRIDES_INTERPROCESS_SERIALIZATION  ///< This type implements `_getInterprocessJson()`.
#include "VuoList_VuoTree.h"

VuoTree VuoTree_makeFromJson(struct json_object *js);
struct json_object * VuoTree_getJson(const VuoTree value);
struct json_object * VuoTree_getInterprocessJson(const VuoTree value);
char * VuoTree_getSummary(const VuoTree value);

VuoTree VuoTree_makeEmpty(void);
VuoTree VuoTree_make(VuoText name, VuoDictionary_VuoText_VuoText attributes, VuoText content, VuoList_VuoTree children);
VuoTree VuoTree_makeFromJsonText(VuoText json);
VuoTree VuoTree_makeFromXmlText(VuoText xml, bool includeWhitespace);
VuoText VuoTree_serializeAsXml(VuoTree tree, bool indent);
VuoText VuoTree_serializeAsJson(VuoTree tree, bool indent);
void VuoTree_retain(VuoTree value);
void VuoTree_release(VuoTree value);

VuoText VuoTree_getName(VuoTree tree);
VuoDictionary_VuoText_VuoText VuoTree_getAttributes(VuoTree tree);
VuoText VuoTree_getAttribute(VuoTree tree, VuoText attribute);
VuoText VuoTree_getContent(VuoTree tree, bool includeDescendants);
VuoList_VuoTree VuoTree_getChildren(VuoTree tree);
struct json_object * VuoTree_getContainedValue(VuoTree tree);
VuoList_VuoTree VuoTree_findItemsUsingXpath(VuoTree tree, VuoText xpath);
VuoList_VuoTree VuoTree_findItemsWithName(VuoTree tree, VuoText name, VuoTextComparison comparison, bool includeDescendants);
VuoList_VuoTree VuoTree_findItemsWithAttribute(VuoTree tree, VuoText attribute, VuoText value, VuoTextComparison valueComparison, bool includeDescendants);
VuoList_VuoTree VuoTree_findItemsWithContent(VuoTree tree, VuoText content, VuoTextComparison comparison, bool includeDescendants);

///@{
/**
 * Automatically generated function.
 */
char * VuoTree_getString(const VuoTree value);
char * VuoTree_getInterprocessString(const VuoTree value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

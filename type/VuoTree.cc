/**
 * @file
 * VuoTree implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <algorithm>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

extern "C"
{
#include "type.h"
#include "VuoTree.h"
#include "VuoList_VuoTree.h"

#include <ctype.h>  // isspace()
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Tree",
					  "description" : "Hierarchically structured information.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoList_VuoTree",
						  "VuoDictionary_VuoText_VuoText",
						  "VuoTextHtml"
					  ]
				  });
#endif
/// @}
}

/// The character encoding of XML text.
static const char *encoding = "UTF-8";

/**
 * Frees an `xmlDoc`.
 */
static void VuoTree_freeXmlDoc(void *value)
{
	xmlFreeDoc( (xmlDocPtr)value );
}

/**
 * Releases a `json_object`.
 */
static void VuoTree_freeJsonObject(void *value)
{
	json_object_put((json_object *)value);
}


/**
 * Returns a tree whose root is @a node.
 *
 * Use this function if the XML node belongs to an existing `VuoTree`.
 * It assumes the node's XML document has already been registered.
 */
static VuoTree VuoTree_makeFromXmlNode(xmlNode *node)
{
	// Don't register rootXmlNode, to avoid multiply registering it if it's the root of more than one tree.
	return (VuoTree){ node, NULL, NULL };
}

/**
 * Returns a tree whose root is the root node of @a doc.
 *
 * Use this function if this is the first `VuoTree` to refer to the XML document.
 * It assumes the XML document has not yet been registered.
 *
 * If the document lacks a root node, @a doc is freed and an empty tree is returned.
 */
static VuoTree VuoTree_makeFromXmlDoc(xmlDoc *doc)
{
	xmlNode *node = xmlDocGetRootElement(doc);
	if (! node)
	{
		xmlFreeDoc(doc);
		return VuoTree_makeEmpty();
	}

	VuoRegister(doc, VuoTree_freeXmlDoc);
	return VuoTree_makeFromXmlNode(node);
}

/**
 * Converts Apple's weird double-encoded Property List XML format
 * into a straightforward XML document.
 */
static xmlNodePtr VuoTree_boilDownPropertyList(xmlNodePtr plist)
{
	if (strcmp((const char *)plist->name, "dict") == 0)
	{
		xmlNodePtr n = xmlNewNode(NULL, (const xmlChar *)"");
		for (xmlNodePtr cur = plist->children; cur; cur = cur->next)
		{
			if (cur->type != XML_ELEMENT_NODE)
				continue;

			// A 'key' element followed by a value element.
			if (strcmp((const char *)cur->name, "key") != 0)
			{
				VUserLog("Error: Malformed property list XML: Expected a <key> element, but found <%s>.", cur->name);
				return NULL;
			}

			// Extract the key from the first text content.  (Faster than xmlNodeGetContent.)
			xmlChar *key = NULL;
			for (xmlNode *nn = cur->children; nn; nn = nn->next)
				if (nn->type == XML_TEXT_NODE)
				{
					key = nn->content;
					break;
				}


			cur = cur->next;
			if (!cur)
			{
				VUserLog("Error: Malformed property list XML: Expected a value element to follow the <key> element, but instead reached the end.");
				return NULL;
			}

			xmlNodePtr subNode = VuoTree_boilDownPropertyList(cur);
			subNode->name = xmlStrdup(key);
			xmlAddChild(n, subNode);
		}
		return n;
	}
	else if (strcmp((const char *)plist->name, "array") == 0)
	{
		xmlNodePtr n = xmlNewNode(NULL, (const xmlChar *)"");
		for (xmlNodePtr cur = plist->children; cur; cur = cur->next)
			xmlAddChild(n, VuoTree_boilDownPropertyList(cur));
		return n;
	}
	else if (strcmp((const char *)plist->name, "false") == 0)
	{
		xmlNodePtr n = xmlNewNode(NULL, (const xmlChar *)"");
		xmlAddChild(n, xmlNewText(xmlCharStrdup("false")));
		return n;
	}
	else if (strcmp((const char *)plist->name, "true") == 0)
	{
		xmlNodePtr n = xmlNewNode(NULL, (const xmlChar *)"");
		xmlAddChild(n, xmlNewText(xmlCharStrdup("true")));
		return n;
	}
	else
	{
		// Leave other types as-is (string, data, date, integer, real).
		// (Don't attempt to decode <data>'s base64, since raw binary data isn't valid XML or VuoText.
		// The composition author should use a separate node to decode it into VuoData.)
		xmlNodePtr n = xmlCopyNode(plist, true);
		n->name = xmlCharStrdup("item");
		return n;
	}
}

/**
 * Converts an XML string to a VuoTree.
 */
static VuoTree VuoTree_parseXml(const char *xmlString, bool includeWhitespace)
{
	if (! xmlString)
		return VuoTree_makeEmpty();

	size_t length = strlen(xmlString);
	if (length == 0)
		return VuoTree_makeEmpty();

	int flags = 0;
	if (! includeWhitespace)
		flags |= XML_PARSE_NOBLANKS;

	xmlDoc *doc = xmlReadMemory(xmlString, length, "VuoTree.xml", encoding, flags);
	if (! doc)
	{
		VUserLog("Error: Couldn't parse tree as XML");
		return VuoTree_makeEmpty();
	}


	// If the XML file is an Apple Property List, decode it.
	xmlNodePtr root = xmlDocGetRootElement(doc);
	if (doc->intSubset && doc->intSubset->SystemID
		&& strcmp((const char *)doc->intSubset->SystemID, "http://www.apple.com/DTDs/PropertyList-1.0.dtd") == 0)
	{
		xmlNodePtr transformedRoot = VuoTree_boilDownPropertyList(root->children);
		if (!transformedRoot)
			return VuoTree_makeEmpty();

		transformedRoot->name = xmlCharStrdup("document");

		xmlFreeDoc(doc);

		doc = xmlNewDoc((const xmlChar *)"1.0");
		xmlDocSetRootElement(doc, transformedRoot);
	}


	return VuoTree_makeFromXmlDoc(doc);
}

/**
 * Converts the tree's root XML node and children to an XML string.
 */
static xmlChar * VuoTree_serializeXmlNodeAsXml(VuoTree tree, bool indent, int level)
{
	if (! tree.rootXmlNode)
		return xmlStrdup((const xmlChar *)"");

	xmlNode *treeRoot = xmlCopyNode((xmlNode *)tree.rootXmlNode, 1);
	VuoDefer(^{ xmlUnlinkNode(treeRoot); xmlFreeNode(treeRoot); });

	list<xmlNode *> nodesToVisit;
	nodesToVisit.push_back(treeRoot);
	while (! nodesToVisit.empty())
	{
		xmlNode *currentNode = nodesToVisit.front();
		nodesToVisit.pop_front();

		if (xmlStrlen(currentNode->name) == 0)
		{
			const char *name = (currentNode == treeRoot && level == 0 ? "document" : "item");
			xmlNodeSetName(currentNode, (const xmlChar *)name);
		}

		for (xmlNode *n = currentNode->children; n; n = n->next)
			if (n->type == XML_ELEMENT_NODE)
				nodesToVisit.push_back(n);
	}

	xmlBuffer *buffer = xmlBufferCreate();
	VuoDefer(^{ xmlBufferFree(buffer); });

	int ret = xmlNodeDump(buffer, treeRoot->doc, treeRoot, level, indent);

	if (ret < 0)
	{
		VUserLog("Error: Couldn't serialize tree named '%s' as XML.", treeRoot->name);
		return xmlStrdup((const xmlChar *)"");
	}

	xmlChar *xml = xmlBufferDetach(buffer);

	if (tree.children)
	{
		xmlChar *closingTag;
		if (xml[xmlStrlen(xml)-2] == '/')
		{
			// Split <name ... /> into <name ...> and </name>
			closingTag = xmlStrdup((const xmlChar *)"</");
			closingTag = xmlStrcat(closingTag, treeRoot->name);
			closingTag = xmlStrcat(closingTag, (const xmlChar *)">");
			xml[xmlStrlen(xml)-2] = '>';
			xml[xmlStrlen(xml)-1] = 0;
		}
		else
		{
			// Split <name ...></name> into <name ...> and </name>
			int closingTagLength = xmlStrlen(treeRoot->name) + 3;
			int splitIndex = xmlStrlen(xml) - closingTagLength;
			closingTag = xmlStrsub(xml, splitIndex, closingTagLength);
			xml[splitIndex] = 0;
		}

		string whitespaceBeforeChild = (indent ? "\n" + string(2 * (level+1), ' ') : "");

		unsigned long childCount = VuoListGetCount_VuoTree(tree.children);
		for (unsigned long i = 1; i <= childCount; ++i)
		{
			xml = xmlStrcat(xml, (const xmlChar *)whitespaceBeforeChild.c_str());

			VuoTree child = VuoListGetValue_VuoTree(tree.children, i);
			xmlChar *childXml = VuoTree_serializeXmlNodeAsXml(child, indent, level+1);
			xml = xmlStrcat(xml, childXml);
		}

		string whitespaceBeforeClosingTag = (indent ? "\n" + string(2 * level, ' ') : "");
		xml = xmlStrcat(xml, (const xmlChar *)whitespaceBeforeClosingTag.c_str());

		xml = xmlStrcat(xml, closingTag);
	}

	return xml;
}

/**
 * Returns a new XML node with @a name as its name if it's a valid element name, otherwise @name as an attribute.
 */
static xmlNode * createXmlNode(const char *name)
{
	if (! name || strlen(name) == 0)
		return xmlNewNode(NULL, (const xmlChar *)"");

	string testElement = "<" + string(name) + "/>";
	xmlDoc *testDoc = xmlReadMemory(testElement.c_str(), testElement.length(), "VuoTree.xml", encoding, 0);
	if (testDoc)
	{
		// <name>...</name>
		xmlFreeDoc(testDoc);
		return xmlNewNode(NULL, (const xmlChar *)name);
	}
	else
	{
		// <item name="name">...</item>
		xmlNode *node = xmlNewNode(NULL, (const xmlChar *)"item");
		xmlNewProp(node, (const xmlChar *)"name", (const xmlChar *)name);
		return node;
	}
}

/**
 * Converts a JSON string to a VuoTree.
 */
static VuoTree VuoTree_parseJson(const char *jsonString)
{
	if (! jsonString || strlen(jsonString) == 0)
		return VuoTree_makeEmpty();

	enum json_tokener_error error;
	json_object *json = json_tokener_parse_verbose(jsonString, &error);
	if (! json)
	{
		VUserLog("Error: Couldn't parse tree as JSON: %s", json_tokener_error_desc(error));
		return VuoTree_makeEmpty();
	}

	bool hasRoot;
	if (json_object_is_type(json, json_type_object) && json_object_object_length(json) == 1)
	{
		hasRoot = true;
		json_object_object_foreach(json, key, value)
		{
			if (json_object_is_type(value, json_type_array))
			{
				hasRoot = false;
				break;
			}
		}
	}
	else
		hasRoot = false;

	json_object *renamedRootObject = NULL;
	json_object *renamedArrayObject = NULL;
	if (! hasRoot)
	{
		// The JSON doesn't have a root, so add one. The names are placeholders and will be changed to empty strings in the XML.
		// Change ... to {"json":...}
		// Change [...] to {"json":{"array":[...]}} — special case for arrays

		if (json_object_is_type(json, json_type_array))
		{
			json_object *arrayParent = json_object_new_object();
			json_object_object_add(arrayParent, "array", json);
			json = arrayParent;
			renamedArrayObject = arrayParent;
		}

		json_object *parent = json_object_new_object();
		json_object_object_add(parent, "json", json);
		json = parent;
		renamedRootObject = parent;
	}

	xmlDoc *doc = xmlNewDoc((const xmlChar *)"1.0");
	xmlNode *root = NULL;
	map<json_object *, xmlNode *> parents;

	list<json_object *> jsonsToVisit;
	jsonsToVisit.push_back(json);
	while (! jsonsToVisit.empty())
	{
		json_object *currentJson = jsonsToVisit.front();
		jsonsToVisit.pop_front();

		xmlNode *parentNode = parents[currentJson];

		// Translate {"key1":value1,"key2":value2,...} to <key1>value1</key1><key2>value2</key2>...
		// Translate {"key1":[value1,value2,...],...} to <key1>value1</key1><key1>value2</key1>... — special case for arrays

		json_type type = json_object_get_type(currentJson);
		if (type == json_type_object)
		{
			json_object_object_foreach(currentJson, key, value)
			{
				const char *nodeName = (currentJson == renamedRootObject || currentJson == renamedArrayObject ? "" : key);

				if (json_object_is_type(value, json_type_array))
				{
					int length = json_object_array_length(value);
					for (int i = 0; i < length; ++i)
					{
						json_object *arrayValue = json_object_array_get_idx(value, i);

						xmlNode *currentNode = createXmlNode(nodeName);
						xmlAddChild(parentNode, currentNode);

						parents[arrayValue] = currentNode;
						jsonsToVisit.push_back(arrayValue);
					}
				}
				else
				{
					xmlNode *currentNode = createXmlNode(nodeName);

					if (parentNode)
						xmlAddChild(parentNode, currentNode);
					else
						root = currentNode;

					parents[value] = currentNode;
					jsonsToVisit.push_back(value);
				}
			}
		}
		else
		{
			const char *content = json_object_get_string(currentJson);
			xmlNodeSetContent(parentNode, (const xmlChar *)content);
		}
	}

	xmlDocSetRootElement(doc, root);

	json_object_put(json);

	VuoTree tree = VuoTree_makeFromXmlDoc(doc);

	tree.rootJson = json_tokener_parse(jsonString);  // Use the original JSON, not the modified one from above.
	VuoRegister(tree.rootJson, VuoTree_freeJsonObject);

	return tree;
}

/**
 * Helper for `VuoTree_serializeXmlNodeAsJson()`.
 */
typedef struct
{
	xmlNode *node;
	const char *text;
} ChildInfo;

/**
 * Helper for `VuoTree_serializeXmlNodeAsJson()`. Creates a new JSON string from @a s minus its control characters.
 */
static json_object * createJsonString(const char *s)
{
	const int charCount = 5;
	const char *controlChars[charCount] = { "\b", "\f", "\n", "\r", "\t" };
	VuoText t[charCount+1];
	t[0] = s;
	for (int i = 0; i < charCount; ++i)
		t[i+1] = VuoText_replace(t[i], controlChars[i], "");

	json_object *ret = json_object_new_string(t[charCount]);

	for (int i = charCount; i > 0; --i)
	{
		if (t[i] != t[i-1])
		{
			VuoRetain(t[i]);
			VuoRelease(t[i]);
		}
	}

	return ret;
}

/**
 * Helper for `VuoTree_serializeXmlNodeAsJson()`. Adds @a value to @a container, which may be either an array or an object.
 */
static void addToContainer(json_object *container, const char *key, json_object *value)
{
	if (json_object_is_type(container, json_type_array))
		json_object_array_add(container, value);
	else
		json_object_object_add(container, key, value);
}

/**
 * Converts an `xmlNode` partway to a JSON-formatted string. (Not all the way, to avoid an extra copy.)
 *
 * Assumes @a node is an element node.
 */
static json_object * VuoTree_serializeXmlNodeAsJson(VuoTree tree, bool atRoot)
{
	xmlNode *rootNode = (xmlNode *)tree.rootXmlNode;

	json_object *topLevelJson = json_object_new_object();
	const char *topLevelName = (xmlStrlen(rootNode->name) > 0 ? (const char *)rootNode->name : (atRoot ? "document" : "item"));

	map<xmlNode *, json_object *> containers;
	containers[rootNode] = topLevelJson;

	map<xmlNode *, const char *> names;
	names[rootNode] = topLevelName;

	list<xmlNode *> nodesToVisit;
	nodesToVisit.push_back( (xmlNode *)tree.rootXmlNode );
	while (! nodesToVisit.empty())
	{
		xmlNode *currentNode = nodesToVisit.front();
		nodesToVisit.pop_front();
//		VLog("visit %s", names[currentNode]);
//		VLog("  %s", json_object_to_json_string(containers[currentNode]));

		// Make a list of currentNode's attributes and children, indexed by name.

		vector< pair<const char *, ChildInfo> > childInfos;
		int elementCount = 0;
		int textCount = 0;

		for (xmlAttr *attribute = currentNode->properties; attribute; attribute = attribute->next)
		{
			xmlChar *attributeValue = xmlNodeListGetString(rootNode->doc, attribute->children, 1);
			ChildInfo info = { NULL, (const char *)attributeValue };
			childInfos.push_back(make_pair( (const char *)attribute->name, info ));
		}

		for (xmlNode *childNode = currentNode->children; childNode; childNode = childNode->next)
		{
			if (childNode->type == XML_ELEMENT_NODE)
			{
				const char *childName = (xmlStrlen(childNode->name) > 0 ? (const char *)childNode->name : "item");
				names[childNode] = childName;

				ChildInfo info = { childNode, NULL };
				childInfos.push_back(make_pair( childName, info ));

				++elementCount;
			}
			else if (childNode->type == XML_TEXT_NODE || childNode->type == XML_CDATA_SECTION_NODE)
			{
				ChildInfo info = { childNode, (char *)childNode->content };
				childInfos.push_back(make_pair( (const char *)NULL, info ));

				++textCount;
			}
		}

		json_object *currentContainer = containers[currentNode];
		const char *currentName = names[currentNode];

		if (childInfos.empty())
		{
			// No attributes or children — {"currentName":null}

			addToContainer(currentContainer, currentName, NULL);
//			VLog("  %s", json_object_to_json_string(currentContainer));
		}
		else if (childInfos.size() == 1 && childInfos[0].first == NULL)
		{
			// Single text child — {"currentName":"text"}

			json_object *text = createJsonString(childInfos[0].second.text);
			addToContainer(currentContainer, currentName, text);
//			VLog("  %s", json_object_to_json_string(currentContainer));
		}
		else
		{
			// Consolidate the list of attributes and children, grouping by name.

			vector< pair<const char *, vector<ChildInfo> > > mergedChildInfo;
			map<size_t, bool> childInfosVisited;

			for (size_t i = 0; i < childInfos.size(); ++i)
			{
				if (childInfosVisited[i])
					continue;

				vector<ChildInfo> sameNameInfo;
				sameNameInfo.push_back(childInfos[i].second);
				childInfosVisited[i] = true;

				if (childInfos[i].first)
				{
					for (size_t j = i+1; j < childInfos.size(); ++j)
					{
						if (! childInfos[j].first)
							break;

						if (! strcmp(childInfos[i].first, childInfos[j].first))
						{
							sameNameInfo.push_back(childInfos[j].second);
							childInfosVisited[j] = true;
						}
					}
				}

				mergedChildInfo.push_back(make_pair( childInfos[i].first, sameNameInfo ));
//				VLog("    %s : %lu", childInfos[i].first, sameNameInfo.size());
			}

			// Multiple children — {"currentName":{...}} or {"currentName":[...]}

			bool isMixedContent = elementCount > 0 && textCount > 0;
			json_object *childContainer = (isMixedContent ? json_object_new_array() : json_object_new_object());
			addToContainer(currentContainer, currentName, childContainer);
//			VLog("  %s", json_object_to_json_string(currentContainer));

			for (vector< pair<const char *, vector<ChildInfo> > >::iterator i = mergedChildInfo.begin(); i != mergedChildInfo.end(); ++i)
			{
				if ((*i).second.size() == 1)
				{
					// Child with unique name — {"childName":"text"} or {"childName":{...}}

					if ((*i).second[0].text)
					{
						json_object *text = createJsonString((*i).second[0].text);
						const char *childName = ((*i).first ? (*i).first : "content");
						addToContainer(childContainer, childName, text);
					}
					else
					{
						json_object *singleChildContainer;
						if (isMixedContent)
						{
							singleChildContainer = json_object_new_object();
							json_object_array_add(childContainer, singleChildContainer);
						}
						else
							singleChildContainer = childContainer;

						containers[(*i).second[0].node] = singleChildContainer;
					}
				}
				else
				{
					// Children with same name — {"childName":[...]}

					json_object *sameNameArray = json_object_new_array();
					for (vector<ChildInfo>::iterator j = (*i).second.begin(); j != (*i).second.end(); ++j)
					{
						if ((*j).text)
						{
							json_object *text = createJsonString((*j).text);
							json_object_array_add(sameNameArray, text);
						}
						else
							containers[(*j).node] = sameNameArray;
					}

					json_object *sameNameArrayContainer;
					if (isMixedContent)
					{
						sameNameArrayContainer = json_object_new_object();
						json_object_array_add(childContainer, sameNameArrayContainer);
					}
					else
						sameNameArrayContainer = childContainer;

					const char *childName = ((*i).first ? (*i).first : "content");
					addToContainer(sameNameArrayContainer, childName, sameNameArray);
//					VLog("  %s", json_object_to_json_string(currentContainer));
				}
			}
		}

		for (xmlNode *childNode = currentNode->children; childNode; childNode = childNode->next)
			if (childNode->type == XML_ELEMENT_NODE)
				nodesToVisit.push_back(childNode);
	}

	if (tree.children)
	{
		const char *rootKey = NULL;
		json_object *rootValue = NULL;
		json_object_object_foreach(topLevelJson, key, value)
		{
			rootKey = key;
			rootValue = value;
			break;
		}

		if (! rootValue)
		{
			// Change {"rootKey":NULL} to {"rootKey":{}}
			rootValue = json_object_new_object();
			json_object_object_add(topLevelJson, rootKey, rootValue);
		}

		unsigned long childCount = VuoListGetCount_VuoTree(tree.children);
		for (unsigned long i = 1; i <= childCount; ++i)
		{
			VuoTree child = VuoListGetValue_VuoTree(tree.children, i);
			if (! child.rootXmlNode)
				continue;

			json_object *childJson = VuoTree_serializeXmlNodeAsJson(child, false);

			const char *childKey = NULL;
			json_object *childValue = NULL;
			json_object_object_foreach(childJson, key, value)
			{
				childKey = key;
				childValue = value;
				break;
			}

			json_object *siblingValue = NULL;
			if (json_object_object_get_ex(rootValue, childKey, &siblingValue))
			{
				if (! json_object_is_type(siblingValue, json_type_array))
				{
					// Change {"rootKey":{"childKey":...}} to {"rootKey":{"childKey":[...]}
					json_object *siblingsArray = json_object_new_array();
					json_object_get(siblingValue);
					json_object_array_add(siblingsArray, siblingValue);
					json_object_object_add(rootValue, childKey, siblingsArray);
					siblingValue = siblingsArray;
				}

				// Change {"rootKey":{"childKey":[...]} to {"rootKey":{"childKey":[...,childValue]}
				json_object_array_add(siblingValue, childValue);
			}
			else
			{
				// Change {"rootKey":{...}} to {"rootKey":{...,"childKey":childValue}}
				json_object_object_add(rootValue, childKey, childValue);
			}
		}
	}

	return topLevelJson;
}


/**
 * Decodes the JSON object @a js to create a new value.
 */
VuoTree VuoTree_makeFromJson(struct json_object *js)
{
	json_object *o;
	if (json_object_object_get_ex(js, "pointer", &o))
	{
		xmlNode *node = (xmlNode *)json_object_get_int64(o);
		VuoTree tree = VuoTree_makeFromXmlNode(node);

		if (json_object_object_get_ex(js, "jsonPointer", &o))
			tree.rootJson = (json_object *)json_object_get_int64(o);

		if (json_object_object_get_ex(js, "childrenPointer", &o))
			tree.children = (VuoList_VuoTree)json_object_get_int64(o);

		return tree;
	}
	else if (json_object_object_get_ex(js, "xml", &o))
	{
		const char *treeAsXml = json_object_get_string(o);
		return VuoTree_parseXml(treeAsXml, true);
	}
	else if (json_object_object_get_ex(js, "json", &o))
	{
		const char *treeAsJson = json_object_get_string(o);
		return VuoTree_parseJson(treeAsJson);
	}

	return VuoTree_makeEmpty();
}

/**
 * Encodes @a value as a JSON object that can only be decoded in the same process.
 */
struct json_object * VuoTree_getJson(const VuoTree value)
{
	json_object *js = json_object_new_object();

	json_object_object_add(js, "pointer", json_object_new_int64((int64_t)value.rootXmlNode));
	json_object_object_add(js, "jsonPointer", json_object_new_int64((int64_t)value.rootJson));
	json_object_object_add(js, "childrenPointer", json_object_new_int64((int64_t)value.children));

	return js;
}

/**
 * Encodes @a value as a JSON object that can be decoded in a separate process.
 */
struct json_object * VuoTree_getInterprocessJson(const VuoTree value)
{
	json_object *js = json_object_new_object();

	if (value.rootJson)
	{
		const char *treeAsJson = json_object_to_json_string(value.rootJson);
		json_object_object_add(js, "json", json_object_new_string(treeAsJson));
	}
	else
	{
		xmlChar *treeAsXml = VuoTree_serializeXmlNodeAsXml(value, false, 0);
		json_object_object_add(js, "xml", json_object_new_string((const char *)treeAsXml));
		xmlFree(treeAsXml);
	}

	return js;
}

/**
 * Returns a human-readable description of @a value.
 */
char * VuoTree_getSummary(const VuoTree value)
{
	ostringstream summary;

	VuoText name = VuoTree_getName(value);
	VuoLocal(name);
	if (! VuoText_isEmpty(name))
	{
		char *nameSummary = VuoText_getSummary(name);
		summary << "name: " << nameSummary << "<br>";
		free(nameSummary);
	}

	VuoDictionary_VuoText_VuoText attributes = VuoTree_getAttributes(value);
	unsigned long attributeCount = VuoListGetCount_VuoText(attributes.keys);
	if (attributeCount > 0)
	{
		summary << "attributes: <table>";
		unsigned long maxAttributes = 5;
		for (unsigned long i = 1; i <= attributeCount && i <= maxAttributes; ++i)
		{
			VuoText name = VuoListGetValue_VuoText(attributes.keys, i);
			VuoText value = VuoListGetValue_VuoText(attributes.values, i);
			char *nameSummary = VuoText_getSummary(name);
			char *valueSummary = VuoText_getSummary(value);
			summary << "<tr><td>" << nameSummary << "</td><td> → " << valueSummary << "</td></tr>";
			free(nameSummary);
			free(valueSummary);
		}
		if (attributeCount > maxAttributes)
			summary << "<tr><td colspan=\"2\">…</td></tr>";
		summary << "</table>";
	}
	VuoDictionary_VuoText_VuoText_retain(attributes);
	VuoDictionary_VuoText_VuoText_release(attributes);

	VuoText content = VuoTree_getContent(value, false);
	VuoLocal(content);
	if (! VuoText_isEmpty(content))
	{
		char *contentSummary = VuoText_getSummary(content);
		summary << "content: " << contentSummary << "<br>";
		free(contentSummary);
	}

	VuoList_VuoTree children = VuoTree_getChildren(value);
	VuoLocal(children);
	unsigned long childCount = VuoListGetCount_VuoTree(children);
	if (childCount > 0)
	{
		summary << childCount << " " << (childCount == 1 ? "child" : "children") << "<ul>";
		VuoTree *childrenArray = VuoListGetData_VuoTree(children);
		unsigned long maxChildren = 15;
		for (unsigned long i = 0; i < childCount && i < maxChildren; ++i)
		{
			VuoText name = VuoTree_getName(childrenArray[i]);
			char *nameSummary = VuoText_getSummary(name);
			if (strlen(nameSummary) > 0 && string(nameSummary).find_first_not_of(' ') != string::npos)
				summary << "<li>" << nameSummary << "</li>";
			else
				summary << "<li>&nbsp;</li>";
			free(nameSummary);
		}
		if (childCount > maxChildren)
			summary << "<li>…</li>";
		summary << "</ul>";
	}

	string summaryStr = summary.str();
	if (summaryStr.empty())
		summaryStr = "(empty tree)";

	return strdup(summaryStr.c_str());
}


/**
 * Returns a tree with nothing in it.
 */
VuoTree VuoTree_makeEmpty(void)
{
	return (VuoTree){ NULL, NULL, NULL };
}

/**
 * Returns a tree that consists of the given components.
 */
VuoTree VuoTree_make(VuoText name, VuoDictionary_VuoText_VuoText attributes, VuoText content, VuoList_VuoTree children)
{
	xmlDoc *doc = xmlNewDoc((const xmlChar *)"1.0");
	xmlNode *root = createXmlNode(name);
	xmlDocSetRootElement(doc, root);

	xmlNodeSetContent(root, (const xmlChar *)content);

	unsigned long attributeCount = VuoListGetCount_VuoText(attributes.keys);
	for (unsigned long i = 1; i <= attributeCount; ++i)
	{
		VuoText key = VuoListGetValue_VuoText(attributes.keys, i);
		VuoText value = VuoListGetValue_VuoText(attributes.values, i);
		xmlNewProp(root, (const xmlChar *)key, (const xmlChar *)value);
	}

	VuoTree tree = VuoTree_makeFromXmlDoc(doc);

	tree.children = children;

	return tree;
}

/**
 * Returns a tree parsed from @a json.
 */
VuoTree VuoTree_makeFromJsonText(VuoText json)
{
	return VuoTree_parseJson(json);
}

/**
 * Returns a tree parsed from @a xml.
 */
VuoTree VuoTree_makeFromXmlText(VuoText xml, bool includeWhitespace)
{
	return VuoTree_parseXml(xml, includeWhitespace);
}

/**
 * Returns an XML-formatted string representation of a tree.
 */
VuoText VuoTree_serializeAsXml(VuoTree tree, bool indent)
{
	xmlChar *xmlAsString = VuoTree_serializeXmlNodeAsXml(tree, indent, 0);
	VuoText xml = VuoText_make((const char *)xmlAsString);
	xmlFree(xmlAsString);
	return xml;
}

/**
 * Returns an JSON-formatted string representation of a tree.
 */
VuoText VuoTree_serializeAsJson(VuoTree tree, bool indent)
{
	if (! tree.rootXmlNode && ! tree.rootJson)
		return VuoText_make("");

	json_object *json = tree.rootJson;
	if (! json)
		json = VuoTree_serializeXmlNodeAsJson(tree, true);

	int flags = (indent ? JSON_C_TO_STRING_PRETTY : JSON_C_TO_STRING_PLAIN);
	const char *jsonAsString = json_object_to_json_string_ext(json, flags);
	VuoText jsonAsText = VuoText_make(jsonAsString);

	if (! tree.rootJson)
		json_object_put(json);

	return jsonAsText;
}

/**
 * Replaces the auto-generated function. Skips `rootXmlNode` since we don't register it.
 */
void VuoTree_retain(VuoTree value)
{
	if (value.rootXmlNode)
		VuoRetain(((xmlNode *)value.rootXmlNode)->doc);
	VuoRetain(value.rootJson);
	VuoRetain(value.children);
}

/**
 * Replaces the auto-generated function. Skips `rootXmlNode` since we don't register it.
 */
void VuoTree_release(VuoTree value)
{
	if (value.rootXmlNode)
		VuoRelease(((xmlNode *)value.rootXmlNode)->doc);
	VuoRelease(value.rootJson);
	VuoRelease(value.children);
}


/**
 * Returns the name of the tree's root item.
 */
VuoText VuoTree_getName(VuoTree tree)
{
	if (! tree.rootXmlNode)
		return VuoText_make("");

	const xmlChar *name = ((xmlNode *)tree.rootXmlNode)->name;
	return VuoText_make((const char *)name);
}

/**
 * Returns the attributes of the tree's root item.
 */
VuoDictionary_VuoText_VuoText VuoTree_getAttributes(VuoTree tree)
{
	VuoDictionary_VuoText_VuoText attributes = VuoDictionaryCreate_VuoText_VuoText();

	if (! tree.rootXmlNode)
		return attributes;

	for (xmlAttr *a = ((xmlNode *)tree.rootXmlNode)->properties; a; a = a->next)
	{
		const xmlChar *keyAsString = a->name;
		VuoText key = VuoText_make((const char *)keyAsString);

		xmlChar *valueAsString = xmlNodeListGetString(((xmlNode *)tree.rootXmlNode)->doc, a->children, 1);
		VuoText value = VuoText_make((const char *)valueAsString);
		xmlFree(valueAsString);

		VuoDictionarySetKeyValue_VuoText_VuoText(attributes, key, value);
	}

	return attributes;
}

/**
 * Returns the value of the attribute with the given name in the tree's root item,
 * or an empty text if not found.
 */
VuoText VuoTree_getAttribute(VuoTree tree, VuoText attribute)
{
	if (! tree.rootXmlNode)
		return VuoText_make("");

	xmlChar *value = xmlGetProp((xmlNode *)tree.rootXmlNode, (const xmlChar *)attribute);
	VuoText valueAsText = VuoText_make((const char *)value);
	xmlFree(value);
	return valueAsText;
}

/**
 * Returns the content of @a node, not including content of descendants.
 */
static xmlChar * VuoTree_getContentOfXmlNode(xmlNode *node)
{
	xmlChar *content = xmlStrdup((const xmlChar *)"");
	for (xmlNode *n = node->children; n; n = n->next)
		if (n->type == XML_TEXT_NODE || n->type == XML_CDATA_SECTION_NODE)
			content = xmlStrcat(content, n->content);

	return content;
}

/**
 * Returns the content of the tree's root item and, optionally, of its descendants.
 */
VuoText VuoTree_getContent(VuoTree tree, bool includeDescendants)
{
	if (! tree.rootXmlNode)
		return VuoText_make("");

	xmlChar *content = NULL;

	if (includeDescendants)
	{
		content = xmlNodeGetContent( (xmlNode *)tree.rootXmlNode );

		if (tree.children)
		{
			unsigned long childCount = VuoListGetCount_VuoTree(tree.children);
			for (unsigned long i = 1; i <= childCount; ++i)
			{
				VuoTree child = VuoListGetValue_VuoTree(tree.children, i);
				VuoText childContent = VuoTree_getContent(child, true);
				content = xmlStrcat(content, (const xmlChar *)childContent);
			}
		}
	}
	else
	{
		content = VuoTree_getContentOfXmlNode( (xmlNode *)tree.rootXmlNode );
	}

	VuoText contentAsText = VuoText_make((const char *)content);

	xmlFree(content);

	return contentAsText;
}

/**
 * Returns a list of the subtrees that are direct descendants of the tree's root item.
 */
VuoList_VuoTree VuoTree_getChildren(VuoTree tree)
{
	if (tree.children)
		return VuoListCopy_VuoTree(tree.children);

	VuoList_VuoTree children = VuoListCreate_VuoTree();

	if (! tree.rootXmlNode)
		return children;

	for (xmlNode *n = ((xmlNode *)tree.rootXmlNode)->children; n; n = n->next)
	{
		if (n->type == XML_ELEMENT_NODE)
		{
			VuoTree child = VuoTree_makeFromXmlNode(n);
			VuoListAppendValue_VuoTree(children, child);
		}
	}

	return children;
}

/**
 * Interprets the content and children of the tree as a JSON representation of a Vuo data type value.
 */
struct json_object * VuoTree_getContainedValue(VuoTree tree)
{
	VuoText jsonText = VuoTree_serializeAsJson(tree, false);
	VuoLocal(jsonText);

	json_object *json = json_tokener_parse(jsonText);

	json_object *child = NULL;
	if (json)
	{
		json_object_object_foreach(json, key, val)
		{
			child = val;
			json_object_get(child);
			break;
		}

		json_object_put(json);
	}

	return child;
}

/**
 * Returns the subtrees found by searching @a tree with XPath expression @a xpath,
 * or an empty list if there's an error.
 *
 * If @a xpath is a relative path, the root of @a tree is the context node.
 */
VuoList_VuoTree VuoTree_findItemsUsingXpath(VuoTree tree, VuoText xpath)
{
	VuoTree treeIncludingChildren;
	if (tree.children)
	{
		// The children may belong to different XML docs, so build a new XML doc that includes them.
		xmlChar *treeAsXml = VuoTree_serializeXmlNodeAsXml(tree, false, 0);
		treeIncludingChildren = VuoTree_parseXml((const char *)treeAsXml, false);
		VuoTree_retain(treeIncludingChildren);
		xmlFree(treeAsXml);
	}
	else
	{
		treeIncludingChildren = tree;
	}
	VuoDefer(^{ if (tree.children) VuoTree_release(treeIncludingChildren); });

	xmlNode *treeRoot = (xmlNode *)treeIncludingChildren.rootXmlNode;

	if (! treeRoot || VuoText_isEmpty(xpath))
		return VuoListCreate_VuoTree();

	xmlXPathContext *xpathContext = xmlXPathNewContext(treeRoot->doc);
	if (! xpathContext)
	{
		VUserLog("Error: Couldn't create xmlXPathContext");
		return VuoListCreate_VuoTree();
	}
	VuoDefer(^{ xmlXPathFreeContext(xpathContext); });

	// Replace smartquotes with plain quotes in the XPath.
	const int numSmartquotes = 4;
	const char *smartquotes[numSmartquotes] = { "“", "”", "‘", "’" };
	const char *plainquotes[numSmartquotes] = { "\"", "\"", "'", "'" };
	VuoText requotedXpaths[numSmartquotes];
	for (int i = 0; i < numSmartquotes; ++i)
	{
		requotedXpaths[i] = VuoText_replace(xpath, smartquotes[i], plainquotes[i]);
		xpath = requotedXpaths[i];
	}
	VuoText *requotedXPathsPtr = requotedXpaths;
	VuoDefer(^{
				 for (int i = 0; i < numSmartquotes; ++i)
				 {
					 if (requotedXPathsPtr[i] != xpath)
					 {
						 VuoRetain(requotedXPathsPtr[i]);
						 VuoRelease(requotedXPathsPtr[i]);
					 }
				 }
			 });

	// Set the xmlNode that relative XPaths are relative to — the root of the tree.
	int ret = xmlXPathSetContextNode(treeRoot, xpathContext);
	if (ret < 0)
	{
		VUserLog("Error: Couldn't set context node to '%s'", treeRoot->name);
		return VuoListCreate_VuoTree();
	}

	// Is the tree a child sharing an xmlDoc with its parent?
	bool isSubtree = (treeRoot != xmlDocGetRootElement(treeRoot->doc));

	xmlChar *prefixedXpath;
	if (isSubtree && xpath[0] == '/')
	{
		// Child tree + absolute path: Construct the full absolute path for the xmlDoc.
		xmlChar *xpathPrefix = xmlStrdup((const xmlChar *)"");
		for (xmlNode *parent = treeRoot->parent; parent; parent = parent->parent)
		{
			if (parent->type == XML_ELEMENT_NODE)
			{
				xpathPrefix = xmlStrcat(xmlStrdup(parent->name), xpathPrefix);
				xpathPrefix = xmlStrcat(xmlStrdup((const xmlChar *)"/"), xpathPrefix);
			}
		}
		prefixedXpath = xmlStrcat(xpathPrefix, (const xmlChar *)xpath);
	}
	else
	{
		prefixedXpath = xmlStrdup((const xmlChar *)xpath);
	}
	VuoDefer(^{ xmlFree(prefixedXpath); });

	// Perform the query.
	xmlXPathObject *xpathObject = xmlXPathEvalExpression(prefixedXpath, xpathContext);
	if (! xpathObject)
	{
		VUserLog("Error: Couldn't evaluate XPath expression '%s'", prefixedXpath);
		return VuoListCreate_VuoTree();
	}
	VuoDefer(^{ xmlXPathFreeObject(xpathObject); });

	if (xmlXPathNodeSetIsEmpty(xpathObject->nodesetval))
		return VuoListCreate_VuoTree();

	// Turn the found nodes into a set of unique element nodes.
	vector<xmlNode *> foundNodes;
	map<xmlNode *, vector<xmlNode *> > attributesForFoundNode;
	for (int i = 0; i < xpathObject->nodesetval->nodeNr; ++i)
	{
		xmlNode *foundNode = xpathObject->nodesetval->nodeTab[i];

		if (foundNode->type == XML_ATTRIBUTE_NODE || foundNode->type == XML_TEXT_NODE || foundNode->type == XML_CDATA_SECTION_NODE)
		{
			if (find(foundNodes.begin(), foundNodes.end(), foundNode->parent) == foundNodes.end())
				foundNodes.push_back(foundNode->parent);

			if (foundNode->type == XML_ATTRIBUTE_NODE)
				attributesForFoundNode[foundNode->parent].push_back(foundNode);
		}
		else if (foundNode->type == XML_ELEMENT_NODE)
		{
			foundNodes.push_back(foundNode);
		}
	}

	// Turn the unique element nodes into VuoTrees.
	VuoList_VuoTree foundTrees = VuoListCreate_VuoTree();
	for (vector<xmlNode *>::iterator i = foundNodes.begin(); i != foundNodes.end(); ++i)
	{
		xmlNode *foundNode = *i;

		if (isSubtree)
		{
			// Child tree: Skip found xmlNodes that are within the xmlDoc but not this subtree.
			bool isNodeInTree = false;
			for (xmlNode *n = foundNode; n; n = n->parent)
			{
				if (n == treeRoot)
				{
					isNodeInTree = true;
					break;
				}
			}
			if (! isNodeInTree)
				continue;
		}

		VuoTree foundTree;
		map<xmlNode *, vector<xmlNode *> >::iterator attributesIter = attributesForFoundNode.find(foundNode);
		if (attributesIter == attributesForFoundNode.end())
		{
			// Searching for elements or text: The returned tree is a subtree of @a tree (including attributes and content).
			foundTree = VuoTree_makeFromXmlNode(foundNode);
		}
		else
		{
			// Searching for attributes: The returned tree contains only the parent element and found attributes.
			VuoDictionary_VuoText_VuoText attributes = VuoDictionaryCreate_VuoText_VuoText();
			for (vector<xmlNode *>::iterator j = attributesIter->second.begin(); j != attributesIter->second.end(); ++j)
			{
				xmlNode *foundAttribute = *j;
				VuoText key = VuoText_make((const char *)foundAttribute->name);
				VuoText value = VuoText_make((const char *)xmlNodeGetContent(foundAttribute));
				VuoDictionarySetKeyValue_VuoText_VuoText(attributes, key, value);
			}
			foundTree = VuoTree_make((const char *)foundNode->name, attributes, "", NULL);
		}

		VuoListAppendValue_VuoTree(foundTrees, foundTree);
	}

	return foundTrees;
}

/**
 * Returns true if the subtree's name matches @a name.
 */
static bool compareName(xmlNode *node, VuoText name, VuoTextComparison comparison, VuoText unused)
{
	return VuoText_compare((const char *)node->name, comparison, name);
}

/**
 * Returns true if the subtree has an attribute called @a attribute and its value matches @a value.
 */
static bool compareAttribute(xmlNode *node, VuoText value, VuoTextComparison comparison, VuoText attribute)
{
	xmlChar *actualValue = xmlGetProp(node, (const xmlChar *)attribute);
	VuoDefer(^{ xmlFree(actualValue); });
	return VuoText_compare((const char *)actualValue, comparison, value);
}

/**
 * Returns true if the subtree's content matches @a content. This only checks the content of the tree
 * item itself, not the content of its descendants.
 */
static bool compareContent(xmlNode *node, VuoText content, VuoTextComparison comparison, VuoText unused)
{
	xmlChar *actualContent = VuoTree_getContentOfXmlNode(node);
	VuoDefer(^{ xmlFree(actualContent); });
	return VuoText_compare((const char *)actualContent, comparison, content);
}

/**
 * Helper function for the `VuoTree_findItemsWith*` functions. Traverses the eligible subtrees
 * (determined by @a includeDescendants and @a atFindRoot) and checks if each matches the search parameters.
 */
static VuoList_VuoTree VuoTree_findItems(VuoTree tree, bool (*compare)(xmlNode *node, VuoText, VuoTextComparison, VuoText),
										 VuoText targetText, VuoTextComparison comparison, VuoText attribute,
										 bool includeDescendants, bool atFindRoot)
{
	xmlNode *treeRoot = (xmlNode *)tree.rootXmlNode;

	if (! treeRoot)
		return VuoListCreate_VuoTree();

	VuoList_VuoTree foundTrees = VuoListCreate_VuoTree();
	if (compare(treeRoot, targetText, comparison, attribute))
		VuoListAppendValue_VuoTree(foundTrees, tree);

	if (atFindRoot || includeDescendants)
	{
		if (tree.children)
		{
			unsigned long childCount = VuoListGetCount_VuoTree(tree.children);
			for (unsigned long i = 1; i <= childCount; ++i)
			{
				VuoTree child = VuoListGetValue_VuoTree(tree.children, i);
				VuoList_VuoTree childFoundTrees = VuoTree_findItems(child, compare, targetText, comparison, attribute, includeDescendants, false);
				unsigned long foundCount = VuoListGetCount_VuoTree(childFoundTrees);
				for (unsigned long j = 1; j <= foundCount; ++j)
				{
					VuoTree childFoundTree = VuoListGetValue_VuoTree(childFoundTrees, j);
					VuoListAppendValue_VuoTree(foundTrees, childFoundTree);
				}
			}
		}
		else
		{
			list<xmlNode *> nodesToVisit;
			for (xmlNode *n = treeRoot->children; n; n = n->next)
				if (n->type == XML_ELEMENT_NODE)
					nodesToVisit.push_back(n);

			while (! nodesToVisit.empty())
			{
				xmlNode *node = nodesToVisit.front();
				nodesToVisit.pop_front();

				if (compare(node, targetText, comparison, attribute))
				{
					VuoTree foundTree = VuoTree_makeFromXmlNode(node);
					VuoListAppendValue_VuoTree(foundTrees, foundTree);
				}

				if (includeDescendants)
					for (xmlNode *n = node->children; n; n = n->next)
						if (n->type == XML_ELEMENT_NODE)
							nodesToVisit.push_back(n);
			}
		}
	}

	return foundTrees;
}

/**
 * Searches @a tree for subtrees whose name matches @a name.
 *
 * If @a includeDescendants is false, only the tree's root and its immediate children are searched.
 * If @a includeDescendants is true, all descendants are searched.
 */
VuoList_VuoTree VuoTree_findItemsWithName(VuoTree tree, VuoText name, VuoTextComparison comparison, bool includeDescendants)
{
	return VuoTree_findItems(tree, compareName, name, comparison, NULL, includeDescendants, true);
}

/**
 * Searches @a tree for subtrees that have an attribute called @a attribute (exact match) and its
 * value matches @a value (based on @a valueComparison).
 *
 * If @a includeDescendants is false, only the tree's root and its immediate children are searched.
 * If @a includeDescendants is true, all descendants are searched.
 */
VuoList_VuoTree VuoTree_findItemsWithAttribute(VuoTree tree, VuoText attribute, VuoText value, VuoTextComparison valueComparison, bool includeDescendants)
{
	return VuoTree_findItems(tree, compareAttribute, value, valueComparison, attribute, includeDescendants, true);
}

/**
 * Searches @a tree for subtrees whose content matches @a name.
 *
 * If a subtree contains a child subtree whose content matches, only the child subtree is returned,
 * not the parent.
 *
 * If @a includeDescendants is false, only the tree's root and its immediate children are searched.
 * If @a includeDescendants is true, all descendants are searched.
 */
VuoList_VuoTree VuoTree_findItemsWithContent(VuoTree tree, VuoText content, VuoTextComparison comparison, bool includeDescendants)
{
	return VuoTree_findItems(tree, compareContent, content, comparison, NULL, includeDescendants, true);
}

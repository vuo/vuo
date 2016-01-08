/**
 * @file
 * VuoCompilerSpecializedNodeClass interface.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERSPECIALIZEDNODECLASS_H
#define VUOCOMPILERSPECIALIZEDNODECLASS_H

#include "VuoCompilerNodeClass.hh"
#include "VuoGenericType.hh"

/**
 * A specialization of a generic node class.
 *
 * In this node class, anywhere from none to all of the generic port types in the generic node class
 * are replaced by concrete port types.
 */
class VuoCompilerSpecializedNodeClass : public VuoCompilerNodeClass
{
private:
	VuoCompilerNodeClass *genericNodeClass;
	static void replaceGenericTypesWithSpecialized(string &nodeClassSource, map<string, string> specializedForGenericTypeName, VuoNode *nodeToBack);

protected:
	map<string, string> specializedForGenericTypeName;  ///< The type name that replaces each generic type name in the original generic node class

	VuoCompilerSpecializedNodeClass(string nodeClassName, Module *module);
	VuoCompilerSpecializedNodeClass(VuoCompilerSpecializedNodeClass *compilerNodeClass, VuoNode *nodeToBack);

public:
	virtual VuoType * getOriginalPortType(VuoPortClass *portClass);
	virtual string getOriginalGenericNodeClassName(void);
	virtual string getOriginalGenericNodeClassDescription(void);
	virtual VuoNodeSet *getOriginalGenericNodeSet(void);
	virtual string createUnspecializedNodeClassName(set<VuoPortClass *> portClassesToUnspecialize);
	virtual string createSpecializedNodeClassNameWithReplacement(string genericTypeName, string specializedTypeName);
	string createDefaultSpecializedNodeClassName(void);

	static VuoNodeClass * newNodeClass(string nodeClassName, VuoCompiler *compiler, dispatch_queue_t llvmQueue, VuoNode *nodeToBack=NULL);
	static void replaceGenericTypesWithBacking(string &nodeClassSource, VuoNode *nodeToBack=NULL);
	static vector<string> getGenericTypeNamesFromPorts(VuoCompilerNodeClass *nodeClass);
	static map<string, string> getBackingTypeNamesFromPorts(VuoNodeClass *nodeClass);
	static map<string, string> getBackingTypeNamesFromPorts(VuoNode *node);
	static string extractGenericNodeClassName(string specializedNodeClassName, size_t genericTypeCount);
	static string createSpecializedNodeClassName(string genericNodeClassName, vector<string> specializedTypeNames);
};

#endif

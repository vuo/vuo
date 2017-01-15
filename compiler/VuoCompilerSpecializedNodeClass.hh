/**
 * @file
 * VuoCompilerSpecializedNodeClass interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERSPECIALIZEDNODECLASS_H
#define VUOCOMPILERSPECIALIZEDNODECLASS_H

#include "VuoCompilerNodeClass.hh"

class VuoCompiler;
class VuoCompilerInstanceDataClass;
class VuoCompilerNode;
class VuoCompilerNodeClass;
class VuoNode;

/**
 * A specialization of a generic node class.
 *
 * When the compiler loads the node classes from the module folders, each generic node class (e.g. `vuo.data.hold`)
 * is represented by a VuoCompilerNodeClass (not VuoCompilerSpecializedNodeClass) instance.
 *
 * When the compiler instantiates a specialization of a generic node class to add to a composition, it does this in one
 * of two ways. If the node class is fully specialized (e.g. `vuo.data.hold.VuoText`), then it looks up or creates the
 * VuoCompilerSpecializedNodeClass with that implementation (e.g. `vuo.data.hold.c` compiled with `VuoText` in place of
 * `VuoGenericType1`). If the node class is not fully specialized (e.g. `vuo.data.hold.VuoGenericType1`), then it creates
 * a unique VuoCompilerSpecializedNodeClass for that node, and that VuoCompilerSpecializedNodeClass does not yet have an
 * implementation.
 *
 * When the compiler compiles a composition, each not-fully-specialized VuoCompilerSpecializedNodeClass
 * (e.g. `vuo.data.hold.VuoGenericType1`) is assigned an implementation, in the form of a "backing" node class
 * (e.g. `vuo.data.hold.VuoText`). The backing node class is a fully-specialized VuoCompilerSpecializedNodeClass that's
 * effectively substituted in for the not-fully-specialized node class during the compiling of the composition.
 *
 * If two nodes in a composition are of the same not-fully-specialized node class, then each node has its own unique
 * VuoCompilerSpecializedNodeClass instance. These two VuoCompilerSpecializedNodeClass instances may have different backing
 * node classes or the same backing node class, depending on what each node is connected to in the composition.
 */
class VuoCompilerSpecializedNodeClass : public VuoCompilerNodeClass
{
private:
	VuoCompilerNodeClass *genericNodeClass;
	VuoCompilerNodeClass *backingNodeClass;

	string createDefaultSpecializedNodeClassName(void);
	string createFullySpecializedNodeClassName(VuoNode *nodeToBack);
	static void replaceGenericTypesWithSpecialized(string &nodeClassSource, map<string, string> specializedForGenericTypeName);
	static vector<string> getGenericTypeNamesFromPorts(VuoCompilerNodeClass *nodeClass);
	static map<string, string> getBackingTypeNamesFromPorts(VuoNode *node);
	static string extractGenericNodeClassName(string specializedNodeClassName, size_t genericTypeCount);
	static string createSpecializedNodeClassName(string genericNodeClassName, vector<string> specializedTypeNames);

	void initialize(void);
	string getClassIdentifier(void);
	Function * getEventFunction(void);
	Function * getInitFunction(void);
	Function * getFiniFunction(void);
	Function * getCallbackStartFunction(void);
	Function * getCallbackUpdateFunction(void);
	Function * getCallbackStopFunction(void);
	Function * getCompositionContextInitFunction(void);
	Function * getCompositionContextFiniFunction(void);
	Function * getCompositionSerializeFunction(void);
	Function * getCompositionUnserializeFunction(void);
	Function * getTriggerWorkerFunction(string portIdentifier);
	vector<VuoCompilerTriggerDescription *> getTriggerDescriptions(void);
	VuoCompilerInstanceDataClass * getInstanceDataClass(void);
	string getDoxygenDocumentation(void);
	string getDefaultSpecializedTypeName(string genericTypeName);
	vector<string> getAutomaticKeywords(void);
	set<string> getDependencies(void);
	bool isStateful(void);

protected:
	map<string, string> specializedForGenericTypeName;  ///< The type name that replaces each generic type name in the original generic node class

	VuoCompilerSpecializedNodeClass(string nodeClassName, Module *module);
	VuoCompilerSpecializedNodeClass(VuoCompilerSpecializedNodeClass *compilerNodeClass);
	VuoCompilerSpecializedNodeClass(VuoNodeClass *baseNodeClass);

	friend class TestNodes;
	friend class TestVuoCompilerNodeClass;

public:
	void updateBackingNodeClass(VuoNode *nodeToBack, VuoCompiler *compiler);
	virtual VuoCompilerNode * createReplacementBackingNode(VuoNode *nodeToBack, string backingNodeClassName, VuoCompiler *compiler);
	bool isFullySpecialized(void);
	virtual VuoType * getOriginalPortType(VuoPortClass *portClass);
	virtual string getOriginalGenericNodeClassName(void);
	virtual string getOriginalGenericNodeClassDescription(void);
	virtual VuoNodeSet * getOriginalGenericNodeSet(void);
	virtual string createUnspecializedNodeClassName(set<VuoPortClass *> portClassesToUnspecialize);
	virtual string createSpecializedNodeClassNameWithReplacement(string genericTypeName, string specializedTypeName);
	string getDependencyName(void);

	static VuoNodeClass * newNodeClass(string nodeClassName, VuoCompiler *compiler, dispatch_queue_t llvmQueue);
	static VuoCompilerNodeClass * getNodeClassForNode(VuoCompilerNodeClass *origNodeClass, VuoCompiler *compiler);
	static void replaceGenericTypesWithBacking(string &nodeClassSource);
};

#endif

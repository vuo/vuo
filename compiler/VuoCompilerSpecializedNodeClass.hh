/**
 * @file
 * VuoCompilerSpecializedNodeClass interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCompilerNodeClass.hh"

class VuoCompiler;
class VuoCompilerInstanceDataClass;
class VuoCompilerNode;
class VuoNode;
class VuoNodeSet;

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
	string genericNodeClassName;  ///< The name of the generic node class that this node class specializes.
	VuoCompilerNodeClass *genericNodeClass;  ///< The node class with name `genericNodeClassName`, or null if it hasn't yet been filled in by `updateGenericNodeClass()`.
	VuoCompilerNodeClass *backingNodeClass;

	void parseSpecializedModuleDetails(void);
	string createDefaultSpecializedNodeClassName(void);
	string createFullySpecializedNodeClassName(VuoNode *nodeToBack);
	static vector<string> getGenericTypeNamesFromPorts(VuoCompilerNodeClass *nodeClass);
	static map<string, string> getBackingTypeNamesFromPorts(VuoNode *node);

	string getClassIdentifier(void);
	Function * getEventFunction(void);
	Function * getInitFunction(void);
	Function * getFiniFunction(void);
	Function * getCallbackStartFunction(void);
	Function * getCallbackUpdateFunction(void);
	Function * getCallbackStopFunction(void);
	Function * getCompositionAddNodeMetadataFunction(void);
	Function * getCompositionPerformDataOnlyTransmissionsFunction(void);
	Function * getCompositionSetPublishedInputPortValueFunction(void);
	Function * getTriggerWorkerFunction(string portIdentifier);
	vector<VuoCompilerTriggerDescription *> getTriggerDescriptions(void);
	VuoCompilerInstanceDataClass * getInstanceDataClass(void);
	string getDoxygenDocumentation(void);
	string getDefaultSpecializedTypeName(string genericTypeName);
	vector<string> getAutomaticKeywords(void);
	bool isStateful(void);

protected:
	map<string, string> specializedForGenericTypeName;  ///< The type name that replaces each generic type name in the original generic node class

	VuoCompilerSpecializedNodeClass(string nodeClassName, Module *module);
	VuoCompilerSpecializedNodeClass(VuoCompilerSpecializedNodeClass *compilerNodeClass);
	VuoCompilerSpecializedNodeClass(VuoNodeClass *baseNodeClass);
	static VuoPortClass * copyPortClassFromModel(VuoPortClass *modelPortClass, bool isInput);

	friend class TestNodes;
	friend class TestVuoCompilerNodeClass;

public:
	bool updateGenericNodeClass(std::function<VuoCompilerNodeClass *(const string &)> lookUpNodeClass);
	void updateBackingNodeClass(VuoNode *nodeToBack, VuoCompiler *compiler);
	virtual VuoCompilerNode * createReplacementBackingNode(VuoNode *nodeToBack, string backingNodeClassName, VuoCompiler *compiler);
	bool isFullySpecialized(void);
	static bool hasGenericPortTypes(VuoCompilerNodeClass *nodeClass);
	virtual VuoType * getOriginalPortType(VuoPortClass *portClass);
	virtual string getOriginalGenericNodeClassName(void);
	virtual string getOriginalGenericNodeClassDescription(void);
	virtual VuoNodeSet * getOriginalGenericNodeSet(void);
	virtual string createUnspecializedNodeClassName(set<VuoPortClass *> portClassesToUnspecialize);
	virtual string createSpecializedNodeClassNameWithReplacement(string genericTypeName, string specializedTypeName);
	static string parseGenericNodeClassName(string specializedNodeClassName, size_t genericTypeCount);
	static string createSpecializedNodeClassName(string genericNodeClassName, vector<string> specializedTypeNames);
	set<string> getDependencies(void);
	string getDependencyName(void);

	static VuoNodeClass * newNodeClass(const string &nodeClassName, VuoCompiler *compiler, dispatch_queue_t llvmQueue);
	static VuoNodeClass * newNodeClass(const string &nodeClassName, Module *module);
	static json_object * buildSpecializedModuleDetails(const map<string, string> &specializedForGenericTypeName, const string &genericNodeClassName = "");
	static VuoCompilerNodeClass * getNodeClassForNode(VuoCompilerNodeClass *origNodeClass, VuoCompiler *compiler);
	static bool isSpecializationOfNodeClass(const string &potentialSpecializedNodeClassName, VuoCompilerNodeClass *potentialGenericNodeClass);
};

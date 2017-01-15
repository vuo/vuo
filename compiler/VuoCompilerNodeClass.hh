/**
 * @file
 * VuoCompilerNodeClass interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERNODECLASS_H
#define VUOCOMPILERNODECLASS_H

#include "VuoBaseDetail.hh"
#include "VuoCompilerModule.hh"
#include "VuoNodeClass.hh"

class VuoCompiler;
class VuoCompilerInputEventPortClass;
class VuoCompilerInputDataClass;
class VuoCompilerInstanceDataClass;
class VuoCompilerNodeArgumentClass;
class VuoCompilerOutputDataClass;
class VuoCompilerOutputEventPortClass;
class VuoCompilerTriggerDescription;
class VuoCompilerTriggerPortClass;
class VuoNode;
class VuoNodeClass;
class VuoPortClass;
class VuoType;

/**
 * The compiler detail class for @c VuoNodeClass.
 */
class VuoCompilerNodeClass : public VuoBaseDetail<VuoNodeClass>, public VuoCompilerModule  // Order matters: VuoCompilerModule after VuoBaseDetail - http://stackoverflow.com/a/2254306/238387
{
private:
	Function *eventFunction;
	Function *initFunction;
	Function *finiFunction;
	Function *callbackStartFunction;
	Function *callbackUpdateFunction;
	Function *callbackStopFunction;
	VuoCompilerInstanceDataClass *instanceDataClass;
	vector<VuoCompilerTriggerDescription *> triggerDescriptions;
	map<string, vector<string> > compatibleSpecializedForGenericTypeName;
	set<VuoCompilerInputEventPortClass *> portsWithExplicitEventBlockingNone;

	static bool isNodeClass(Module *module, string moduleKey);
	void parse(void);
	set<string> globalsToRename(void);
	void parseMetadata(void);
	void parseEventFunction(void);
	void parseInitFunction(void);
	void parseFiniFunction(void);
	void parseCallbackStartFunction(void);
	void parseCallbackUpdateFunction(void);
	void parseCallbackStopFunction(void);
	void parseParameters(Function *function, unsigned long acceptanceFlags);
	void instantiateCompilerNode(VuoNode *node);

	VuoCompilerInputDataClass * parseInputDataParameter(string annotation, Argument *a);
	VuoCompilerOutputDataClass * parseOutputDataParameter(string annotation, Argument *a);
	VuoCompilerInputEventPortClass * parseInputEventParameter(string annotation, Argument *a);
	VuoCompilerOutputEventPortClass * parseOutputEventParameter(string annotation, Argument *a);
	VuoCompilerTriggerPortClass * parseTriggerParameter(string annotation, Argument *a);
	VuoCompilerInstanceDataClass * parseInstanceDataParameter(string annotation, Argument *a);
	VuoType * parseTypeParameter(string annotation);
	struct json_object * parseDetailsParameter(string annotation);
	VuoPortClass * getExistingPortClass(VuoCompilerNodeArgumentClass *argumentClass, bool isInput);

	friend class TestVuoCompilerType;
	friend class TestVuoCompilerNodeClass;

protected:
	VuoCompilerNodeClass(string className, Module * module);
	VuoCompilerNodeClass(VuoCompilerNodeClass *compilerNodeClass);
	VuoCompilerNodeClass(VuoNodeClass *baseNodeClass);
	VuoPortClass * getInputPortClassWithName(string portName);
	VuoPortClass * getOutputPortClassWithName(string portName);

	map<string, string> defaultSpecializedForGenericTypeName;  ///< If this node class is generic, use these specialized types when creating an instance.

	static void parseGenericTypes(json_object *moduleDetails, map<string, string> &defaultSpecializedForGenericTypeName, map<std::string, vector<std::string> > &compatibleSpecializedForGenericTypeName);

public:
	VuoNode * newNode(string title = "", double x = 0, double y = 0);
	VuoNode * newNode(VuoNode *nodeToCopyMetadataFrom);
	static VuoNodeClass * newNodeClass(string nodeClassName, Module * module);
	static VuoNodeClass * newNodeClassWithoutImplementation(VuoNodeClass *baseNodeClass);
	virtual ~VuoCompilerNodeClass(void);

	virtual string getClassIdentifier(void);
	virtual Function * getEventFunction(void);
	virtual Function * getInitFunction(void);
	virtual Function * getFiniFunction(void);
	virtual Function * getCallbackStartFunction(void);
	virtual Function * getCallbackUpdateFunction(void);
	virtual Function * getCallbackStopFunction(void);
	virtual Function * getCompositionContextInitFunction(void);
	virtual Function * getCompositionContextFiniFunction(void);
	virtual Function * getCompositionSerializeFunction(void);
	virtual Function * getCompositionUnserializeFunction(void);
	virtual Function * getTriggerWorkerFunction(string portIdentifier);
	virtual vector<VuoCompilerTriggerDescription *> getTriggerDescriptions(void);
	virtual VuoCompilerInstanceDataClass * getInstanceDataClass(void);
	virtual string getDoxygenDocumentation(void);
	virtual string getDefaultSpecializedTypeName(string genericTypeName);
	virtual vector<string> getAutomaticKeywords(void);
	virtual bool isStateful(void);
	bool isSubcomposition(void);
};

#endif

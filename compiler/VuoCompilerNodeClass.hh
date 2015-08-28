/**
 * @file
 * VuoCompilerNodeClass interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERNODECLASS_H
#define VUOCOMPILERNODECLASS_H

#include "VuoBaseDetail.hh"
#include "VuoCompilerModule.hh"

#include "VuoNode.hh"
#include "VuoNodeClass.hh"

#include "VuoCompilerTriggerPortClass.hh"
#include "VuoCompilerInputEventPortClass.hh"
#include "VuoCompilerInstanceDataClass.hh"
#include "VuoCompilerOutputEventPortClass.hh"
#include "VuoCompilerPortClass.hh"

class VuoCompiler;

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
	map<string, set<string> > compatibleSpecializedForGenericTypeName;
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
	VuoCompilerInputEventPortClass * parseInputEventParameter(string annotation, Argument *a, string &dataPortName);
	VuoCompilerOutputEventPortClass * parseOutputEventParameter(string annotation, Argument *a, string &dataPortName);
	VuoCompilerTriggerPortClass * parseTriggerParameter(string annotation, Argument *a);
	VuoCompilerInstanceDataClass * parseInstanceDataParameter(string annotation, Argument *a);
	VuoType * parseTypeParameter(string annotation);
	struct json_object * parseInputDataDetailsParameter(string annotation);
	struct json_object * parseInputEventDetailsParameter(string annotation);
	int parseEventBlockingParameter(string annotation);
	int parseEventThrottlingParameter(string annotation);
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

	static void parseGenericTypes(json_object *moduleDetails, map<string, string> &defaultSpecializedForGenericTypeName, map<string, set<string> > &compatibleSpecializedForGenericTypeName);

public:
	VuoNode * newNode(string title = "", double x = 0, double y = 0);
	VuoNode * newNode(VuoNode *nodeToCopyMetadataFrom);
	static VuoNodeClass * newNodeClass(string nodeClassName, Module * module);
	static VuoNodeClass * newNodeClassWithoutImplementation(VuoNodeClass *baseNodeClass);

	string getClassIdentifier(void);
	Function * getEventFunction(void);
	Function * getInitFunction(void);
	Function * getFiniFunction(void);
	Function * getCallbackStartFunction(void);
	Function * getCallbackUpdateFunction(void);
	Function * getCallbackStopFunction(void);
	VuoCompilerInstanceDataClass * getInstanceDataClass(void);
	string getDoxygenDocumentation(void);
	string getDefaultSpecializedTypeName(string genericTypeName);
	vector<string> getAutomaticKeywords(void);
};

#endif

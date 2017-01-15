/**
 * @file
 * VuoCompilerNodeClass implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <sstream>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <json-c/json.h>
#pragma clang diagnostic pop

#include "VuoCompiler.hh"
#include "VuoCompilerBitcodeParser.hh"
#include "VuoCompilerDataClass.hh"
#include "VuoCompilerInputDataClass.hh"
#include "VuoCompilerInputEventPortClass.hh"
#include "VuoCompilerInstanceDataClass.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerNodeArgumentClass.hh"
#include "VuoCompilerNodeClass.hh"
#include "VuoCompilerOutputDataClass.hh"
#include "VuoCompilerOutputEventPortClass.hh"
#include "VuoCompilerPort.hh"
#include "VuoCompilerPortClass.hh"
#include "VuoCompilerSpecializedNodeClass.hh"
#include "VuoCompilerTriggerDescription.hh"
#include "VuoCompilerTriggerPortClass.hh"
#include "VuoGenericType.hh"
#include "VuoPort.hh"
#include "VuoPortClass.hh"
#include "VuoStringUtilities.hh"


/**
 * Flags indicating how each kind of argument class is accepted in a node function's parameter list.
 * Specify @c *_PRESENT and @c *_ABSENT if optional, @c *_PRESENT if required, @c *_ABSENT if disallowed.
 */
enum VuoNodeArgumentAcceptance
{
	INPUT_DATA_ABSENT = 1 << 0,
	INPUT_DATA_PRESENT = 1 << 1,
	OUTPUT_DATA_ABSENT = 1 << 2,
	OUTPUT_DATA_PRESENT = 1 << 3,
	INPUT_EVENT_ABSENT = 1 << 4,
	INPUT_EVENT_PRESENT = 1 << 5,
	OUTPUT_EVENT_ABSENT = 1 << 6,
	OUTPUT_EVENT_PRESENT = 1 << 7,
	OUTPUT_TRIGGER_ABSENT = 1 << 8,
	OUTPUT_TRIGGER_PRESENT = 1 << 9,
	INSTANCE_DATA_ABSENT = 1 << 10,
	INSTANCE_DATA_PRESENT = 1 << 11
};


/**
 * Creates a node class implementation from an LLVM module, and creates its corresponding base @c VuoNodeClass.
 */
VuoCompilerNodeClass::VuoCompilerNodeClass(string className, Module *module)
	: VuoBaseDetail<VuoNodeClass>("VuoCompilerNodeClass with dummy base", new VuoNodeClass(className, vector<string>(), vector<string>())),
	  VuoCompilerModule(getBase(), module)
{
	instanceDataClass = NULL;
	initFunction = NULL;
	finiFunction = NULL;
	callbackStartFunction = NULL;
	callbackUpdateFunction = NULL;
	callbackStopFunction = NULL;

	parse();
}

/**
 * Creates a new compiler node class and creates a new base @c VuoNodeClass, both from @a compilerNodeClass.
 *
 * Transfers ownership of the port classes and instance data class from @a compilerNodeClass to the
 * newly constructed node class so that @a compilerNodeClass can be safely destroyed.
 */
VuoCompilerNodeClass::VuoCompilerNodeClass(VuoCompilerNodeClass *compilerNodeClass)
	: VuoBaseDetail<VuoNodeClass>("VuoCompilerNodeClass with substantial base", new VuoNodeClass(
		compilerNodeClass->getBase()->getClassName(),
		compilerNodeClass->getBase()->getRefreshPortClass(),
		compilerNodeClass->getBase()->getInputPortClasses(),
		compilerNodeClass->getBase()->getOutputPortClasses())),
	  VuoCompilerModule(getBase(), compilerNodeClass->module)
{
	getBase()->setDefaultTitle(compilerNodeClass->getBase()->getDefaultTitle());
	getBase()->setDescription(compilerNodeClass->getBase()->getDescription());
	getBase()->setVersion(compilerNodeClass->getBase()->getVersion());
	getBase()->setKeywords(compilerNodeClass->getBase()->getKeywords());
	getBase()->setInterface(compilerNodeClass->getBase()->isInterface());
	getBase()->setNodeSet(compilerNodeClass->getBase()->getNodeSet());
	getBase()->setExampleCompositionFileNames(compilerNodeClass->getBase()->getExampleCompositionFileNames());
	getBase()->setDeprecated(compilerNodeClass->getBase()->getDeprecated());
	getBase()->setCompiler(this);

	this->dependencies = compilerNodeClass->dependencies;
	this->eventFunction = compilerNodeClass->eventFunction;
	this->initFunction = compilerNodeClass->initFunction;
	this->finiFunction = compilerNodeClass->finiFunction;
	this->callbackStartFunction = compilerNodeClass->callbackStartFunction;
	this->callbackUpdateFunction = compilerNodeClass->callbackUpdateFunction;
	this->callbackStopFunction = compilerNodeClass->callbackStopFunction;
	this->instanceDataClass = compilerNodeClass->instanceDataClass;
	this->triggerDescriptions = compilerNodeClass->triggerDescriptions;
	this->defaultSpecializedForGenericTypeName = compilerNodeClass->defaultSpecializedForGenericTypeName;

	compilerNodeClass->getBase()->setInputPortClasses(vector<VuoPortClass *>());
	compilerNodeClass->getBase()->setOutputPortClasses(vector<VuoPortClass *>());
	compilerNodeClass->instanceDataClass = NULL;
}

/**
 * Creates a new implementation-less compiler node class, using the given node class for its base VuoNodeClass.
 */
VuoCompilerNodeClass::VuoCompilerNodeClass(VuoNodeClass *baseNodeClass)
	: VuoBaseDetail<VuoNodeClass>("VuoCompilerNodeClass with existing base", baseNodeClass),
	  VuoCompilerModule(baseNodeClass, NULL)
{
	getBase()->setCompiler(this);

	instanceDataClass = NULL;
	eventFunction = NULL;
	initFunction = NULL;
	finiFunction = NULL;
	callbackStartFunction = NULL;
	callbackUpdateFunction = NULL;
	callbackStopFunction = NULL;
}

/**
 * Destructor.
 */
VuoCompilerNodeClass::~VuoCompilerNodeClass(void)
{
	delete instanceDataClass;

	vector<VuoPortClass *> inputPortClasses = getBase()->getInputPortClasses();
	for (vector<VuoPortClass *>::iterator i = inputPortClasses.begin(); i != inputPortClasses.end(); ++i)
		delete (*i)->getCompiler();

	vector<VuoPortClass *> outputPortClasses = getBase()->getOutputPortClasses();
	for (vector<VuoPortClass *>::iterator i = outputPortClasses.begin(); i != outputPortClasses.end(); ++i)
		delete (*i)->getCompiler();
}

/**
 * Creates a substantial base node instance from this node class.
 */
VuoNode * VuoCompilerNodeClass::newNode(string title, double x, double y)
{
	if (title.empty())
		title = getBase()->getDefaultTitleWithoutSuffix();

	VuoNode *n = getBase()->newNode(title, x, y);
	instantiateCompilerNode(n);
	return n;
}

/**
 * Creates a substantial base node instance with its metadata copied from the existing @c nodeToCopyMetadataFrom.
 */
VuoNode * VuoCompilerNodeClass::newNode(VuoNode *nodeToCopyMetadataFrom)
{
	VuoNode *n = getBase()->newNode(nodeToCopyMetadataFrom);
	instantiateCompilerNode(n);
	return n;
}

/**
  * Instantiates a compiler node and ports for the provided base @c node.
  *
  * Helper function for VuoCompilerNodeClass::newNode().
  */
void VuoCompilerNodeClass::instantiateCompilerNode(VuoNode *node)
{
	// Instantiate compiler ports.
	vector<VuoPort *> inputPorts = node->getInputPorts();
	vector<VuoPort *> outputPorts = node->getOutputPorts();
	vector<VuoPort *> ports;
	ports.insert(ports.end(), inputPorts.begin(), inputPorts.end());
	ports.insert(ports.end(), outputPorts.begin(), outputPorts.end());

	for (vector<VuoPort *>::iterator i = ports.begin(); i != ports.end(); ++i)
	{
		VuoPort *basePort = *i;
		VuoCompilerPortClass *portClass = static_cast<VuoCompilerPortClass *>(basePort->getClass()->getCompiler());
		basePort->setCompiler(portClass->newPort(basePort));
	}

	// Instantiate compiler node.
	new VuoCompilerNode(node);
}

/**
 * Creates a new implementation-less compiler node class, using the given node class for its base VuoNodeClass.
 */
VuoNodeClass * VuoCompilerNodeClass::newNodeClassWithoutImplementation(VuoNodeClass *baseNodeClass)
{
	return (new VuoCompilerNodeClass(baseNodeClass))->getBase();
}

/**
 * Creates a new substantial @c VuoNodeClass from the node class definition in the module.
 * If the module does not contain a node class definition, returns NULL.
 */
VuoNodeClass * VuoCompilerNodeClass::newNodeClass(string nodeClassName, Module * module)
{
	if (! isNodeClass(module, nodeClassName))
		return NULL;

	VuoCompilerNodeClass * cnc = new VuoCompilerNodeClass(nodeClassName, module);

	// Reconstruct, this time with a base VuoNodeClass containing actual (non-dummy) ports.
	VuoCompilerNodeClass * cnc2 = new VuoCompilerNodeClass(cnc);
	delete cnc;
	return cnc2->getBase();
}

/**
 * Returns true if the LLVM module contains certain functions, indicating that it is
 * intended to be a node class definition.
 */
bool VuoCompilerNodeClass::isNodeClass(Module *module, string moduleKey)
{
	return hasOriginalOrMangledGlobal("nodeEvent", module, moduleKey) ||
			hasOriginalOrMangledGlobal("nodeInstanceEvent", module, moduleKey);
}

/**
 * Overrides the implementation in @c VuoCompilerModule.
 */
void VuoCompilerNodeClass::parse(void)
{
	VuoCompilerModule::parse();

	parseEventFunction();
	if (instanceDataClass)
	{
		parseInitFunction();
		parseFiniFunction();
		parseCallbackStartFunction();
		parseCallbackUpdateFunction();
		parseCallbackStopFunction();
	}

	// Add each non-generic port type as a dependency.
	vector<VuoPortClass *> portClasses;
	vector<VuoPortClass *> inputPortClasses = getBase()->getInputPortClasses();
	vector<VuoPortClass *> outputPortClasses = getBase()->getOutputPortClasses();
	portClasses.insert(portClasses.end(), inputPortClasses.begin(), inputPortClasses.end());
	portClasses.insert(portClasses.end(), outputPortClasses.begin(), outputPortClasses.end());
	for (vector<VuoPortClass *>::iterator i = portClasses.begin(); i != portClasses.end(); ++i)
	{
		VuoType *type = static_cast<VuoCompilerPortClass *>((*i)->getCompiler())->getDataVuoType();
		if (type && ! dynamic_cast<VuoGenericType *>(type))
		{
			string typeName = type->getModuleKey();
			dependencies.insert(typeName);
			string innermostTypeName = VuoType::extractInnermostTypeName(typeName);
			dependencies.insert(innermostTypeName);
		}
	}
}

/**
 * Overrides the implementation in @c VuoCompilerModule.
 */
set<string> VuoCompilerNodeClass::globalsToRename(void)
{
	set<string> globals = VuoCompilerModule::globalsToRename();
	globals.insert("nodeEvent");
	globals.insert("nodeInstanceEvent");
	globals.insert("nodeInstanceInit");
	globals.insert("nodeInstanceFini");
	globals.insert("nodeInstanceTriggerStart");
	globals.insert("nodeInstanceTriggerUpdate");
	globals.insert("nodeInstanceTriggerStop");
	return globals;
}

/**
 * Parses the metadata of the node class (name, description, ...) from the LLVM module.
 */
void VuoCompilerNodeClass::parseMetadata(void)
{
	VuoCompilerModule::parseMetadata();

	json_object *nodeDetails = NULL;
	if (json_object_object_get_ex(moduleDetails, "node", &nodeDetails))
	{
		getBase()->setInterface( parseBool(nodeDetails, "isInterface") );
		getBase()->setExampleCompositionFileNames( parseArrayOfStrings(nodeDetails, "exampleCompositions") );
		getBase()->setDeprecated( parseBool(nodeDetails, "isDeprecated") );

		json_object *triggersArray = NULL;
		if (json_object_object_get_ex(nodeDetails, "triggers", &triggersArray))
			triggerDescriptions = VuoCompilerTriggerDescription::parseFromJson(triggersArray);
	}

	parseGenericTypes(moduleDetails, defaultSpecializedForGenericTypeName, compatibleSpecializedForGenericTypeName);
}

/**
 * Parses the "genericTypes" portion of a node class's metadata.
 *
 * @see VuoModuleMetadata
 */
void VuoCompilerNodeClass::parseGenericTypes(json_object *moduleDetails,
											 map<string, string> &defaultSpecializedForGenericTypeName,
											 map<string, vector<string> > &compatibleSpecializedForGenericTypeName)
{
	json_object *genericTypeDetails = NULL;
	if (json_object_object_get_ex(moduleDetails, "genericTypes", &genericTypeDetails))
	{
		json_object_object_foreach(genericTypeDetails, genericTypeName, genericTypeDetailsForOneType)
		{
			string defaultType = parseString(genericTypeDetailsForOneType, "defaultType");
			if (! defaultType.empty())
				defaultSpecializedForGenericTypeName[genericTypeName] = defaultType;

			vector<string> compatibleTypes = parseArrayOfStrings(genericTypeDetailsForOneType, "compatibleTypes");
			if (! compatibleTypes.empty())
				compatibleSpecializedForGenericTypeName[genericTypeName] = compatibleTypes;
		}
	}
}

/**
 * Parses the event function (nodeEvent or nodeInstanceEvent) from the LLVM module.
 *
 * Assumes this function is called before @c parseInitFunction(), @c parseCallbackStartFunction(),
 * and @c parseCallbackUpdateFunction() (for handling parameters that appear in multiple functions).
 */
void VuoCompilerNodeClass::parseEventFunction(void)
{
	eventFunction = parser->getFunction(nameForGlobal("nodeEvent"));
	VuoNodeArgumentAcceptance acceptsInstanceData = INSTANCE_DATA_ABSENT;

	if (! eventFunction)
	{
		eventFunction = parser->getFunction(nameForGlobal("nodeInstanceEvent"));
		acceptsInstanceData = INSTANCE_DATA_PRESENT;

		if (! eventFunction)
		{
			VUserLog("Error: Node class '%s' is missing function nodeEvent or nodeInstanceEvent.", getBase()->getClassName().c_str());
			return;
		}
	}

	parseParameters(eventFunction,
				   INPUT_DATA_ABSENT | INPUT_DATA_PRESENT |
				   OUTPUT_DATA_ABSENT | OUTPUT_DATA_PRESENT |
				   INPUT_EVENT_ABSENT | INPUT_EVENT_PRESENT |
				   OUTPUT_EVENT_ABSENT | OUTPUT_EVENT_PRESENT |
				   OUTPUT_TRIGGER_ABSENT | OUTPUT_TRIGGER_PRESENT |
				   acceptsInstanceData);
}

/**
 * Parses the init function from the LLVM module.
 *
 * Assumes this function is called after @c parseEventFunction(), and before @c parseCallbackStartFunction()
 * and @c parseCallbackUpdateFunction() (for handling parameters that appear in multiple functions).
 */
void VuoCompilerNodeClass::parseInitFunction(void)
{
	initFunction = parser->getFunction(nameForGlobal("nodeInstanceInit"));
	if (! initFunction)
		return;

	parseParameters(initFunction,
				   INPUT_DATA_ABSENT | INPUT_DATA_PRESENT |
				   OUTPUT_DATA_ABSENT |
				   INPUT_EVENT_ABSENT |
				   OUTPUT_EVENT_ABSENT |
				   OUTPUT_TRIGGER_ABSENT |
				   INSTANCE_DATA_ABSENT);
}

/**
 * Parses the fini function from the LLVM module.
 */
void VuoCompilerNodeClass::parseFiniFunction(void)
{
	finiFunction = parser->getFunction(nameForGlobal("nodeInstanceFini"));
}

/**
 * Parses the @c nodeInstanceTriggerStart() function (if any) from the LLVM module.
 *
 * If any parameters are trigger ports that were not also in the event function,
 * appends them to this node class's list of output ports.
 *
 * Assumes this function is called after @c parseEventFunction() and @c parseInitFunction(),
 * and before @c parseCallbackUpdateFunction() (for handling parameters that appear in multiple functions).
 */
void VuoCompilerNodeClass::parseCallbackStartFunction(void)
{
	callbackStartFunction = parser->getFunction(nameForGlobal("nodeInstanceTriggerStart"));
	if (! callbackStartFunction)
		return;

	parseParameters(callbackStartFunction,
				   INPUT_DATA_ABSENT | INPUT_DATA_PRESENT |
				   OUTPUT_DATA_ABSENT |
				   INPUT_EVENT_ABSENT |
				   OUTPUT_EVENT_ABSENT |
				   OUTPUT_TRIGGER_ABSENT | OUTPUT_TRIGGER_PRESENT |
				   INSTANCE_DATA_PRESENT);
}

/**
 * Parses the @c nodeInstanceTriggerUpdate() function (if any) from the LLVM module.
 *
 * If any parameters are trigger ports that were not also in the event function,
 * appends them to this node class's list of output ports.
 *
 * Assumes this function is called after @c parseEventFunction(), @c parseInitFunction,
 * and @c parseCallbackStartFunction() (for handling parameters that appear in multiple functions).
 */
void VuoCompilerNodeClass::parseCallbackUpdateFunction(void)
{
	callbackUpdateFunction = parser->getFunction(nameForGlobal("nodeInstanceTriggerUpdate"));
	if (! callbackUpdateFunction)
		return;

	parseParameters(callbackUpdateFunction,
					INPUT_DATA_ABSENT | INPUT_DATA_PRESENT |
					OUTPUT_DATA_ABSENT |
					INPUT_EVENT_ABSENT |
					OUTPUT_EVENT_ABSENT |
					OUTPUT_TRIGGER_ABSENT | OUTPUT_TRIGGER_PRESENT |
					INSTANCE_DATA_PRESENT);
}

/**
 * Parses the @c nodeInstanceTriggerStop() function (if any) from the LLVM module.
 */
void VuoCompilerNodeClass::parseCallbackStopFunction(void)
{
	callbackStopFunction = parser->getFunction(nameForGlobal("nodeInstanceTriggerStop"));
	if (! callbackStopFunction)
		return;

	parseParameters(callbackStopFunction,
					INPUT_DATA_ABSENT |
					OUTPUT_DATA_ABSENT |
					INPUT_EVENT_ABSENT |
					OUTPUT_EVENT_ABSENT |
					OUTPUT_TRIGGER_ABSENT | OUTPUT_TRIGGER_PRESENT |
					INSTANCE_DATA_PRESENT);
}

/**
 * Parses the arguments of a node class API function. Any new port classes (not parsed in previous
 * calls to this function) are appended to this node class's list of port classes.
 *
 * @param function The function whose arguments will be parsed.
 * @param acceptanceFlags A bitwise-or of VuoNodeArgumentAcceptance values, indicating which argument types are required, optional, and disallowed.
 */
void VuoCompilerNodeClass::parseParameters(Function *function, unsigned long acceptanceFlags)
{
	vector<pair<Argument *, string> > annotatedArguments = parser->getAnnotatedArguments(function);

	vector<VuoCompilerNodeArgumentClass *> inputArgumentClasses;
	vector<VuoCompilerNodeArgumentClass *> outputArgumentClasses;
	map<string, VuoType *> vuoTypeForArgumentName;
	map<string, json_object *> detailsForArgumentName;
	map<string, VuoCompilerPortClass *> portClassForArgumentName;
	map<string, VuoCompilerDataClass *> dataClassForArgumentName;

	bool sawInputData = false;
	bool sawOutputData = false;
	bool sawInputEvent = false;
	bool sawOutputEvent = false;
	bool sawOutputTrigger = false;
	bool sawInstanceData = false;

	// Create the node argument class for each annotated argument.
	for (vector<pair<Argument *, string> >::iterator i = annotatedArguments.begin(); i != annotatedArguments.end(); ++i)
	{
		Argument *argument = i->first;
		string annotation = i->second;
		string argumentName = parser->getArgumentNameInSourceCode(argument->getName());

		VuoCompilerNodeArgumentClass *argumentClass = NULL;
		VuoPortClass *existingPortClass = NULL;
		json_object *details = NULL;
		VuoType *type = NULL;

		if ((argumentClass = parseInputDataParameter(annotation, argument)) != NULL)
		{
			existingPortClass = getExistingPortClass(argumentClass, true);
			if (! existingPortClass)
			{
				inputArgumentClasses.push_back(argumentClass);
				dataClassForArgumentName[argumentName] = static_cast<VuoCompilerDataClass *>(argumentClass);
			}

			sawInputData = true;
		}
		else if ((argumentClass = parseOutputDataParameter(annotation, argument)) != NULL)
		{
			existingPortClass = getExistingPortClass(argumentClass, false);
			if (! existingPortClass)
			{
				outputArgumentClasses.push_back(argumentClass);
				dataClassForArgumentName[argumentName] = static_cast<VuoCompilerDataClass *>(argumentClass);
			}

			sawOutputData = true;
		}
		else if ((argumentClass = parseInputEventParameter(annotation, argument)) != NULL)
		{
			existingPortClass = getExistingPortClass(argumentClass, true);
			if (! existingPortClass)
			{
				inputArgumentClasses.push_back(argumentClass);
				portClassForArgumentName[argumentName] = static_cast<VuoCompilerPortClass *>(argumentClass);
			}

			sawInputEvent = true;
		}
		else if ((argumentClass = parseOutputEventParameter(annotation, argument)) != NULL)
		{
			existingPortClass = getExistingPortClass(argumentClass, false);
			if (! existingPortClass)
			{
				outputArgumentClasses.push_back(argumentClass);
				portClassForArgumentName[argumentName] = static_cast<VuoCompilerPortClass *>(argumentClass);
			}

			sawOutputEvent = true;
		}
		else if ((argumentClass = parseTriggerParameter(annotation, argument)) != NULL)
		{
			existingPortClass = getExistingPortClass(argumentClass, false);
			if (! existingPortClass)
			{
				outputArgumentClasses.push_back(argumentClass);
				portClassForArgumentName[argumentName] = static_cast<VuoCompilerPortClass *>(argumentClass);
			}

			sawOutputTrigger = true;
		}
		else if ((argumentClass = parseInstanceDataParameter(annotation, argument)) != NULL)
		{
			if (instanceDataClass)
				existingPortClass = instanceDataClass->getBase();
			else
				instanceDataClass = static_cast<VuoCompilerInstanceDataClass *>(argumentClass);

			sawInstanceData = true;
		}
		else if ((type = parseTypeParameter(annotation)) != NULL)
		{
			vuoTypeForArgumentName[argumentName] = type;
		}
		else if ((details = parseDetailsParameter(annotation)) != NULL)
		{
			detailsForArgumentName[argumentName] = details;
		}

		VuoCompilerNodeArgumentClass *argumentClassInNodeClass = (existingPortClass ? existingPortClass->getCompiler() : argumentClass);
		if (argumentClassInNodeClass)
		{
			size_t argumentIndex = argument->getArgNo();
			if (function == eventFunction)
				argumentClassInNodeClass->setIndexInEventFunction(argumentIndex);
			else if (function == initFunction)
				argumentClassInNodeClass->setIndexInInitFunction(argumentIndex);
			else if (function == callbackStartFunction)
				argumentClassInNodeClass->setIndexInCallbackStartFunction(argumentIndex);
			else if (function == callbackUpdateFunction)
				argumentClassInNodeClass->setIndexInCallbackUpdateFunction(argumentIndex);
			else if (function == callbackStopFunction)
				argumentClassInNodeClass->setIndexInCallbackStopFunction(argumentIndex);
		}

		if (existingPortClass)
			delete argumentClass;
	}

	// Check that all required arguments and no disallowed arguments are present.
	{
		string functionName = function->getName().str();
		string wronglyAbsentMessage = " is required in " + functionName;
		string wronglyPresentMessage = " is not allowed in " + functionName;

		if (sawInputData && ! (acceptanceFlags & INPUT_DATA_PRESENT))
			VUserLog("Error: %s", ("VuoInputData" + wronglyPresentMessage).c_str());
		if (sawOutputData && ! (acceptanceFlags & OUTPUT_DATA_PRESENT))
			VUserLog("Error: %s", ("VuoOutputData" + wronglyPresentMessage).c_str());
		if (sawInputEvent && ! (acceptanceFlags & INPUT_EVENT_PRESENT))
			VUserLog("Error: %s", ("VuoInputEvent" + wronglyPresentMessage).c_str());
		if (sawOutputEvent && ! (acceptanceFlags & OUTPUT_EVENT_PRESENT))
			VUserLog("Error: %s", ("VuoOutputEvent" + wronglyPresentMessage).c_str());
		if (sawOutputTrigger && ! (acceptanceFlags & OUTPUT_TRIGGER_PRESENT))
			VUserLog("Error: %s", ("VuoOutputTrigger" + wronglyPresentMessage).c_str());
		if (sawInstanceData && ! (acceptanceFlags & INSTANCE_DATA_PRESENT))
			VUserLog("Error: %s", ("VuoInstanceData" + wronglyPresentMessage).c_str());

		if (! sawInputData && ! (acceptanceFlags & INPUT_DATA_ABSENT))
			VUserLog("Error: %s", ("VuoInputData" + wronglyAbsentMessage).c_str());
		if (! sawOutputData && ! (acceptanceFlags & OUTPUT_DATA_ABSENT))
			VUserLog("Error: %s", ("VuoOutputData" + wronglyAbsentMessage).c_str());
		if (! sawInputEvent && ! (acceptanceFlags & INPUT_EVENT_ABSENT))
			VUserLog("Error: %s", ("VuoInputEvent" + wronglyAbsentMessage).c_str());
		if (! sawOutputEvent && ! (acceptanceFlags & OUTPUT_EVENT_ABSENT))
			VUserLog("Error: %s", ("VuoOutputEvent" + wronglyAbsentMessage).c_str());
		if (! sawOutputTrigger && ! (acceptanceFlags & OUTPUT_TRIGGER_ABSENT))
			VUserLog("Error: %s", ("VuoOutputTrigger" + wronglyAbsentMessage).c_str());
		if (! sawInstanceData && ! (acceptanceFlags & INSTANCE_DATA_ABSENT))
			VUserLog("Error: %s", ("VuoInstanceData" + wronglyAbsentMessage).c_str());
	}

	// For each event portion of a data-and-event port, find the corresponding data portion. Rename the event portion to match.
	map<string, VuoCompilerInputEventPortClass *> inputEventPortClassForDataClassName;
	for (map<string, VuoCompilerPortClass *>::iterator i = portClassForArgumentName.begin(); i != portClassForArgumentName.end(); ++i)
	{
		string argumentName = i->first;
		VuoCompilerInputEventPortClass *eventPortClass = dynamic_cast<VuoCompilerInputEventPortClass *>( i->second );

		if (eventPortClass)
		{
			bool isDataInDetails = false;
			string dataPortName = parseString(detailsForArgumentName[argumentName], "data", &isDataInDetails);
			if (isDataInDetails)
			{
				eventPortClass->getBase()->setName(dataPortName);
				inputEventPortClassForDataClassName[dataPortName] = eventPortClass;
			}
		}
	}
	map<string, VuoCompilerOutputEventPortClass *> outputEventPortClassForDataClassName;
	for (map<string, VuoCompilerPortClass *>::iterator i = portClassForArgumentName.begin(); i != portClassForArgumentName.end(); ++i)
	{
		string argumentName = i->first;
		VuoCompilerOutputEventPortClass *eventPortClass = dynamic_cast<VuoCompilerOutputEventPortClass *>( i->second );

		if (eventPortClass)
		{
			bool isDataInDetails = false;
			string dataPortName = parseString(detailsForArgumentName[argumentName], "data", &isDataInDetails);
			if (isDataInDetails)
			{
				eventPortClass->getBase()->setName(dataPortName);
				outputEventPortClassForDataClassName[dataPortName] = eventPortClass;
			}
		}
	}

	// Match up data classes with event port classes. Add event and trigger port classes to addedInputPortClasses/addedOutputPortClasses.
	vector<VuoPortClass *> addedInputPortClasses;
	for (vector<VuoCompilerNodeArgumentClass *>::iterator i = inputArgumentClasses.begin(); i != inputArgumentClasses.end(); ++i)
	{
		VuoCompilerInputDataClass *dataClass = dynamic_cast<VuoCompilerInputDataClass *>(*i);
		if (dataClass)
		{
			VuoCompilerInputEventPortClass *eventPortClass = inputEventPortClassForDataClassName[dataClass->getBase()->getName()];

			if (! eventPortClass)
			{
				eventPortClass = new VuoCompilerInputEventPortClass(dataClass->getBase()->getName());
				eventPortClass->setDataClass(dataClass);
				addedInputPortClasses.push_back(eventPortClass->getBase());
			}
			else
			{
				// Since setDataClass() changes the base, we need to remove the old base and add the new one.
				vector<VuoPortClass *>::iterator oldBase = find(addedInputPortClasses.begin(), addedInputPortClasses.end(), eventPortClass->getBase());
				eventPortClass->setDataClass(dataClass);
				if (oldBase != addedInputPortClasses.end())
				{
					addedInputPortClasses.insert(oldBase,eventPortClass->getBase());
					addedInputPortClasses.erase(oldBase);
				}
			}
		}
		else
			addedInputPortClasses.push_back(((VuoCompilerPortClass *)(*i))->getBase());
	}
	vector<VuoPortClass *> addedOutputPortClasses;
	for (vector<VuoCompilerNodeArgumentClass *>::iterator i = outputArgumentClasses.begin(); i != outputArgumentClasses.end(); ++i)
	{
		VuoCompilerOutputDataClass *dataClass = dynamic_cast<VuoCompilerOutputDataClass *>(*i);
		if (dataClass)
		{
			VuoCompilerOutputEventPortClass *eventPortClass = outputEventPortClassForDataClassName[dataClass->getBase()->getName()];
			if (! eventPortClass)
			{
				eventPortClass = new VuoCompilerOutputEventPortClass(dataClass->getBase()->getName());
				eventPortClass->setDataClass(dataClass);
				addedOutputPortClasses.push_back(eventPortClass->getBase());
			}
			else
			{
				// Since setDataClass() changes the base, we need to remove the old base and add the new one.
				vector<VuoPortClass *>::iterator oldBase = find(addedOutputPortClasses.begin(), addedOutputPortClasses.end(), eventPortClass->getBase());
				eventPortClass->setDataClass(dataClass);
				if (oldBase != addedOutputPortClasses.end())
				{
					addedOutputPortClasses.insert(oldBase,eventPortClass->getBase());
					addedOutputPortClasses.erase(oldBase);
				}
			}
		}
		else
			addedOutputPortClasses.push_back(((VuoCompilerPortClass *)(*i))->getBase());
	}

	// Set the refresh port class if it hasn't been already (from a previous function).
	if (! getBase()->getRefreshPortClass()->hasCompiler())
	{
		VuoPortClass *refreshPortClass = (new VuoCompilerInputEventPortClass("refresh"))->getBase();

		// Remove the existing dummy refresh port class.
		vector<VuoPortClass *> inputPortClasses = getBase()->getInputPortClasses();
		inputPortClasses.erase(inputPortClasses.begin());
		getBase()->setInputPortClasses(inputPortClasses);

		// Set the new refresh port class.
		getBase()->setRefreshPortClass(refreshPortClass);
		addedInputPortClasses.insert(addedInputPortClasses.begin(), refreshPortClass);
	}

	// Append the port classes defined in this function to the node class's list of port classes.
	vector<VuoPortClass *> inputPortClasses = getBase()->getInputPortClasses();
	inputPortClasses.insert(inputPortClasses.end(), addedInputPortClasses.begin(), addedInputPortClasses.end());
	getBase()->setInputPortClasses(inputPortClasses);
	vector<VuoPortClass *> outputPortClasses = getBase()->getOutputPortClasses();
	outputPortClasses.insert(outputPortClasses.end(), addedOutputPortClasses.begin(), addedOutputPortClasses.end());
	getBase()->setOutputPortClasses(outputPortClasses);

	// Set the VuoType for each added port.
	vector<VuoPortClass *> portClasses;
	portClasses.insert(portClasses.end(), addedInputPortClasses.begin(), addedInputPortClasses.end());
	portClasses.insert(portClasses.end(), addedOutputPortClasses.begin(), addedOutputPortClasses.end());
	for (vector<VuoPortClass *>::iterator i = portClasses.begin(); i != portClasses.end(); ++i)
	{
		VuoCompilerEventPortClass *eventPortClass = dynamic_cast<VuoCompilerEventPortClass *>((*i)->getCompiler());
		if (eventPortClass)
		{
			VuoCompilerDataClass *dataClass = eventPortClass->getDataClass();
			if (dataClass)
			{
				string dataClassName = dataClass->getBase()->getName();
				map<string, VuoType *>::iterator vuoTypeIter = vuoTypeForArgumentName.find(dataClassName);
				if (vuoTypeIter != vuoTypeForArgumentName.end())
				{
					dataClass->setVuoType(vuoTypeIter->second);
					vuoTypeForArgumentName.erase(vuoTypeIter);
				}
			}
		}

		VuoCompilerTriggerPortClass *triggerPortClass = dynamic_cast<VuoCompilerTriggerPortClass *>((*i)->getCompiler());
		if (triggerPortClass)
		{
			string triggerName = triggerPortClass->getBase()->getName();
			map<string, VuoType *>::iterator vuoTypeIter = vuoTypeForArgumentName.find(triggerName);
			if (vuoTypeIter != vuoTypeForArgumentName.end())
			{
				triggerPortClass->setDataVuoType(vuoTypeIter->second);
				vuoTypeForArgumentName.erase(vuoTypeIter);
			}
		}
	}
	for (map<string, VuoType *>::iterator i = vuoTypeForArgumentName.begin(); i != vuoTypeForArgumentName.end(); ++i)
		delete i->second;

	// Set the details for each added port.
	for (map<string, json_object *>::iterator i = detailsForArgumentName.begin(); i != detailsForArgumentName.end(); ++i)
	{
		string argumentName = i->first;
		json_object *details = i->second;

		VuoCompilerPortClass *portClass = portClassForArgumentName[argumentName];
		if (portClass)
			portClass->setDetails(details);
		else
		{
			VuoCompilerDataClass *dataClass = dataClassForArgumentName[argumentName];
			if (dataClass)
				dataClass->setDetails(details);
		}
	}

	// Set the event-blocking behavior for each added input port.
	for (vector<VuoPortClass *>::iterator i = addedInputPortClasses.begin(); i != addedInputPortClasses.end(); ++i)
	{
		VuoCompilerInputEventPortClass *eventPortClass = dynamic_cast<VuoCompilerInputEventPortClass *>( (*i)->getCompiler() );
		bool isEventBlockingInDetails = false;
		string eventBlockingStr = parseString(eventPortClass->getDetails(), "eventBlocking", &isEventBlockingInDetails);
		if (isEventBlockingInDetails)
		{
			VuoPortClass::EventBlocking eventBlocking;
			if (eventBlockingStr == "none")
				eventBlocking = VuoPortClass::EventBlocking_None;
			else if (eventBlockingStr == "door")
				eventBlocking = VuoPortClass::EventBlocking_Door;
			else if (eventBlockingStr == "wall")
				eventBlocking = VuoPortClass::EventBlocking_Wall;
			else
			{
				VUserLog("Error: Unknown option for \"eventBlocking\": %s\n", eventBlockingStr.c_str());
				continue;
			}
			eventPortClass->getBase()->setEventBlocking(eventBlocking);

			if (eventBlocking == VuoPortClass::EventBlocking_None)
				portsWithExplicitEventBlockingNone.insert(eventPortClass);
		}
	}

	// Set the event-throttling behavior for each added trigger port.
	for (vector<VuoPortClass *>::iterator i = addedOutputPortClasses.begin(); i != addedOutputPortClasses.end(); ++i)
	{
		VuoCompilerTriggerPortClass *triggerPortClass = dynamic_cast<VuoCompilerTriggerPortClass *>( (*i)->getCompiler() );
		if (triggerPortClass)
		{
			bool isEventThrottlingInDetails = false;
			string eventThrottlingStr = parseString(triggerPortClass->getDetails(), "eventThrottling", &isEventThrottlingInDetails);
			if (isEventThrottlingInDetails)
			{
				VuoPortClass::EventThrottling eventThrottling;
				if (eventThrottlingStr == "enqueue")
					eventThrottling = VuoPortClass::EventThrottling_Enqueue;
				else if (eventThrottlingStr == "drop")
					eventThrottling = VuoPortClass::EventThrottling_Drop;
				else
				{
					VUserLog("Error: Unknown option for \"throttling\": %s\n", eventThrottlingStr.c_str());
					continue;
				}
				triggerPortClass->getBase()->setDefaultEventThrottling(eventThrottling);
			}
		}
	}

	// Update the port action status for each input port.
	for (vector<VuoPortClass *>::iterator i = inputPortClasses.begin(); i != inputPortClasses.end(); ++i)
	{
		VuoPortClass *portClass = *i;
		VuoCompilerInputEventPortClass *eventPortClass = static_cast<VuoCompilerInputEventPortClass *>((*i)->getCompiler());
		if (eventPortClass->getBase() != getBase()->getRefreshPortClass())
		{
			bool isPortActionInDetails = false;
			bool hasPortAction = parseBool(eventPortClass->getDetails(), "hasPortAction", &isPortActionInDetails);

			if (isPortActionInDetails)
				portClass->setPortAction(hasPortAction);
			else if (! eventPortClass->getDataClass() ||
					 portsWithExplicitEventBlockingNone.find(eventPortClass) != portsWithExplicitEventBlockingNone.end())
				portClass->setPortAction(true);
		}
	}
}

/**
 * Parses a "vuoInputData" annotated function parameter. Returns null if not a "vuoInputData".
 */
VuoCompilerInputDataClass * VuoCompilerNodeClass::parseInputDataParameter(string annotation, Argument *a)
{
	if (annotation != "vuoInputData")
		return NULL;

	string argumentName = parser->getArgumentNameInSourceCode(a->getName());
	bool isLoweredToTwoParameters = parser->isFirstOfTwoLoweredArguments(a);
	VuoCompilerInputDataClass *dataClass = new VuoCompilerInputDataClass(argumentName,
																		 a->getType(),
																		 isLoweredToTwoParameters);
	return dataClass;
}

/**
 * Parses a "vuoOutputData" annotated function parameter. Returns null if not a "vuoOutputData".
 */
VuoCompilerOutputDataClass * VuoCompilerNodeClass::parseOutputDataParameter(string annotation, Argument *a)
{
	if (annotation != "vuoOutputData")
		return NULL;

	string argumentName = parser->getArgumentNameInSourceCode(a->getName());
	if (! a->getType()->isPointerTy())
	{
		VUserLog("Error: Output port data %s must be a pointer.\n", argumentName.c_str());
		return NULL;
	}

	VuoCompilerOutputDataClass *dataClass = new VuoCompilerOutputDataClass(argumentName,
																		   ((PointerType *)a->getType())->getElementType());
	return dataClass;
}

/**
 * Parses a "vuoInputEvent" annotated function parameter. Returns null if not a "vuoInputEvent".
 */
VuoCompilerInputEventPortClass * VuoCompilerNodeClass::parseInputEventParameter(string annotation, Argument *a)
{
	if (! VuoStringUtilities::beginsWith(annotation, "vuoInputEvent"))
		return NULL;

	string argumentName = parser->getArgumentNameInSourceCode(a->getName());

	VuoCompilerInputEventPortClass *portClass = new VuoCompilerInputEventPortClass(argumentName,
																				   a->getType());
	return portClass;
}

/**
 * Parses a "vuoOutputEvent" annotated function parameter. Returns null if not a "vuoOutputEvent".
 */
VuoCompilerOutputEventPortClass * VuoCompilerNodeClass::parseOutputEventParameter(string annotation, Argument *a)
{
	if (! VuoStringUtilities::beginsWith(annotation, "vuoOutputEvent"))
		return NULL;

	string argumentName = parser->getArgumentNameInSourceCode(a->getName());
	if (! a->getType()->isPointerTy())
	{
		VUserLog("Error: Output port %s must be a pointer.\n", argumentName.c_str());
		return NULL;
	}

	VuoCompilerOutputEventPortClass *portClass = new VuoCompilerOutputEventPortClass(argumentName,
																					 ((PointerType *)a->getType())->getElementType());
	return portClass;
}

/**
 * Parses a "vuoOutputTrigger" annotated function parameter. Returns null if not a "vuoOutputTrigger".
 */
VuoCompilerTriggerPortClass * VuoCompilerNodeClass::parseTriggerParameter(string annotation, Argument *a)
{
	if (! VuoStringUtilities::beginsWith(annotation, "vuoOutputTrigger:"))
		return NULL;

	string argumentName = parser->getArgumentNameInSourceCode(a->getName());
	if (! a->getType()->isPointerTy())
	{
		VUserLog("Error: Output trigger %s must be a pointer.\n", argumentName.c_str());
		return NULL;
	}

	VuoCompilerTriggerPortClass *portClass = new VuoCompilerTriggerPortClass(argumentName,
																				 (PointerType *)a->getType());
	return portClass;
}

/**
 * Parses a "vuoInstanceData" annotated function parameter. Returns null if not a "vuoInstanceData".
 */
VuoCompilerInstanceDataClass * VuoCompilerNodeClass::parseInstanceDataParameter(string annotation, Argument *a)
{
	if (annotation != "vuoInstanceData")
		return NULL;

	string argumentName = parser->getArgumentNameInSourceCode(a->getName());
	if (! a->getType()->isPointerTy())
	{
		VUserLog("Error: Node instance data %s must be a pointer.\n", argumentName.c_str());
		return NULL;
	}

	VuoCompilerInstanceDataClass *instanceDataClass = new VuoCompilerInstanceDataClass(argumentName,
																					   ((PointerType *)a->getType())->getElementType());
	return instanceDataClass;
}

/**
 * Parses a "vuoType" annotated function parameter. Returns null if not a "vuoType".
 */
VuoType * VuoCompilerNodeClass::parseTypeParameter(string annotation)
{
	if (! VuoStringUtilities::beginsWith(annotation, "vuoType:"))
		return NULL;

	string typeName = VuoStringUtilities::substrAfter(annotation, "vuoType:");

	VuoType *type;
	if (typeName == "void")
	{
		type = NULL;
	}
	else if (VuoGenericType::isGenericTypeName(typeName))
	{
		string innermostTypeName = VuoType::extractInnermostTypeName(typeName);
		vector<string> compatibleTypes;
		map<string, vector<string> >::iterator compatibleTypesIter = compatibleSpecializedForGenericTypeName.find(innermostTypeName);
		if (compatibleTypesIter != compatibleSpecializedForGenericTypeName.end())
		{
			string prefix = (VuoType::isListTypeName(typeName) ? VuoType::listTypeNamePrefix : "");
			vector<string> innermostCompatibleTypes = compatibleTypesIter->second;
			for (vector<string>::iterator i = innermostCompatibleTypes.begin(); i != innermostCompatibleTypes.end(); ++i)
				compatibleTypes.push_back(prefix + *i);
		}

		type = new VuoGenericType(typeName, compatibleTypes);
	}
	else
	{
		type = new VuoType(typeName);
	}
	return type;
}

/**
 * Parses a "vuoDetails" annotated function parameter. Returns null if not a "vuoDetails".
 */
json_object * VuoCompilerNodeClass::parseDetailsParameter(string annotation)
{
	if (! VuoStringUtilities::beginsWith(annotation, "vuoDetails:"))
		return NULL;

	json_object *detailsObj = NULL;
	string details = VuoStringUtilities::substrAfter(annotation, "vuoDetails:");
	if (details.find_first_not_of(' ') != string::npos)
	{
		detailsObj = json_tokener_parse(details.c_str());
		if (! detailsObj)
			VUserLog("Error: Couldn't parse vuoDetails for `%s`: %s\n", getBase()->getClassName().c_str(), details.c_str());
	}
	return detailsObj;
}

/**
 * If a port class with the same name as @a argumentClass already exists in this node class, returns that port class.
 * Otherwise, returns null.
 */
VuoPortClass * VuoCompilerNodeClass::getExistingPortClass(VuoCompilerNodeArgumentClass *argumentClass, bool isInput)
{
	string argumentName = argumentClass->getBase()->getName();
	VuoPortClass *existingInputPortClass = getInputPortClassWithName(argumentName);
	VuoPortClass *existingOutputPortClass = getOutputPortClassWithName(argumentName);
	if (existingInputPortClass || existingOutputPortClass)
	{
		if (! isInput && getBase()->getClassName() == VuoNodeClass::publishedInputNodeClassName)
		{
			if (! existingOutputPortClass)
				return NULL;
		}
		else if ((isInput && existingOutputPortClass) || (! isInput && existingInputPortClass))
		{
			VUserLog("Error: Port %s is declared as an input port in one function and an output port in another function.\n", argumentName.c_str());
			return NULL;
		}

		VuoPortClass *existingPortClass = (existingInputPortClass ? existingInputPortClass : existingOutputPortClass);
		if (dynamic_cast<VuoCompilerDataClass *>(argumentClass))
			existingPortClass = dynamic_cast<VuoCompilerEventPortClass *>(existingPortClass->getCompiler())->getDataClass()->getBase();

		return existingPortClass;
	}
	else
	{
		return NULL;
	}
}

/**
 * The unique class name for this node class, rendered as an identifier in the generated code.
 *
 * Possible characters: @c [A-Za-z0-9_]
 *
 * @eg{vuo_math_lessThan_i64}
 */
string VuoCompilerNodeClass::getClassIdentifier(void)
{
	return VuoStringUtilities::transcodeToIdentifier(getBase()->getClassName());
}

/**
 * Returns an LLVM Function for this node class's implementation of the @c #nodeEvent or @c #nodeInstanceEvent function.
 */
Function * VuoCompilerNodeClass::getEventFunction(void)
{
	return eventFunction;
}

/**
 * If this node class is stateful, returns an LLVM Function for this node class's implementation of the @c #nodeInstanceInit function.  Otherwise null.
 */
Function * VuoCompilerNodeClass::getInitFunction(void)
{
	return initFunction;
}

/**
 * If this node class is stateful, returns an LLVM Function for this node class's implementation of the @c #nodeInstanceFini function.  Otherwise null.
 */
Function * VuoCompilerNodeClass::getFiniFunction(void)
{
	return finiFunction;
}

/**
 * If this node class is stateful, returns an LLVM Function for this node class's implementation of the @c #nodeInstanceTriggerStart function.  Otherwise null.
 */
Function * VuoCompilerNodeClass::getCallbackStartFunction(void)
{
	return callbackStartFunction;
}

/**
 * If this node class is stateful, returns an LLVM Function for this node class's implementation of the @c #nodeInstanceTriggerUpdate function.  Otherwise null.
 */
Function * VuoCompilerNodeClass::getCallbackUpdateFunction(void)
{
	return callbackUpdateFunction;
}

/**
 * If this node class is stateful, returns an LLVM Function for this node class's implementation of the @c #nodeInstanceTriggerStop function.  Otherwise null.
 */
Function * VuoCompilerNodeClass::getCallbackStopFunction(void)
{
	return callbackStopFunction;
}

/**
 * If this node class is a subcomposition, returns an LLVM Function for this subcomposition's implementation of the @c compositionContextInit function.  Otherwise null.
 */
Function * VuoCompilerNodeClass::getCompositionContextInitFunction(void)
{
	return parser->getFunction(nameForGlobal("compositionContextInit"));
}

/**
 * If this node class is a subcomposition, returns an LLVM Function for this subcomposition's implementation of the @c compositionContextFini function.  Otherwise null.
 */
Function * VuoCompilerNodeClass::getCompositionContextFiniFunction(void)
{
	return parser->getFunction(nameForGlobal("compositionContextFini"));
}

/**
 * If this node class is a subcomposition, returns an LLVM Function for this subcomposition's implementation of the @c compositionSerialize function.  Otherwise null.
 */
Function * VuoCompilerNodeClass::getCompositionSerializeFunction(void)
{
	return parser->getFunction(nameForGlobal("compositionSerialize"));
}

/**
 * If this node class is a subcomposition, returns an LLVM Function for this subcomposition's implementation of the @c compositionUnserialize function.  Otherwise null.
 */
Function * VuoCompilerNodeClass::getCompositionUnserializeFunction(void)
{
	return parser->getFunction(nameForGlobal("compositionUnserialize"));
}

/**
 * If this node class is a subcomposition, returns this subcomposition's implementation of the trigger worker function for the trigger with identifier @a portIdentifier.
 */
Function * VuoCompilerNodeClass::getTriggerWorkerFunction(string portIdentifier)
{
	return parser->getFunction(nameForGlobal(portIdentifier));
}

/**
 * If this node is a subcomposition, returns information about the triggers internal to the subcomposition, parsed from the subcomposition's metadata.
 */
vector<VuoCompilerTriggerDescription *> VuoCompilerNodeClass::getTriggerDescriptions(void)
{
	return triggerDescriptions;
}

/**
 * Returns the input port class matching the specified @c portName, if one exists.  Otherwise null.
 */
VuoPortClass * VuoCompilerNodeClass::getInputPortClassWithName(string portName)
{
	vector<VuoPortClass *> inputPortClasses = getBase()->getInputPortClasses();
	for (vector<VuoPortClass *>::iterator i = inputPortClasses.begin(); i != inputPortClasses.end(); ++i)
		if ((*i)->getName() == portName)
			return *i;

	return NULL;
}

/**
 * Returns the output port class matching the specified @c portName, if one exists.  Otherwise null.
 */
VuoPortClass * VuoCompilerNodeClass::getOutputPortClassWithName(string portName)
{
	vector<VuoPortClass *> outputPortClasses = getBase()->getOutputPortClasses();
	for (vector<VuoPortClass *>::iterator i = outputPortClasses.begin(); i != outputPortClasses.end(); ++i)
		if ((*i)->getName() == portName)
			return *i;

	return NULL;
}

/**
 * If this node class is stateful, returns the instance data class.  Otherwise null.
 */
VuoCompilerInstanceDataClass * VuoCompilerNodeClass::getInstanceDataClass(void)
{
	return instanceDataClass;
}

/**
 * Returns a string containing documentation for this node class in Doxygen format.
 */
string VuoCompilerNodeClass::getDoxygenDocumentation(void)
{
	ostringstream documentation;

	documentation << "/**" << endl;
	documentation << " * " << getBase()->getDescription() << endl;
	documentation << " * " << endl;

	vector<VuoPortClass *> inputPortClasses = getBase()->getInputPortClasses();
	vector<VuoPortClass *> outputPortClasses = getBase()->getOutputPortClasses();

	vector< pair<string, VuoPortClass *> > portClasses;
	for (vector<VuoPortClass *>::iterator i = inputPortClasses.begin(); i != inputPortClasses.end(); ++i)
	{
		portClasses.push_back( make_pair("in", *i) );
	}
	for (vector<VuoPortClass *>::iterator i = outputPortClasses.begin(); i != outputPortClasses.end(); ++i)
	{
		bool isTrigger = dynamic_cast<VuoCompilerTriggerPortClass *>((*i)->getCompiler());
		portClasses.push_back( make_pair(isTrigger ? "gen" : "out", *i) );
	}

	for (vector< pair<string, VuoPortClass *> >::iterator i = portClasses.begin(); i != portClasses.end(); ++i)
	{
		string portKind = i->first;
		VuoPortClass *portClass = i->second;
		VuoCompilerPortClass *compilerPortClass = static_cast<VuoCompilerPortClass *>(portClass->getCompiler());
		VuoType *portType = compilerPortClass->getDataVuoType();
		string portTypeName = (portType ? portType->getModuleKey() : "event");
		string portClassName = portClass->getName();
		documentation << " * @param[" << portKind << "] " << portTypeName << " " << portClassName << endl;
	}

	documentation << " */";

	return documentation.str();
}

/**
 * If this node class is generic, returns the default specialized types to replace the generic type with
 * when creating an instance.
 */
string VuoCompilerNodeClass::getDefaultSpecializedTypeName(string genericTypeName)
{
	map<string, string>::iterator typeNameIter = defaultSpecializedForGenericTypeName.find(genericTypeName);
	if (typeNameIter != defaultSpecializedForGenericTypeName.end())
		return typeNameIter->second;

	return "";
}

/**
 * Returns a list of keywords automatically associated with this node class,
 * based on attributes such as its port classes and premium status.
 */
vector<string> VuoCompilerNodeClass::getAutomaticKeywords(void)
{
	vector<string> keywords;

	// Automatically add trigger-related keywords for nodes containing trigger ports.
	bool nodeHasTriggerPort = false;
	vector<VuoPortClass *> outputPortClasses = getBase()->getOutputPortClasses();
	for (vector<VuoPortClass *>::iterator i = outputPortClasses.begin(); i != outputPortClasses.end(); ++i)
	{
		if ((*i)->getPortType() == VuoPortClass::triggerPort)
		{
			nodeHasTriggerPort = true;
			break;
		}
	}

	if (nodeHasTriggerPort)
	{
		keywords.push_back("bang");
		keywords.push_back("events");
		keywords.push_back("trigger");
		keywords.push_back("fire");
	}

	bool nodeTitleBeginsWithSend = VuoStringUtilities::beginsWith(getBase()->getDefaultTitle(), "Send");
	bool nodeTitleBeginsWithReceive = VuoStringUtilities::beginsWith(getBase()->getDefaultTitle(), "Receive");

	if (nodeTitleBeginsWithSend || nodeTitleBeginsWithReceive)
	{
		keywords.push_back("i/o");
		keywords.push_back("interface");

		if (nodeTitleBeginsWithSend)
		{
			keywords.push_back("output");
			keywords.push_back("consumer");
		}

		if (nodeTitleBeginsWithReceive)
		{
			keywords.push_back("input");
			keywords.push_back("provider");
		}
	}

	if (VuoStringUtilities::beginsWith(getBase()->getClassName(), "vuo.type."))
		keywords.push_back("conversion");

	if (getPremium())
		keywords.push_back("premium");

	return keywords;
}

/**
 * Returns true if this node class is stateful (implements `nodeInstance*` functions instead of `nodeEvent`).
 */
bool VuoCompilerNodeClass::isStateful(void)
{
	return (instanceDataClass != NULL);
}

/**
 * Returns true if this node class is a subcomposition (implemented in Vuo language, as opposed to text code).
 */
bool VuoCompilerNodeClass::isSubcomposition(void)
{
	return parser && (getCompositionContextInitFunction() != NULL);
}

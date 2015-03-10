/**
 * @file
 * VuoCompilerNodeClass implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <sstream>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <json/json.h>
#pragma clang diagnostic pop

#include "VuoCompiler.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerNodeClass.hh"
#include "VuoCompilerSpecializedNodeClass.hh"

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
 * Creates a new compiler node class and creates a new base @c VuoNodeClass, both from @c compilerNodeClass.
 */
VuoCompilerNodeClass::VuoCompilerNodeClass(VuoCompilerNodeClass *compilerNodeClass)
	: VuoBaseDetail<VuoNodeClass>("VuoCompilerNodeClass with substantial base", new VuoNodeClass(
		compilerNodeClass->getBase()->getClassName(),
		compilerNodeClass->getBase()->getRefreshPortClass(),
		compilerNodeClass->getBase()->getDonePortClass(),
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
	getBase()->setCompiler(this);

	this->dependencies = compilerNodeClass->dependencies;
	this->eventFunction = compilerNodeClass->eventFunction;
	this->initFunction = compilerNodeClass->initFunction;
	this->finiFunction = compilerNodeClass->finiFunction;
	this->callbackStartFunction = compilerNodeClass->callbackStartFunction;
	this->callbackUpdateFunction = compilerNodeClass->callbackUpdateFunction;
	this->callbackStopFunction = compilerNodeClass->callbackStopFunction;
	this->instanceDataClass = compilerNodeClass->instanceDataClass;
	this->defaultSpecializedForGenericTypeName = compilerNodeClass->defaultSpecializedForGenericTypeName;
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
 * Creates a substantial base node instance from this node class.
 */
VuoNode * VuoCompilerNodeClass::newNode(string title, double x, double y)
{
	if (title.empty())
		title = getBase()->getDefaultTitle();

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
											 map<string, set<string> > &compatibleSpecializedForGenericTypeName)
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
				compatibleSpecializedForGenericTypeName[genericTypeName].insert(compatibleTypes.begin(), compatibleTypes.end());
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
			fprintf(stderr, "Missing function nodeEvent or nodeInstanceEvent\n");
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
	map<string, VuoCompilerInputEventPortClass *> inputEventPortClassForDataClassName;
	map<string, VuoCompilerOutputEventPortClass *> outputEventPortClassForDataClassName;
	map<string, json_object *> detailsForInputDataClassName;
	map<string, VuoType *> vuoTypeForArgumentName;
	map<string, enum VuoPortClass::EventBlocking> eventBlockingForArgumentName;
	map<string, VuoCompilerInputEventPortClass *> inputEventPortClassForArgumentName;

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
		string dataPortName;
		json_object *inputDataDetails = NULL;
		VuoType *type = NULL;
		int eventBlocking;

		if ((argumentClass = parseInputDataParameter(annotation, argument)) != NULL)
		{
			existingPortClass = getExistingPortClass(argumentClass, true);
			if (! existingPortClass)
				inputArgumentClasses.push_back(argumentClass);

			sawInputData = true;
		}
		else if ((argumentClass = parseOutputDataParameter(annotation, argument)) != NULL)
		{
			existingPortClass = getExistingPortClass(argumentClass, false);
			if (! existingPortClass)
				outputArgumentClasses.push_back(argumentClass);

			sawOutputData = true;
		}
		else if ((argumentClass = parseInputEventParameter(annotation, argument, dataPortName)) != NULL)
		{
			existingPortClass = getExistingPortClass(argumentClass, true);
			if (! existingPortClass)
			{
				inputArgumentClasses.push_back(argumentClass);

				if (! dataPortName.empty())
					inputEventPortClassForDataClassName[dataPortName] = static_cast<VuoCompilerInputEventPortClass *>(argumentClass);

				inputEventPortClassForArgumentName[argumentName] = static_cast<VuoCompilerInputEventPortClass *>(argumentClass);
			}

			sawInputEvent = true;
		}
		else if ((argumentClass = parseOutputEventParameter(annotation, argument, dataPortName)) != NULL)
		{
			existingPortClass = getExistingPortClass(argumentClass, false);
			if (! existingPortClass)
			{
				outputArgumentClasses.push_back(argumentClass);

				if (! dataPortName.empty())
					outputEventPortClassForDataClassName[dataPortName] = static_cast<VuoCompilerOutputEventPortClass *>(argumentClass);
			}

			sawOutputEvent = true;
		}
		else if ((argumentClass = parseTriggerParameter(annotation, argument)) != NULL)
		{
			existingPortClass = getExistingPortClass(argumentClass, false);
			if (! existingPortClass)
				outputArgumentClasses.push_back(argumentClass);

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
		else if ((inputDataDetails = parseInputDataDetailsParameter(annotation)) != NULL)
		{
			detailsForInputDataClassName[argumentName] = inputDataDetails;
		}
		else if ((eventBlocking = parseEventBlockingParameter(annotation)) != -1)
		{
			eventBlockingForArgumentName[argumentName] = (VuoPortClass::EventBlocking)eventBlocking;
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
		}
	}

	// Check that all required arguments and no disallowed arguments are present.
	{
		string functionName = function->getName().str();
		string wronglyAbsentMessage = " is not allowed in " + functionName + "\n";
		string wronglyPresentMessage = " is required in " + functionName + "\n";

		if (sawInputData && ! (acceptanceFlags & INPUT_DATA_PRESENT))
			fprintf(stderr, "%s", ("VuoInputData" + wronglyPresentMessage).c_str());
		if (sawOutputData && ! (acceptanceFlags & OUTPUT_DATA_PRESENT))
			fprintf(stderr, "%s", ("VuoOutputData" + wronglyPresentMessage).c_str());
		if (sawInputEvent && ! (acceptanceFlags & INPUT_EVENT_PRESENT))
			fprintf(stderr, "%s", ("VuoInputEvent" + wronglyPresentMessage).c_str());
		if (sawOutputEvent && ! (acceptanceFlags & OUTPUT_EVENT_PRESENT))
			fprintf(stderr, "%s", ("VuoOutputEvent" + wronglyPresentMessage).c_str());
		if (sawOutputTrigger && ! (acceptanceFlags & OUTPUT_TRIGGER_PRESENT))
			fprintf(stderr, "%s", ("VuoOutputTrigger" + wronglyPresentMessage).c_str());
		if (sawInstanceData && ! (acceptanceFlags & INSTANCE_DATA_PRESENT))
			fprintf(stderr, "%s", ("VuoInstanceData" + wronglyPresentMessage).c_str());

		if (! sawInputData && ! (acceptanceFlags & INPUT_DATA_ABSENT))
			fprintf(stderr, "%s", ("VuoInputData" + wronglyAbsentMessage).c_str());
		if (! sawOutputData && ! (acceptanceFlags & OUTPUT_DATA_ABSENT))
			fprintf(stderr, "%s", ("VuoOutputData" + wronglyAbsentMessage).c_str());
		if (! sawInputEvent && ! (acceptanceFlags & INPUT_EVENT_ABSENT))
			fprintf(stderr, "%s", ("VuoInputEvent" + wronglyAbsentMessage).c_str());
		if (! sawOutputEvent && ! (acceptanceFlags & OUTPUT_EVENT_ABSENT))
			fprintf(stderr, "%s", ("VuoOutputEvent" + wronglyAbsentMessage).c_str());
		if (! sawOutputTrigger && ! (acceptanceFlags & OUTPUT_TRIGGER_ABSENT))
			fprintf(stderr, "%s", ("VuoOutputTrigger" + wronglyAbsentMessage).c_str());
		if (! sawInstanceData && ! (acceptanceFlags & INSTANCE_DATA_ABSENT))
			fprintf(stderr, "%s", ("VuoInstanceData" + wronglyAbsentMessage).c_str());
	}

	// Match up data port classes with event port classes. Add event and trigger port classes to addedInputPortClasses/addedOutputPortClasses.
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

	// Set the done port class if it hasn't been already (from a previous function).
	if (! getBase()->getDonePortClass()->hasCompiler())
	{
		VuoPortClass *donePortClass = (new VuoCompilerOutputEventPortClass("done"))->getBase();

		// Remove the existing dummy done port class.
		vector<VuoPortClass *> outputPortClasses = getBase()->getOutputPortClasses();
		outputPortClasses.erase(outputPortClasses.begin());
		getBase()->setOutputPortClasses(outputPortClasses);

		// Set the new done port class.
		getBase()->setDonePortClass(donePortClass);
		addedOutputPortClasses.insert(addedOutputPortClasses.begin(), donePortClass);
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
				VuoType *vuoType = vuoTypeForArgumentName[dataClassName];
				dataClass->setVuoType(vuoType);
			}
		}

		VuoCompilerTriggerPortClass *triggerPortClass = dynamic_cast<VuoCompilerTriggerPortClass *>((*i)->getCompiler());
		if (triggerPortClass)
		{
			string triggerName = triggerPortClass->getBase()->getName();
			VuoType *vuoType = vuoTypeForArgumentName[triggerName];
			triggerPortClass->setDataVuoType(vuoType);
		}
	}

	// Set the default value and other details for each added input port.
	for (vector<VuoPortClass *>::iterator i = addedInputPortClasses.begin(); i != addedInputPortClasses.end(); ++i)
	{
		VuoCompilerInputEventPortClass *eventPortClass = static_cast<VuoCompilerInputEventPortClass *>((*i)->getCompiler());
		VuoCompilerInputDataClass *dataClass = eventPortClass->getDataClass();
		if (dataClass)
		{
			string dataClassName = dataClass->getBase()->getName();
			json_object *details = detailsForInputDataClassName[dataClassName];
			dataClass->setDetails(details);
		}
	}

	// Set the event-blocking behavior for each added input port.
	for (map<string, enum VuoPortClass::EventBlocking>::iterator i = eventBlockingForArgumentName.begin(); i != eventBlockingForArgumentName.end(); ++i)
	{
		string argumentName = i->first;
		VuoPortClass::EventBlocking eventBlocking = i->second;
		VuoCompilerInputEventPortClass *eventPortClass = inputEventPortClassForArgumentName[argumentName];
		if (eventPortClass)
			eventPortClass->getBase()->setEventBlocking(eventBlocking);
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
		fprintf(stderr, "Output port data %s must be a pointer.\n", argumentName.c_str());
		return NULL;
	}

	VuoCompilerOutputDataClass *dataClass = new VuoCompilerOutputDataClass(argumentName,
																		   ((PointerType *)a->getType())->getElementType());
	return dataClass;
}

/**
 * Parses a "vuoInputEvent" annotated function parameter. Returns null if not a "vuoInputEvent".
 */
VuoCompilerInputEventPortClass * VuoCompilerNodeClass::parseInputEventParameter(string annotation, Argument *a, string &dataPortName)
{
	if (! VuoStringUtilities::beginsWith(annotation, "vuoInputEvent:"))
		return NULL;

	string argumentName = parser->getArgumentNameInSourceCode(a->getName());
	dataPortName = VuoStringUtilities::substrAfter(annotation, "vuoInputEvent:");
	string eventPortName = dataPortName.empty() ? argumentName : dataPortName;

	VuoCompilerInputEventPortClass *portClass = new VuoCompilerInputEventPortClass(eventPortName,
																				   a->getType());
	return portClass;
}

/**
 * Parses a "vuoOutputEvent" annotated function parameter. Returns null if not a "vuoOutputEvent".
 */
VuoCompilerOutputEventPortClass * VuoCompilerNodeClass::parseOutputEventParameter(string annotation, Argument *a, string &dataPortName)
{
	if (! VuoStringUtilities::beginsWith(annotation, "vuoOutputEvent:"))
		return NULL;

	string argumentName = parser->getArgumentNameInSourceCode(a->getName());
	if (! a->getType()->isPointerTy())
	{
		fprintf(stderr, "Output port %s must be a pointer.\n", argumentName.c_str());
		return NULL;
	}

	dataPortName = VuoStringUtilities::substrAfter(annotation, "vuoOutputEvent:");
	string eventPortName = dataPortName.empty() ? argumentName : dataPortName;

	VuoCompilerOutputEventPortClass *portClass = new VuoCompilerOutputEventPortClass(eventPortName,
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
		fprintf(stderr, "Output trigger %s must be a pointer.\n", argumentName.c_str());
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
		fprintf(stderr, "Node instance data %s must be a pointer.\n", argumentName.c_str());
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
		set<string> compatibleTypes;
		map<string, set<string> >::iterator compatibleTypesIter = compatibleSpecializedForGenericTypeName.find(innermostTypeName);
		if (compatibleTypesIter != compatibleSpecializedForGenericTypeName.end())
		{
			string prefix = (VuoType::isListTypeName(typeName) ? VuoType::listTypeNamePrefix : "");
			set<string> innermostCompatibleTypes = compatibleTypesIter->second;
			for (set<string>::iterator i = innermostCompatibleTypes.begin(); i != innermostCompatibleTypes.end(); ++i)
				compatibleTypes.insert(prefix + *i);
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
 * Parses a "vuoInputDataDetails" annotated function parameter. Returns null if not a "vuoInputDataDetails".
 */
json_object * VuoCompilerNodeClass::parseInputDataDetailsParameter(string annotation)
{
	if (! VuoStringUtilities::beginsWith(annotation, "vuoInputDataDetails:"))
		return NULL;

	json_object *detailsObj = NULL;
	string details = VuoStringUtilities::substrAfter(annotation, "vuoInputDataDetails:");
	if (details.find_first_not_of(' ') != string::npos)
	{
		detailsObj = json_tokener_parse(details.c_str());
		if (! detailsObj)
			fprintf(stderr, "Couldn't parse vuoInputDataDetails: %s\n", details.c_str());
	}
	return detailsObj;
}

/**
 * Parses a "vuoInputEventBlocking" annotated function parameter. Returns -1 if not a "vuoInputEventBlocking".
 */
int VuoCompilerNodeClass::parseEventBlockingParameter(string annotation)
{
	if (! VuoStringUtilities::beginsWith(annotation, "vuoInputEventBlocking:"))
		return -1;

	string eventBlockingName = VuoStringUtilities::substrAfter(annotation, "vuoInputEventBlocking:");
	if (eventBlockingName == "VuoPortEventBlocking_None")
		return VuoPortClass::EventBlocking_None;
	else if (eventBlockingName == "VuoPortEventBlocking_Door")
		return VuoPortClass::EventBlocking_Door;
	else if (eventBlockingName == "VuoPortEventBlocking_Wall")
		return VuoPortClass::EventBlocking_Wall;

	fprintf(stderr, "Unknown %s\n", annotation.c_str());
	return -1;
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
			fprintf(stderr, "Port %s is declared as an input port in one function and an output port in another function.\n", argumentName.c_str());
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

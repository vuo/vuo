/**
 * @file
 * VuoCompilerPublishedInputNodeClass implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerPublishedInputNodeClass.hh"
#include "VuoCompiler.hh"
#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerConstantStringCache.hh"
#include "VuoCompilerInputDataClass.hh"
#include "VuoCompilerInputEventPortClass.hh"
#include "VuoCompilerOutputEventPortClass.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoCompilerPublishedPortClass.hh"
#include "VuoCompilerTriggerPortClass.hh"
#include "VuoCompilerType.hh"
#include "VuoGenericType.hh"
#include "VuoNode.hh"
#include "VuoPublishedPort.hh"
#include "VuoStringUtilities.hh"
#include "VuoType.hh"
#include <sstream>


/**
 * Creates a node class implementation from an LLVM module, and creates its corresponding base @c VuoNodeClass.
 */
VuoCompilerPublishedInputNodeClass::VuoCompilerPublishedInputNodeClass(string nodeClassName, Module *module) :
	VuoCompilerSpecializedNodeClass(nodeClassName, module)
{
}

/**
 * Creates a new compiler node class and a new base @c VuoNodeClass, both from @c compilerNodeClass.
 */
VuoCompilerPublishedInputNodeClass::VuoCompilerPublishedInputNodeClass(VuoCompilerPublishedInputNodeClass *compilerNodeClass) :
	VuoCompilerSpecializedNodeClass(compilerNodeClass)
{
}

/**
 * Creates a new implementation-less compiler node class, using the given node class for its base VuoNodeClass.
 */
VuoCompilerPublishedInputNodeClass::VuoCompilerPublishedInputNodeClass(VuoNodeClass *baseNodeClass) :
	VuoCompilerSpecializedNodeClass(baseNodeClass)
{
}

/**
 * Returns a node class with an input port corresponding to (same name, same type) each of @a publishedInputPorts,
 * and an event-only trigger output port.
 */
VuoNodeClass * VuoCompilerPublishedInputNodeClass::newNodeClass(vector<VuoPublishedPort *> publishedInputPorts)
{
	bool isFullySpecialized = true;
	vector<string> typeNames;
	for (vector<VuoPublishedPort *>::iterator i = publishedInputPorts.begin(); i != publishedInputPorts.end(); ++i)
	{
		VuoType *type = static_cast<VuoCompilerPort *>((*i)->getCompiler())->getDataVuoType();
		string typeName = (type ? type->getModuleKey() : "event");
		typeNames.push_back(typeName);

		if (dynamic_cast<VuoGenericType *>(type))
			isFullySpecialized = false;
	}

	ostringstream typeCount;
	typeCount << typeNames.size();

	vector<string> nodeClassNameParts;
	nodeClassNameParts.push_back(VuoNodeClass::publishedInputNodeClassName);
	nodeClassNameParts.push_back(typeCount.str());
	nodeClassNameParts.insert(nodeClassNameParts.end(), typeNames.begin(), typeNames.end());
	string nodeClassName = VuoStringUtilities::join(nodeClassNameParts, '.');


	VuoCompilerPublishedInputNodeClass *nodeClass = NULL;

	if (isFullySpecialized)
	{
		// The generic port types have been specialized, so generate LLVM bitcode for the node class.

		Module *module = new Module(nodeClassName, getGlobalContext());

		// VuoModuleMetadata({});
		VuoCompilerCodeGenUtilities::generateModuleMetadata(module, "{}", "");

		// void nodeEvent
		// (
		//	VuoInputData(<item type>,<default value>) publishedPort1,
		//  VuoInputData(<item type>,<default value>) publishedPort2,
		//  VuoInputEvent() publishedPort3,
		//	...
		//  VuoOutputTrigger(fired,void)
		// )
		// {}

		vector<VuoPort *> modelInputPorts( publishedInputPorts.begin(), publishedInputPorts.end() );

		FunctionType *triggerFunctionType = FunctionType::get(Type::getVoidTy(module->getContext()), false);
		PointerType *pointerToTriggerFunctionType = PointerType::get(triggerFunctionType, 0);
		VuoCompilerTriggerPortClass *modelTriggerPortClass = new VuoCompilerTriggerPortClass("fired", pointerToTriggerFunctionType);
		VuoCompilerPort *modelTriggerPort = modelTriggerPortClass->newPort();
		vector<VuoPort *> modelOutputPorts( 1, modelTriggerPort->getBase() );

		map<VuoPort *, string> defaultValuesForInputPorts;
		for (vector<VuoPublishedPort *>::iterator i = publishedInputPorts.begin(); i != publishedInputPorts.end(); ++i)
		{
			VuoPublishedPort *publishedInputPort = *i;
			string defaultValue = static_cast<VuoCompilerPublishedPort *>( publishedInputPort->getCompiler() )->getInitialValue();
			if (! defaultValue.empty())
				defaultValuesForInputPorts[publishedInputPort] = defaultValue;
		}

		map<VuoPort *, size_t> indexOfDataParameter;
		map<VuoPort *, size_t> indexOfEventParameter;
		VuoCompilerConstantStringCache constantStrings;

		Function *eventFunction = VuoCompilerCodeGenUtilities::getNodeEventFunction(module, "", false, false, modelInputPorts, modelOutputPorts,
																					map<VuoPort *, json_object *>(), map<VuoPort *, string>(),
																					defaultValuesForInputPorts, map<VuoPort *, VuoPortClass::EventBlocking>(),
																					indexOfDataParameter, indexOfEventParameter, constantStrings);

		delete modelTriggerPort;
		delete modelTriggerPortClass;

		BasicBlock *block = &(eventFunction->getEntryBlock());
		ReturnInst::Create(module->getContext(), block);


		VuoCompilerPublishedInputNodeClass *dummyNodeClass = new VuoCompilerPublishedInputNodeClass(nodeClassName, module);
		nodeClass = new VuoCompilerPublishedInputNodeClass(dummyNodeClass);
		delete dummyNodeClass;
	}
	else
	{
		// The generic ports have not been specialized, so construct a node class that doesn't yet have an implementation.

		VuoPortClass *refreshPortClass = (new VuoCompilerInputEventPortClass("refresh"))->getBase();

		vector<VuoPortClass *> inputPortClasses;
		inputPortClasses.push_back(refreshPortClass);
		for (vector<VuoPublishedPort *>::iterator i = publishedInputPorts.begin(); i != publishedInputPorts.end(); ++i)
		{
			VuoPublishedPort *publishedInputPort = *i;

			string portName = publishedInputPort->getClass()->getName();
			if (portName == "refresh")
				portName = "refresh_";
			VuoType *portType = static_cast<VuoCompilerPort *>((*i)->getCompiler())->getDataVuoType();

			VuoCompilerInputEventPortClass *portClass = new VuoCompilerInputEventPortClass(portName);
			if (portType)
			{
				VuoCompilerInputDataClass *dataClass = new VuoCompilerInputDataClass("", NULL, false);
				portClass->setDataClass(dataClass);
				portClass->setDataVuoType(portType);

				string defaultValue = static_cast<VuoCompilerPublishedPort *>( publishedInputPort->getCompiler() )->getInitialValue();
				if (! defaultValue.empty())
				{
					json_object *defaultValueJson = json_tokener_parse(defaultValue.c_str());
					json_object *details = json_object_new_object();
					json_object_object_add(details, "default", defaultValueJson);
					portClass->setDetails(details);
				}
			}
			inputPortClasses.push_back(portClass->getBase());
		}

		vector<VuoPortClass *> outputPortClasses;
		{
			VuoCompilerOutputEventPortClass *portClass = new VuoCompilerOutputEventPortClass("fired");
			outputPortClasses.push_back(portClass->getBase());
		}

		VuoNodeClass *baseNodeClass = new VuoNodeClass(nodeClassName, refreshPortClass, inputPortClasses, outputPortClasses);
		nodeClass = new VuoCompilerPublishedInputNodeClass(baseNodeClass);
	}

	nodeClass->getBase()->setDefaultTitle(VuoNodeClass::publishedInputNodeIdentifier);
	nodeClass->getBase()->setVersion("1.0.0");

	return nodeClass->getBase();
}

/**
 * Returns a fully specialized node of class @a backingNodeClassName that will replace the node class of @a nodeToBack.
 */
VuoCompilerNode * VuoCompilerPublishedInputNodeClass::createReplacementBackingNode(VuoNode *nodeToBack, string backingNodeClassName, VuoCompiler *compiler)
{
	vector<string> nodeClassNameParts = VuoStringUtilities::split(backingNodeClassName, '.');
	vector<string> typeNames( nodeClassNameParts.begin() + 3, nodeClassNameParts.end() );
	vector<VuoPort *> ports = nodeToBack->getInputPorts();

	vector<VuoPublishedPort *> publishedPorts;
	for (int i = VuoNodeClass::unreservedInputPortStartIndex; i < ports.size(); ++i)
	{
		VuoPort *port = ports[i];
		string portName = port->getClass()->getName();
		string typeName = typeNames[i - VuoNodeClass::unreservedInputPortStartIndex];
		VuoType *vuoType = (typeName == "event" ? NULL : compiler->getType(typeName)->getBase());
		Type *llvmType = (vuoType == NULL ? NULL : vuoType->getCompiler()->getType());
		VuoPortClass::PortType eventOrData = (vuoType == NULL ? VuoPortClass::eventOnlyPort : VuoPortClass::dataAndEventPort);
		VuoCompilerPublishedPortClass *portClass = new VuoCompilerPublishedPortClass(portName, eventOrData, llvmType);
		portClass->setDataVuoType(vuoType);
		VuoCompilerPublishedPort *publishedPort = static_cast<VuoCompilerPublishedPort *>( portClass->newPort() );
		publishedPorts.push_back( static_cast<VuoPublishedPort *>(publishedPort->getBase()) );
	}

	return compiler->createPublishedInputNode(publishedPorts)->getCompiler();
}

/**
 * Returns this port's type in the (hypothetical) unspecialized published input node class.
 */
VuoType * VuoCompilerPublishedInputNodeClass::getOriginalPortType(VuoPortClass *portClass)
{
	/// @todo https://b33p.net/kosada/node/7655
	return NULL;
}

/**
 * Returns the original node's class name (without any type suffixes).
 */
string VuoCompilerPublishedInputNodeClass::getOriginalGenericNodeClassName(void)
{
	return VuoNodeClass::publishedInputNodeClassName;
}

/**
 * Returns the original node's class description (i.e., nothing).
 */
string VuoCompilerPublishedInputNodeClass::getOriginalGenericNodeClassDescription(void)
{
	return "";
}

/**
 * Returns the original node's node set (i.e., none).
 */
VuoNodeSet * VuoCompilerPublishedInputNodeClass::getOriginalGenericNodeSet(void)
{
	return NULL;
}

/**
 * Returns the name for the published input node class that would result if the given ports were changed back to their original types.
 */
string VuoCompilerPublishedInputNodeClass::createUnspecializedNodeClassName(set<VuoPortClass *> portClassesToUnspecialize)
{
	/// @todo https://b33p.net/kosada/node/7655
	return "";
}

/**
 * Returns the name for the published input node class that would result if the given specialized type were substituted for the
 * generic item type.
 */
string VuoCompilerPublishedInputNodeClass::createSpecializedNodeClassNameWithReplacement(string genericTypeName, string specializedTypeName)
{
	/// @todo https://b33p.net/kosada/node/7655
	return "";
}

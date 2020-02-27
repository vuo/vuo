/**
 * @file
 * VuoCompilerPublishedPort implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <dlfcn.h>
#include <sstream>
#include "VuoCompilerInputDataClass.hh"
#include "VuoCompilerInputEventPortClass.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoCompilerPublishedPortClass.hh"
#include "VuoCompilerType.hh"
#include "VuoCable.hh"
#include "VuoGenericType.hh"
#include "VuoPort.hh"
#include "VuoStringUtilities.hh"
#include "VuoHeap.h"

/**
 * Creates a VuoCompilerPublishedPort along with its VuoCompilerPublishedPortClass.
 */
VuoCompilerPublishedPort * VuoCompilerPublishedPort::newPort(string name, VuoType *type)
{
	Type *llvmType = (type ? type->getCompiler()->getType() : nullptr);
	VuoPortClass::PortType eventOrData = (type ? VuoPortClass::dataAndEventPort : VuoPortClass::eventOnlyPort);
	VuoCompilerPublishedPortClass *portClass = new VuoCompilerPublishedPortClass(name, eventOrData, llvmType);
	portClass->setDataVuoType(type);
	return static_cast<VuoCompilerPublishedPort *>( portClass->newPort() );
}

/**
 * Creates a published port that is not connected to any port in a running composition.
 */
VuoCompilerPublishedPort::VuoCompilerPublishedPort(VuoPort *basePort)
	: VuoCompilerPort(basePort)
{
}

/**
 * Returns a unique, consistent identifier for this port.
 */
string VuoCompilerPublishedPort::getIdentifier(void)
{
	return getBase()->getClass()->getName();
}

/**
 * Sets the initial value of the port data. This is the value it has from the time that
 * the composition begins executing until the first time the port data is overwritten.
 *
 * @param initialValueAsString String representation of the initial value.
 */
void VuoCompilerPublishedPort::setInitialValue(string initialValueAsString)
{
	static_cast<VuoCompilerPublishedPortClass *>(getBase()->getClass()->getCompiler())->setDetail("default", initialValueAsString);
}

/**
 * Returns the string representation of the initial value of the port data.
 */
string VuoCompilerPublishedPort::getInitialValue(void)
{
	json_object *details = static_cast<VuoCompilerPublishedPortClass *>(getBase()->getClass()->getCompiler())->getDetails();
	json_object *value = NULL;
	if (json_object_object_get_ex(details, "default", &value))
		return json_object_to_json_string_ext(value, JSON_C_TO_STRING_PLAIN);

	return "";
}

/**
 * Returns details for this published port, such as "suggestedMin".
 *
 * If the detail was set by a call to setDetail(), then that value is used. Otherwise, the detail's value
 * is coalesced across all connected ports.
 */
json_object * VuoCompilerPublishedPort::getDetails(bool isInput)
{
	// Coalesce the details across all connected ports.
	json_object *coalescedDetails = json_object_new_object();

	VuoType *type = static_cast<VuoCompilerPortClass *>(getBase()->getClass()->getCompiler())->getDataVuoType();
	if (isInput && type)
	{
//		VLog("Coalescing input %s %s:", type->getModuleKey().c_str(), getIdentifier().c_str());
		json_object *coalescedSuggestedMin = NULL;
		json_object *coalescedSuggestedMax = NULL;
		json_object *coalescedSuggestedStep = NULL;
		json_object *coalescedAuto = NULL;
		json_object *coalescedAutoSupersedesDefault = NULL;
		json_object *coalescedMenuItems = NULL;
		bool atLeastOneDataSourceCoalesced = false;

		vector<VuoCable *> connectedCables = getBase()->getConnectedCables();
		for (vector<VuoCable *>::iterator i = connectedCables.begin(); i != connectedCables.end(); ++i)
		{
			VuoPort *connectedPort = (*i)->getToPort();
			if (! (connectedPort && connectedPort->hasCompiler()) )
				continue;

			VuoCompilerInputEventPortClass *connectedInputEventPortClass = dynamic_cast<VuoCompilerInputEventPortClass *>(connectedPort->getClass()->getCompiler());
			if (connectedInputEventPortClass)
			{
				VuoCompilerInputDataClass *connectedInputDataClass = connectedInputEventPortClass->getDataClass();
				if (connectedInputDataClass)
				{
					json_object *js = connectedInputDataClass->getDetails();
					json_object *o;

					string typeName = type->getModuleKey();

					if (json_object_object_get_ex(js, "suggestedMin", &o))
					{
//						VLog("	Cable with suggestedMin %s", json_object_to_json_string(o));
						if (!coalescedSuggestedMin)
							coalescedSuggestedMin = o;
						else
						{
							// Use the larger of the current port's suggestedMin.x and all previous ports' suggestedMin.x.
							/// @todo https://b33p.net/kosada/node/7317
							bool newIsBetter = false;
							if (typeName == "VuoInteger")
								newIsBetter = json_object_get_int64(o) > json_object_get_int64(coalescedSuggestedMin);
							else if (typeName == "VuoReal")
								newIsBetter = json_object_get_double(o) > json_object_get_double(coalescedSuggestedMin);
							else if (typeName == "VuoPoint2d" || typeName == "VuoPoint3d" || typeName == "VuoPoint4d")
							{
								double oldX=0,newX=0;
								json_object *x;
								if (json_object_object_get_ex(coalescedSuggestedMin, "x", &x))
									oldX = json_object_get_double(x);
								if (json_object_object_get_ex(o, "x", &x))
									newX = json_object_get_double(x);
								newIsBetter = newX > oldX;
							}

							if (newIsBetter)
								coalescedSuggestedMin = o;
						}
					}

					if (json_object_object_get_ex(js, "suggestedMax", &o))
					{
//						VLog("	Cable with suggestedMax %s", json_object_to_json_string(o));
						if (!coalescedSuggestedMax)
							coalescedSuggestedMax = o;
						else
						{
							// Use the smaller of the current port's suggestedMax.x and all previous ports' suggestedMax.x.
							/// @todo https://b33p.net/kosada/node/7317
							bool newIsBetter = false;
							if (typeName == "VuoInteger")
								newIsBetter = json_object_get_int64(o) < json_object_get_int64(coalescedSuggestedMax);
							else if (typeName == "VuoReal")
								newIsBetter = json_object_get_double(o) < json_object_get_double(coalescedSuggestedMax);
							else if (typeName == "VuoPoint2d" || typeName == "VuoPoint3d" || typeName == "VuoPoint4d")
							{
								double oldX=0,newX=0;
								json_object *x;
								if (json_object_object_get_ex(coalescedSuggestedMax, "x", &x))
									oldX = json_object_get_double(x);
								if (json_object_object_get_ex(o, "x", &x))
									newX = json_object_get_double(x);
								newIsBetter = newX < oldX;
							}

							if (newIsBetter)
								coalescedSuggestedMax = o;
						}
					}

					if (json_object_object_get_ex(js, "suggestedStep", &o))
					{
//						VLog("	Cable with suggestedStep %s", json_object_to_json_string(o));
						if (!coalescedSuggestedStep)
							coalescedSuggestedStep = o;
						else
						{
							// Use the smaller of the current port's suggestedStep.x and all previous ports' suggestedStep.x.
							/// @todo https://b33p.net/kosada/node/7317
							bool newIsBetter = false;
							if (typeName == "VuoInteger")
								newIsBetter = json_object_get_int64(o) < json_object_get_int64(coalescedSuggestedStep);
							else if (typeName == "VuoReal")
								newIsBetter = json_object_get_double(o) < json_object_get_double(coalescedSuggestedStep);
							else if (typeName == "VuoPoint2d" || typeName == "VuoPoint3d" || typeName == "VuoPoint4d")
							{
								double oldX=0,newX=0;
								json_object *x;
								if (json_object_object_get_ex(coalescedSuggestedStep, "x", &x))
									oldX = json_object_get_double(x);
								if (json_object_object_get_ex(o, "x", &x))
									newX = json_object_get_double(x);
								newIsBetter = newX < oldX;
							}

							if (newIsBetter)
								coalescedSuggestedStep = o;
						}
					}

					if (json_object_object_get_ex(js, "auto", &o))
					{
//						VLog("	Cable with auto %s", json_object_to_json_string(o));
						if (!atLeastOneDataSourceCoalesced)
						{
							coalescedAuto = o;

							if (json_object_object_get_ex(js, "autoSupersedesDefault", &o))
								coalescedAutoSupersedesDefault = o;
						}
						else if (coalescedAuto != o)
						{
							coalescedAuto = NULL;
							coalescedAutoSupersedesDefault = NULL;
						}
					}
					else
					{
						coalescedAuto = NULL;
						coalescedAutoSupersedesDefault = NULL;
					}

					if (json_object_object_get_ex(js, "menuItems", &o))
					{
//						VLog("	Cable with menuItems %s", json_object_to_json_string(o));
						if (!coalescedMenuItems)
							coalescedMenuItems = o;
					}

					atLeastOneDataSourceCoalesced = true;
				} // end if (connectedInputDataClass)
			}
		}

		if (coalescedSuggestedMin)
			json_object_object_add(coalescedDetails, "suggestedMin", coalescedSuggestedMin);
		if (coalescedSuggestedMax)
			json_object_object_add(coalescedDetails, "suggestedMax", coalescedSuggestedMax);
		if (coalescedSuggestedStep)
			json_object_object_add(coalescedDetails, "suggestedStep", coalescedSuggestedStep);

		// Use the coalesced auto value only if it came from a single internal source and there
		// are no connected data sources without auto values.
		if (coalescedAuto)
			json_object_object_add(coalescedDetails, "auto", coalescedAuto);
		if (coalescedAutoSupersedesDefault)
			json_object_object_add(coalescedDetails, "autoSupersedesDefault", coalescedAutoSupersedesDefault);

		if (!coalescedMenuItems)
		{
			// If it's a hard-enum type, maybe we can get the menu items from the currently-loaded type.
			// Copied from VuoRuntimeCommunicator::mergeEnumDetails.

			string allowedValuesFunctionName = type->getModuleKey() + "_getAllowedValues";
			typedef void *(*allowedValuesFunctionType)(void);
			allowedValuesFunctionType allowedValuesFunction = (allowedValuesFunctionType)dlsym(RTLD_SELF, allowedValuesFunctionName.c_str());

			string getJsonFunctionName = type->getModuleKey() + "_getJson";
			typedef json_object *(*getJsonFunctionType)(int64_t);
			getJsonFunctionType getJsonFunction = (getJsonFunctionType)dlsym(RTLD_SELF, getJsonFunctionName.c_str());

			string summaryFunctionName = type->getModuleKey() + "_getSummary";
			typedef char *(*summaryFunctionType)(int64_t);
			summaryFunctionType summaryFunction = (summaryFunctionType)dlsym(RTLD_SELF, summaryFunctionName.c_str());

			string listCountFunctionName = "VuoListGetCount_" + type->getModuleKey();
			typedef unsigned long (*listCountFunctionType)(void *);
			listCountFunctionType listCountFunction = (listCountFunctionType)dlsym(RTLD_SELF, listCountFunctionName.c_str());

			string listValueFunctionName = "VuoListGetValue_" + type->getModuleKey();
			typedef int64_t (*listValueFunctionType)(void *, unsigned long);
			listValueFunctionType listValueFunction = (listValueFunctionType)dlsym(RTLD_SELF, listValueFunctionName.c_str());

			if (allowedValuesFunction && getJsonFunction && summaryFunction && listCountFunction && listValueFunction)
			{
				void *allowedValues = allowedValuesFunction();
				VuoRetain(allowedValues);
				unsigned long listCount = listCountFunction(allowedValues);
				json_object *menuItems = json_object_new_array();
				for (unsigned long i = 1; i <= listCount; ++i)
				{
					int64_t value = listValueFunction(allowedValues, i);
					json_object *js = getJsonFunction(value);
					if (!json_object_is_type(js, json_type_string))
						continue;
					const char *key = json_object_get_string(js);
					char *summary = summaryFunction(value);

					json_object *menuItem = json_object_new_object();
					json_object_object_add(menuItem, "value", json_object_new_string(key));
					json_object_object_add(menuItem, "name", json_object_new_string(summary));
					json_object_array_add(menuItems, menuItem);

					free(summary);
				}
				VuoRelease(allowedValues);

				if (json_object_array_length(menuItems))
					coalescedMenuItems = menuItems;
			}
		}
		if (coalescedMenuItems)
			json_object_object_add(coalescedDetails, "menuItems", coalescedMenuItems);

		json_object_get(coalescedSuggestedMin);
		json_object_get(coalescedSuggestedMax);
		json_object_get(coalescedSuggestedStep);
		json_object_get(coalescedAuto);
		json_object_get(coalescedAutoSupersedesDefault);
		json_object_get(coalescedMenuItems);

//		VLog("	Result: %s", json_object_to_json_string(coalescedDetails));
	}


	// Override the coalesced details for any details explicitly set for this published port.

	json_object *details = static_cast<VuoCompilerPublishedPortClass *>(getBase()->getClass()->getCompiler())->getDetails();
	json_object_object_foreach(details, key, val)
	{
		json_object_object_add(coalescedDetails, key, val);
		json_object_get(val);
	}


	return coalescedDetails;
}

/**
 * Returns a string representation of this published port's type and details, as they would appear
 * in the attributes list of a published node declaration within a .vuo (Graphviz DOT format) file.
 */
string VuoCompilerPublishedPort::getGraphvizAttributes(void)
{
	ostringstream output;

	string attributePrefix = " _" + getBase()->getClass()->getName();

	VuoType *type = getDataVuoType();
	string typeName = type ? type->getModuleKey() : "event";
	string escapedTypeName = VuoStringUtilities::transcodeToGraphvizIdentifier(typeName);

	// @todo: The following check may be removed once it is impossible for a published port
	// to have a generic type (https://b33p.net/kosada/node/7673, https://b33p.net/kosada/node/7674,
	// https://b33p.net/kosada/node/7698).  For now, make sure not to associate generic types
	// with published ports in the Graphviz source, so that the upgrade manager
	// (https://b33p.net/kosada/node/7698) can reliably detect published ports whose types need to be
	// inferred and correct for the situation in which the inferred type is generic.
	if (! VuoGenericType::isGenericTypeName(typeName))
		output << attributePrefix << "_type=\"" << escapedTypeName << "\"";

	json_object *details = getDetails(true);
	json_object_object_foreach(details, key, val)
	{
		string attributeSuffix;
		if (strcmp(key, "default") == 0)
		{
			if (! val)
				continue;

			attributeSuffix = "";
		}
		else
			attributeSuffix = string("_") + key;

		string attributeValue = json_object_to_json_string_ext(val, JSON_C_TO_STRING_PLAIN);
		string escapedAttributeValue = VuoStringUtilities::transcodeToGraphvizIdentifier(attributeValue);

		output << attributePrefix << attributeSuffix << "=\"" << escapedAttributeValue << "\"";
	}

	return output.str();
}

/**
 * Returns null, since VuoCompilerPort::generateCreatePortContext() doesn't mean anything for published ports.
 */
Value * VuoCompilerPublishedPort::generateCreatePortContext(Module *module, BasicBlock *block)
{
	return NULL;
}

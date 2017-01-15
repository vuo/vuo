/**
 * @file
 * VuoCompilerPublishedPort implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <sstream>
#include "VuoCompilerEventPort.hh"
#include "VuoCompilerInputDataClass.hh"
#include "VuoCompilerInputEventPortClass.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoCompilerPublishedPortClass.hh"
#include "VuoCable.hh"
#include "VuoGenericType.hh"
#include "VuoPort.hh"
#include "VuoPublishedPort.hh"
#include "VuoStringUtilities.hh"


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
		json_object *coalescedSuggestedMin = NULL;
		json_object *coalescedSuggestedMax = NULL;
		json_object *coalescedSuggestedStep = NULL;

		vector<VuoCable *> connectedCables = getBase()->getConnectedCables();
		for (vector<VuoCable *>::iterator i = connectedCables.begin(); i != connectedCables.end(); ++i)
		{
			VuoPort *connectedPort = (*i)->getToPort();
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
				}
			}
		}

		if (coalescedSuggestedMin)
			json_object_object_add(coalescedDetails, "suggestedMin", coalescedSuggestedMin);
		if (coalescedSuggestedMax)
			json_object_object_add(coalescedDetails, "suggestedMax", coalescedSuggestedMax);
		if (coalescedSuggestedStep)
			json_object_object_add(coalescedDetails, "suggestedStep", coalescedSuggestedStep);

		json_object_get(coalescedSuggestedMin);
		json_object_get(coalescedSuggestedMax);
		json_object_get(coalescedSuggestedStep);
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

	json_object *details = static_cast<VuoCompilerPublishedPortClass *>(getBase()->getClass()->getCompiler())->getDetails();
	json_object_object_foreach(details, key, val)
	{
		string attributeSuffix = (strcmp(key, "default") == 0 ? "" : string("_") + key);

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

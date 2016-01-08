/**
 * @file
 * VuoCompilerPublishedPort implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <sstream>
#include "VuoCompilerEventPort.hh"
#include "VuoCompilerInputEventPortClass.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoGenericType.hh"
#include "VuoPort.hh"
#include "VuoStringUtilities.hh"

/**
 * Creates a published port that is not connected to any port in a running composition.
 *
 * @param name A name for the published port.
 * @param type A type for the published port.
 * @param isOutput A boolean indicating whether the published port is an output port, as opposed to an input port.
 * @param connectedPorts The set of ports within the composition for which this published port is an alias.
 */
VuoCompilerPublishedPort::VuoCompilerPublishedPort(string name, VuoType *type, bool isOutput, const set<VuoCompilerPort *> &connectedPorts)
	: VuoBaseDetail<VuoPublishedPort>("VuoCompilerPublishedPort",
									  new VuoPublishedPort(	name,
															type,
															isOutput,
															getBasePorts(connectedPorts)))
{
	getBase()->setCompiler(this);
	details = json_object_new_object();
}

/**
 * Destructor.
 */
VuoCompilerPublishedPort::~VuoCompilerPublishedPort(void)
{
	json_object_put(details);
}

/**
 * Returns the identifiers of the internal ports for which this published port is an alias.
 *
 * Assumes @c generateAllocation has been called on each @c VuoCompilerPort that was passed to the constructor.
 */
set<string> VuoCompilerPublishedPort::getConnectedPortIdentifiers(void)
{
	set<string> identifiers;
	set<VuoPort *> connectedPorts = getBase()->getConnectedPorts();
	for (set<VuoPort *>::iterator port = connectedPorts.begin(); port != connectedPorts.end(); ++port)
	{
		VuoCompilerPort *compilerPort = (VuoCompilerPort *)(*port)->getCompiler();
		VuoCompilerEventPort *eventPort = dynamic_cast<VuoCompilerEventPort *>(compilerPort);
		if (eventPort)
			identifiers.insert(eventPort->getIdentifier());
	}

	return identifiers;
}

/**
 * Returns the set of VuoPort pointers corresponding to the input @c set of VuoCompilerPort pointers.
 */
set<VuoPort *> VuoCompilerPublishedPort::getBasePorts(set<VuoCompilerPort *>list)
{
	set<VuoPort *> baseList;
	for (set<VuoCompilerPort *>::iterator i = list.begin(); i != list.end(); ++i)
		baseList.insert((*i)->getBase());

	return baseList;
}

/**
 * Returns details for this published port, such as "suggestedMin".
 *
 * If the detail was set by a call to setDetail(), then that value is used. Otherwise, the detail's value
 * is coalesced across all connected ports.
 */
json_object * VuoCompilerPublishedPort::getDetails(void)
{
	// Coalesce the details across all connected ports.

	json_object *coalescedDetails = json_object_new_object();

	if (getBase()->getInput())
	{
		json_object *coalescedSuggestedMin = NULL;
		json_object *coalescedSuggestedMax = NULL;
		json_object *coalescedSuggestedStep = NULL;

		set<VuoPort *> connectedPorts = getBase()->getConnectedPorts();
		for (set<VuoPort *>::iterator connectedPort = connectedPorts.begin(); connectedPort != connectedPorts.end(); ++connectedPort)
		{
			VuoCompilerInputEventPortClass *connectedInputEventPortClass = dynamic_cast<VuoCompilerInputEventPortClass *>((*connectedPort)->getClass()->getCompiler());
			if (connectedInputEventPortClass)
			{
				VuoCompilerInputDataClass *connectedInputDataClass = connectedInputEventPortClass->getDataClass();
				if (connectedInputDataClass)
				{
					json_object *js = connectedInputDataClass->getDetails();
					json_object *o;

					if (json_object_object_get_ex(js, "suggestedMin", &o))
					{
						if (!coalescedSuggestedMin)
							coalescedSuggestedMin = o;
						else
						{
							// Use the larger of the current port's suggestedMin.x and all previous ports' suggestedMin.x.
							/// @todo https://b33p.net/kosada/node/7317
							bool newIsBetter = false;
							string type = getBase()->getType()->getModuleKey();
							if (type == "VuoInteger")
								newIsBetter = json_object_get_int64(o) > json_object_get_int64(coalescedSuggestedMin);
							else if (type == "VuoReal")
								newIsBetter = json_object_get_double(o) > json_object_get_double(coalescedSuggestedMin);
							else if (type == "VuoPoint2d" || type == "VuoPoint3d" || type == "VuoPoint4d")
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
							string type = getBase()->getType()->getModuleKey();
							if (type == "VuoInteger")
								newIsBetter = json_object_get_int64(o) < json_object_get_int64(coalescedSuggestedMax);
							else if (type == "VuoReal")
								newIsBetter = json_object_get_double(o) < json_object_get_double(coalescedSuggestedMax);
							else if (type == "VuoPoint2d" || type == "VuoPoint3d" || type == "VuoPoint4d")
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
							string type = getBase()->getType()->getModuleKey();
							if (type == "VuoInteger")
								newIsBetter = json_object_get_int64(o) < json_object_get_int64(coalescedSuggestedStep);
							else if (type == "VuoReal")
								newIsBetter = json_object_get_double(o) < json_object_get_double(coalescedSuggestedStep);
							else if (type == "VuoPoint2d" || type == "VuoPoint3d" || type == "VuoPoint4d")
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

	json_object_object_foreach(details, key, val)
	{
		json_object_object_add(coalescedDetails, key, val);
		json_object_get(val);
	}


	return coalescedDetails;
}

/**
 * Sets the value of a detail for this published port.
 * The previous detail value for @a key (if any) is replaced by @a value.
 */
void VuoCompilerPublishedPort::setDetail(string key, string value)
{
	json_object_object_add(details, key.c_str(), json_tokener_parse(value.c_str()));
}

/**
 * Unsets the value of a detail for this published port.
 * The previous detail value for @a key (if any) is removed.
 */
void VuoCompilerPublishedPort::unsetDetail(string key)
{
	json_object_object_del(details, key.c_str());
}

/**
 * Returns a string representation of this published port's type and details, as they would appear
 * in the attributes list of a published node declaration within a .vuo (Graphviz DOT format) file.
 */
string VuoCompilerPublishedPort::getGraphvizAttributes(void)
{
	ostringstream output;

	string attributePrefix = " _" + getBase()->getName();

	string typeName = getBase()->getType() ? getBase()->getType()->getModuleKey() : "event";
	string escapedTypeName = VuoStringUtilities::transcodeToGraphvizIdentifier(typeName);

	// @todo: The following check may be removed once it is impossible for a published port
	// to have a generic type (https://b33p.net/kosada/node/7673, https://b33p.net/kosada/node/7674,
	// https://b33p.net/kosada/node/7698).  For now, make sure not to associate generic types
	// with published ports in the Graphviz source, so that the upgrade manager
	// (https://b33p.net/kosada/node/7698) can reliably detect published ports whose types need to be
	// inferred and correct for the situation in which the inferred type is generic.
	if (! VuoGenericType::isGenericTypeName(typeName))
		output << attributePrefix << "_type=\"" << escapedTypeName << "\"";

	json_object_object_foreach(details, key, val)
	{
		string attributeSuffix = (strcmp(key, "default") == 0 ? "" : string("_") + key);

		string attributeValue = json_object_to_json_string_ext(val, JSON_C_TO_STRING_PLAIN);
		string escapedAttributeValue = VuoStringUtilities::transcodeToGraphvizIdentifier(attributeValue);

		output << attributePrefix << attributeSuffix << "=\"" << escapedAttributeValue << "\"";
	}

	return output.str();
}

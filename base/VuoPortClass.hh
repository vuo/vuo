/**
 * @file
 * VuoPortClass interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOPORTCLASS_HH
#define VUOPORTCLASS_HH

#include "VuoBase.hh"

class VuoCompilerPortClass;
class VuoRendererPortClass;
class VuoCompilerNodeArgumentClass;

/**
 * A port type on a @c VuoNodeClass.
 *
 * @see VuoCompilerPortClass
 */
class VuoPortClass : public VuoBase<VuoCompilerNodeArgumentClass,void>
{
public:
	/**
	 * Possible port types.
	 */
	enum PortType
	{
		notAPort, ///< Represents a dummy port, or a @c VuoCompilerNodeArgumentClass subclass that isn't a port (such as instance data or port data).
		dataAndEventPort, ///< See @c #VuoInputData and @c #VuoOutputData.
		eventOnlyPort, ///< See @c #VuoInputEvent and @c #VuoOutputEvent.
		triggerPort ///< See @c #VuoOutputTrigger.
	};

	/**
	 * Possible options for event-blocking behavior. See @ref VuoPortEventBlocking in node.h.
	 */
	enum EventBlocking
	{
		EventBlocking_None, ///< An event received by this input port is transmitted to all output ports.
		EventBlocking_Door, ///< An event received by this input port may be transmitted to all, some, or none of the output ports.
		EventBlocking_Wall ///< An event received by this input port is never transmitted to any output port.
	};

	VuoPortClass(string name, enum PortType portType);
	string getName(void);
	PortType getPortType(void);
	EventBlocking getEventBlocking(void);
	void setEventBlocking(EventBlocking eventBlocking);

	void print(void);

private:
	string name; ///< The parameter's name, used as an identifier and for display.
	enum PortType portType; ///< The port's type.
	enum EventBlocking eventBlocking; ///< The port's event-blocking behavior.
};

#endif // VUOPORTCLASS_HH

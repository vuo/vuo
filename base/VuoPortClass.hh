/**
 * @file
 * VuoPortClass interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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
	 * Options for event-blocking behavior. See @ref VuoInputEvent in node.h.
	 */
	enum EventBlocking
	{
		EventBlocking_None, ///< An event received by this input port is transmitted to all output ports.
		EventBlocking_Door, ///< An event received by this input port may be transmitted to all, some, or none of the output ports.
		EventBlocking_Wall ///< An event received by this input port is never transmitted to any output port.
	};

	/**
	 * Options for event-throttling behavior. See @ref VuoOutputTrigger in node.h.
	 */
	enum EventThrottling
	{
		EventThrottling_Enqueue, ///< An event fired from this port is eventually transmitted downstream.
		EventThrottling_Drop ///< An event fired from this port is dropped if it would have to wait on nodes downstream.
	};

	VuoPortClass(string name, enum PortType portType);
	string getName(void);
	void setName(string name);
	PortType getPortType(void);
	EventBlocking getEventBlocking(void);
	void setEventBlocking(EventBlocking eventBlocking);
	bool hasPortAction(void);
	void setPortAction(bool portAction);
	EventThrottling getDefaultEventThrottling(void);
	void setDefaultEventThrottling(EventThrottling eventThrottling);

	void print(void);

private:
	string name; ///< The parameter's name, used as an identifier and for display.
	enum PortType portType; ///< The port's type.
	enum EventBlocking eventBlocking; ///< The port's event-blocking behavior. Only applies to input ports.
	bool portAction; ///< Whether the port has a port action. Only applies to input ports.
	enum EventThrottling defaultEventThrottling; ///< The port's default event-throttling behavior. Only applies to trigger ports.
};

#endif // VUOPORTCLASS_HH

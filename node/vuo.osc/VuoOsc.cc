/**
 * @file
 * VuoOsc implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoOsc.h"

#include <map>
#include <set>
#include <string>
#include <oscpack/osc/OscPacketListener.h>
#include <oscpack/ip/UdpSocket.h>
#include <CoreServices/CoreServices.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <json/json.h>
#pragma clang diagnostic pop

extern "C"
{
#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoOsc",
					 "dependencies" : [
						 "VuoOscMessage",
						 "CoreServices.framework",
						 "oscpack"
					 ]
				 });
#endif
}


typedef void (*VuoOscReceivedMessageTrigger)(VuoOscMessage);	///< A node's trigger method, to be called when an OSC message is received.

/**
 * This class maintains a list of trigger functions to be called when an OSC message is received.
 * oscpack calls ProcessMessage() when a new OSC message has been parsed,
 * which calls all the trigger functions.
 */
class VuoOscInPacketListener : public osc::OscPacketListener
{
public:
	VuoOscInPacketListener()
	{
		triggerSemaphore = dispatch_semaphore_create(1);
	}

	/**
	 * Adds a trigger callback to be invoked when an OSC message is received.
	 */
	void enableTrigger(VuoOscReceivedMessageTrigger receivedMessage)
	{
		dispatch_semaphore_wait(triggerSemaphore, DISPATCH_TIME_FOREVER);
		triggers.insert(receivedMessage);
		dispatch_semaphore_signal(triggerSemaphore);
	}

	/**
	 * Removes a trigger callback.
	 */
	void disableTrigger(VuoOscReceivedMessageTrigger receivedMessage)
	{
		dispatch_semaphore_wait(triggerSemaphore, DISPATCH_TIME_FOREVER);
		triggers.erase(receivedMessage);
		dispatch_semaphore_signal(triggerSemaphore);
	}

	/**
	 * Returns the number of trigger callbacks enabled for this listener.
	 */
	unsigned int triggerCount(void)
	{
		dispatch_semaphore_wait(triggerSemaphore, DISPATCH_TIME_FOREVER);
		unsigned int size = triggers.size();
		dispatch_semaphore_signal(triggerSemaphore);
		return size;
	}

protected:
	std::set<VuoOscReceivedMessageTrigger> triggers;	///< Trigger methods to call when an OSC message is received.
	dispatch_semaphore_t triggerSemaphore;  ///< Synchronizes access to @c triggers.

	/**
	 * This method is called by oscpack when an OSC message has been parsed.
	 */
	virtual void ProcessMessage(const osc::ReceivedMessage &m, const IpEndpointName &remoteEndpoint)
	{
		if (!triggerCount())
			return;

		try
		{
			// Convert message arguments into a JSON object.
			json_object *jsonArguments = json_object_new_array();
			for (osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin(); arg != m.ArgumentsEnd(); ++arg)
			{
				switch (arg->TypeTag())
				{
					case osc::NIL_TYPE_TAG:
						json_object_array_add(jsonArguments, NULL);
						break;
					case osc::TRUE_TYPE_TAG:
						json_object_array_add(jsonArguments, json_object_new_boolean(true));
						break;
					case osc::FALSE_TYPE_TAG:
						json_object_array_add(jsonArguments, json_object_new_boolean(false));
						break;
					case osc::FLOAT_TYPE_TAG:
						json_object_array_add(jsonArguments, json_object_new_double(arg->AsFloat()));
						break;
					case osc::DOUBLE_TYPE_TAG:
						json_object_array_add(jsonArguments, json_object_new_double(arg->AsDouble()));
						break;
					case osc::INT32_TYPE_TAG:
						json_object_array_add(jsonArguments, json_object_new_int(arg->AsInt32()));
						break;
					case osc::INT64_TYPE_TAG:
						json_object_array_add(jsonArguments, json_object_new_int64(arg->AsInt64()));
						break;
					case osc::STRING_TYPE_TAG:
						json_object_array_add(jsonArguments, json_object_new_string(arg->AsString()));
						break;
					default:
						throw osc::Exception(((std::string)"unknown argument type tag '" + arg->TypeTag() + "'").c_str());
				}
			}

			VuoOscMessage vuoMessage = VuoOscMessage_make(VuoText_make(m.AddressPattern()), jsonArguments);

			// Send it to all listening nodes.
			dispatch_semaphore_wait(triggerSemaphore, DISPATCH_TIME_FOREVER);
			for (std::set<VuoOscReceivedMessageTrigger>::iterator it = triggers.begin(); it != triggers.end(); ++it)
				(*it)(vuoMessage);
			dispatch_semaphore_signal(triggerSemaphore);
		}
		catch (osc::Exception &e)
		{
			VLog("Error parsing message: %s: %s", m.AddressPattern(), e.what());
		}
	}
};

/**
 * Callback needed to asynchronously register a CFNetService.
 * Apparently this doesn't need to do anything.
 */
static void registerCallback(CFNetServiceRef theService, CFStreamError *error, void *info)
{
}

/**
 * Opens a reusable UDP socket, attaches the specified listener to it (on a separate thread),
 * and starts it receiving messages.
 */
class VuoOscInSocket : public UdpSocket
{
	SocketReceiveMultiplexer mux_;
	VuoOscInPacketListener *listener_;
	CFNetServiceRef netService;
	int uses = 0;
	bool stopping = false;

public:
	/**
	 * Runs the receive multiplexer until `stopping` becomes true and the multiplexer receives a `AsynchronousBreak()`.
	 * Catches and logs exceptions, then resumes listening.
	 */
	void listenForMessages(void)
	{
		while (!stopping)
		{
			try
			{
				mux_.Run();
			}
			catch (osc::MalformedPacketException e)
			{
				VLog("Malformed OSC packet: %s", e.what());
			}
			catch (osc::MalformedBundleException e)
			{
				VLog("Malformed OSC bundle: %s", e.what());
			}
			catch (osc::MalformedMessageException e)
			{
				VLog("Malformed OSC message: %s", e.what());
			}
			catch (osc::WrongArgumentTypeException e)
			{
				VLog("OSC: Wrong argument type: %s", e.what());
			}
			catch (osc::MissingArgumentException e)
			{
				VLog("OSC: Missing argument: %s", e.what());
			}
			catch (osc::ExcessArgumentException e)
			{
				VLog("OSC: Excess argument: %s", e.what());
			}
			catch (...)
			{
				VLog("Unknown OSC exception");
			}
		}
	}

	/**
	 * Creates a socket on the specified endpoint.  Invokes the specified listener when a message is received.
	 */
	VuoOscInSocket(const IpEndpointName &localEndpoint, VuoOscInPacketListener *listener)
		: UdpSocket(), listener_(listener)
	{
		netService = NULL;
		SetAllowReuse(true);

		try
		{
			Bind(localEndpoint);
		}
		catch(std::exception const &e)
		{
			VLog("Error: (port %d) %s", localEndpoint.port, e.what());
			return;
		}

		mux_.AttachSocketListener(this, listener_);

		dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
						   listenForMessages();
					   });


		// Create a Bonjour service.
		netService = CFNetServiceCreate(NULL, CFSTR(""), CFSTR("_osc._udp"), CFSTR("Vuo OSC Server"), Port());

		// Add a TXT record, type=server, per http://opensoundcontrol.org/topic/110 proposal #2.
		{
			CFStringRef keys[] = { CFSTR("type") };
			CFTypeRef values[] = { CFSTR("server") };
			CFDictionaryRef attr = CFDictionaryCreate(NULL, (const void **)&keys, (const void **)&values, sizeof(keys) / sizeof(keys[0]), NULL, NULL);
			CFDataRef         txtRecord = CFNetServiceCreateTXTDataWithDictionary( NULL, attr );
			CFRelease(attr);

			CFNetServiceSetTXTData(netService, txtRecord);

			CFRelease(txtRecord);
		}

		// Start advertising the Bonjour service.
		CFNetServiceClientContext clientContext = { 0, NULL, NULL, NULL, NULL };
		CFNetServiceSetClient(netService, registerCallback, &clientContext);
		CFNetServiceScheduleWithRunLoop(netService, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
		CFStreamError error;
		if (CFNetServiceRegisterWithOptions(netService, kCFNetServiceFlagNoAutoRename, &error) == false)
		{
			CFNetServiceUnscheduleFromRunLoop(netService, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
			CFNetServiceSetClient(netService, NULL, NULL);
			CFNetServiceCancel(netService);
			CFRelease(netService);
			netService = NULL;
			VLog("Error: Could not advertise OSC server via Bonjour (domain=%ld, error=%d)", error.domain, error.error);
		}
	}

	/**
	 * Increment this socket's retain count.
	 */
	void use(void)
	{
		++uses;
	}

	/**
	 * Decrement this socket's retain count.
	 */
	void disuse(void)
	{
		--uses;
	}

	/**
	 * Returns this socket's retain count.
	 */
	int useCount(void)
	{
		return uses;
	}

	/**
	 * Returns this socket's listener instance.
	 */
	VuoOscInPacketListener *listener(void)
	{
		return listener_;
	}

	~VuoOscInSocket()
	{
		if (netService)
		{
			// Stop advertising the Bonjour service.
			CFNetServiceUnscheduleFromRunLoop(netService, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
			CFNetServiceSetClient(netService, NULL, NULL);
			CFNetServiceCancel(netService);
			CFRelease(netService);
		}

		if (IsBound())
		{
			stopping = true;
			mux_.AsynchronousBreak();
			mux_.DetachSocketListener(this, listener_);
		}

		delete listener_;
	}
};

std::map<unsigned int, VuoOscInSocket *> VuoOscInPool;	///< Sockets, keyed by port.
dispatch_semaphore_t VuoOscInPool_semaphore = NULL;  ///< Synchronizes access to @c VuoOscInPool.

/**
 * Initializes @c VuoOscInPool_semaphore.
 */
__attribute__((constructor)) static void VuoOscIn_init(void)
{
	VuoOscInPool_semaphore = dispatch_semaphore_create(1);
}

/**
 * Private data for a VuoOscIn instance.
 */
struct VuoOscIn_internal
{
	unsigned int port;	///< The port to listen on.
	VuoOscReceivedMessageTrigger receivedMessage;	///< The trigger function to call when a newly-arrived OSC message has been parsed.
};

void VuoOscIn_destroy(VuoOscIn oi);

/**
 * Creates a reference-counted object to manage receiving messages from a OSC device.
 * If @c port is nonzero, attempts to use the specified port.  If zero, automatically chooses an unused port.
 * If the port is successfully opened, advertises the server via Bonjour.
 */
VuoOscIn VuoOscIn_make(VuoInteger port)
{
	struct VuoOscIn_internal *oii;
	oii = (struct VuoOscIn_internal *)calloc(1, sizeof(struct VuoOscIn_internal));
	VuoRegister(oii, VuoOscIn_destroy);
	oii->port = port;

	dispatch_semaphore_wait(VuoOscInPool_semaphore, DISPATCH_TIME_FOREVER);
	{
		if (VuoOscInPool.find(oii->port) == VuoOscInPool.end())
		{
//			VLog("No socket found for port %d; creating one.", oii->port);
			VuoOscInPacketListener *listener = new VuoOscInPacketListener;
			IpEndpointName endpoint(IpEndpointName::ANY_ADDRESS, oii->port==0 ? IpEndpointName::ANY_PORT : oii->port);
			VuoOscInSocket *socket = new VuoOscInSocket(endpoint, listener);
			VuoOscInPool[oii->port] = socket;
		}
		VuoOscInPool[oii->port]->use();
	}
	dispatch_semaphore_signal(VuoOscInPool_semaphore);

	return (VuoOscIn)oii;
}

/**
 * Sets up the OSC server to call the trigger functions when it receives a message.
 *
 * @threadAny
 */
void VuoOscIn_enableTriggers
(
		VuoOscIn oi,
		VuoOutputTrigger(receivedMessage, VuoOscMessage)
)
{
	if (!oi)
		return;

	struct VuoOscIn_internal *oii = (struct VuoOscIn_internal *)oi;
	oii->receivedMessage = receivedMessage;

	dispatch_semaphore_wait(VuoOscInPool_semaphore, DISPATCH_TIME_FOREVER);
	{
//		VLog("Enabling trigger %p", receivedMessage);
		VuoOscInPool[oii->port]->listener()->enableTrigger(receivedMessage);
	}
	dispatch_semaphore_signal(VuoOscInPool_semaphore);
}

/**
 * Stops the OSC server from calling trigger functions when it receives a message.
 *
 * @threadAny
 */
void VuoOscIn_disableTriggers(VuoOscIn oi)
{
	if (!oi)
		return;

	struct VuoOscIn_internal *oii = (struct VuoOscIn_internal *)oi;

	dispatch_semaphore_wait(VuoOscInPool_semaphore, DISPATCH_TIME_FOREVER);
	{
		VuoOscInPool[oii->port]->listener()->disableTrigger(oii->receivedMessage);
//		VLog("Disabled trigger %p", oii->receivedMessage);
	}
	dispatch_semaphore_signal(VuoOscInPool_semaphore);

	oii->receivedMessage = NULL;
}

/**
 * Destroys an OSC server.
 */
void VuoOscIn_destroy(VuoOscIn oi)
{
	if (!oi)
		return;

	struct VuoOscIn_internal *oii = (struct VuoOscIn_internal *)oi;

	dispatch_semaphore_wait(VuoOscInPool_semaphore, DISPATCH_TIME_FOREVER);
	{
		VuoOscInPool[oii->port]->disuse();
		if (!VuoOscInPool[oii->port]->useCount())
		{
//			VLog("Deleting socket for port %d.", oii->port);
			std::map<unsigned int, VuoOscInSocket *>::iterator it = VuoOscInPool.find(oii->port);
			delete it->second;
			VuoOscInPool.erase(it);
		}
	}
	dispatch_semaphore_signal(VuoOscInPool_semaphore);

	free(oi);
}

/**
 * @file
 * VuoOsc implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoOsc.h"
#include "VuoPool.hh"

#include <map>
#include <set>
#include <vector>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <oscpack/osc/OscOutboundPacketStream.h>
#include <oscpack/osc/OscPacketListener.h>
#include <oscpack/ip/UdpSocket.h>
#include <CoreServices/CoreServices.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <json-c/json.h>
#pragma clang diagnostic pop

extern "C"
{
#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoOsc",
					 "dependencies" : [
						 "VuoOscMessage",
						 "VuoOscInputDevice",
						 "VuoOscOutputDevice",
						 "VuoList_VuoOscMessage",
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
	 * This method is called by oscpack when an OSC bundle has been parsed.
	 */
	virtual void ProcessBundle(const osc::ReceivedBundle& b, const IpEndpointName& remoteEndpoint)
	{
//		VLog("%ld with %d elements", b.TimeTag(), b.ElementCount());

		for (osc::ReceivedBundle::const_iterator i = b.ElementsBegin(); i != b.ElementsEnd(); ++i)
		{
			if (i->IsBundle())
			{
//				VLog("	contains bundle");
				ProcessBundle(osc::ReceivedBundle(*i), remoteEndpoint);
			}
			else
			{
//				VLog("	contains message");
				ProcessMessage(osc::ReceivedMessage(*i), remoteEndpoint);
			}
		}
	}

	/**
	 * This method is called by oscpack or @ref ProcessBundle when an OSC message has been parsed.
	 */
	virtual void ProcessMessage(const osc::ReceivedMessage &m, const IpEndpointName &remoteEndpoint)
	{
		if (!triggerCount())
			return;

		try
		{
			// Convert message arguments into a JSON object.
			json_object *data[VUOOSC_MAX_MESSAGE_ARGUMENTS];
			VuoOscType dataTypes[VUOOSC_MAX_MESSAGE_ARGUMENTS];
			unsigned int i = 0;
			for (osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin(); arg != m.ArgumentsEnd(); ++arg, ++i)
			{
				switch (arg->TypeTag())
				{
					case osc::NIL_TYPE_TAG:
						data[i] = NULL;
						dataTypes[i] = VuoOscType_Auto;
						break;
					case osc::TRUE_TYPE_TAG:
						data[i] = json_object_new_boolean(true);
						dataTypes[i] = VuoOscType_Auto;
						break;
					case osc::FALSE_TYPE_TAG:
						data[i] = json_object_new_boolean(false);
						dataTypes[i] = VuoOscType_Auto;
						break;
					case osc::FLOAT_TYPE_TAG:
						data[i] = json_object_new_double(arg->AsFloat());
						dataTypes[i] = VuoOscType_Float32;
						break;
					case osc::DOUBLE_TYPE_TAG:
						data[i] = json_object_new_double(arg->AsDouble());
						dataTypes[i] = VuoOscType_Auto;
						break;
					case osc::INT32_TYPE_TAG:
						data[i] = json_object_new_int(arg->AsInt32());
						dataTypes[i] = VuoOscType_Int32;
						break;
					case osc::INT64_TYPE_TAG:
						data[i] = json_object_new_int64(arg->AsInt64());
						dataTypes[i] = VuoOscType_Auto;
						break;
					case osc::STRING_TYPE_TAG:
						data[i] = json_object_new_string(arg->AsString());
						dataTypes[i] = VuoOscType_Auto;
						break;
					default:
						throw osc::Exception(((std::string)"unknown argument type tag '" + arg->TypeTag() + "'").c_str());
				}
			}

			VuoOscMessage vuoMessage = VuoOscMessage_make(VuoText_make(m.AddressPattern()), i, data, dataTypes);

			// Send it to all listening nodes.
			dispatch_semaphore_wait(triggerSemaphore, DISPATCH_TIME_FOREVER);
			for (std::set<VuoOscReceivedMessageTrigger>::iterator it = triggers.begin(); it != triggers.end(); ++it)
				(*it)(vuoMessage);
			dispatch_semaphore_signal(triggerSemaphore);
		}
		catch (osc::Exception &e)
		{
			VUserLog("Error parsing message: %s: %s", m.AddressPattern(), e.what());
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
 * Creates a new CFNetService for advertising an OSC server or client.
 */
static CFNetServiceRef VuoOsc_createNetService(char *name, int port, bool isServer)
{
	bool defaultName = false;
	if (VuoText_isEmpty(name))
	{
		name = VuoText_format("Vuo OSC %s", isServer ? "Server" : "Client");
		defaultName = true;
	}

	CFStringRef nameCF = CFStringCreateWithCString(NULL, name, kCFStringEncodingUTF8);
	if (defaultName)
		free(name);
	if (!nameCF)
		return NULL;

	CFNetServiceRef netService = CFNetServiceCreate(NULL, CFSTR(""), CFSTR("_osc._udp"), nameCF, port);
	CFRelease(nameCF);

	// Add a TXT record, type=server, per http://opensoundcontrol.org/topic/110 proposal #2.
	{
		CFStringRef keys[] = { CFSTR("type") };
		CFTypeRef values[] = { isServer ? CFSTR("server") : CFSTR("client") };
		CFDictionaryRef attr = CFDictionaryCreate(NULL, (const void **)&keys, (const void **)&values, sizeof(keys) / sizeof(keys[0]), NULL, NULL);
		CFDataRef txtRecord = CFNetServiceCreateTXTDataWithDictionary( NULL, attr );
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
		VUserLog("Error: Could not advertise OSC %s via Bonjour (domain=%ld, error=%d)", isServer ? "server" : "client", error.domain, error.error);
		return NULL;
	}

	return netService;
}

/**
 * Stop advertising the Bonjour service.
 */
static void VuoOsc_destroyNetService(CFNetServiceRef netService)
{
	CFNetServiceUnscheduleFromRunLoop(netService, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
	CFNetServiceSetClient(netService, NULL, NULL);
	CFNetServiceCancel(netService);
	CFRelease(netService);
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
	dispatch_queue_t queue;

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
				VUserLog("Malformed OSC packet: %s", e.what());
			}
			catch (osc::MalformedBundleException e)
			{
				VUserLog("Malformed OSC bundle: %s", e.what());
			}
			catch (osc::MalformedMessageException e)
			{
				VUserLog("Malformed OSC message: %s", e.what());
			}
			catch (osc::WrongArgumentTypeException e)
			{
				VUserLog("OSC: Wrong argument type: %s", e.what());
			}
			catch (osc::MissingArgumentException e)
			{
				VUserLog("OSC: Missing argument: %s", e.what());
			}
			catch (osc::ExcessArgumentException e)
			{
				VUserLog("OSC: Excess argument: %s", e.what());
			}
			catch (...)
			{
				VUserLog("Unknown OSC exception");
			}
		}
	}

	/**
	 * Creates a socket on the specified endpoint.  Invokes the specified listener when a message is received.
	 */
	VuoOscInSocket(const IpEndpointName &localEndpoint, VuoOscInPacketListener *listener, char *name)
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
			VUserLog("Error: (port %d) %s", localEndpoint.port, e.what());
			return;
		}

		mux_.AttachSocketListener(this, listener_);

		netService = VuoOsc_createNetService(name, Port(), true);

		queue = dispatch_queue_create("org.vuo.osc.receive", NULL);

		dispatch_async(queue, ^{
						   listenForMessages();
					   });
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
			VuoOsc_destroyNetService(netService);

		if (IsBound())
		{
			stopping = true;
			mux_.AsynchronousBreak();
			mux_.DetachSocketListener(this, listener_);

			dispatch_sync(queue, ^{});
			dispatch_release(queue);
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
	VuoOscInputDevice device;	///< The port to listen on, and the name of the Bonjour service.
	VuoOscReceivedMessageTrigger receivedMessage;	///< The trigger function to call when a newly-arrived OSC message has been parsed.
};

void VuoOscIn_destroy(VuoOscIn oi);

/**
 * Creates a reference-counted object to manage receiving messages from a OSC device.
 * If the device's port is nonzero, attempts to use the specified port.  If zero, automatically chooses an unused port.
 * If the port is successfully opened, advertises the server via Bonjour.
 */
VuoOscIn VuoOscIn_make(const VuoOscInputDevice device)
{
	struct VuoOscIn_internal *oii;
	oii = (struct VuoOscIn_internal *)calloc(1, sizeof(struct VuoOscIn_internal));
	VuoRegister(oii, VuoOscIn_destroy);

	oii->device = device;
	VuoOscInputDevice_retain(oii->device);

	dispatch_semaphore_wait(VuoOscInPool_semaphore, DISPATCH_TIME_FOREVER);
	{
		if (VuoOscInPool.find(oii->device.port) == VuoOscInPool.end())
		{
//			VLog("No socket found for port %lld; creating one.", oii->device.port);
			VuoOscInPacketListener *listener = new VuoOscInPacketListener;
			IpEndpointName endpoint(IpEndpointName::ANY_ADDRESS, oii->device.port==0 ? IpEndpointName::ANY_PORT : oii->device.port);
			VuoOscInSocket *socket = new VuoOscInSocket(endpoint, listener, (char *)oii->device.name);
			VuoOscInPool[oii->device.port] = socket;
		}
		VuoOscInPool[oii->device.port]->use();
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
		VuoOscInPool[oii->device.port]->listener()->enableTrigger(receivedMessage);
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
		VuoOscInPool[oii->device.port]->listener()->disableTrigger(oii->receivedMessage);
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
		VuoOscInPool[oii->device.port]->disuse();
		if (!VuoOscInPool[oii->device.port]->useCount())
		{
//			VLog("Deleting socket for port %lld.", oii->device.port);
			std::map<unsigned int, VuoOscInSocket *>::iterator it = VuoOscInPool.find(oii->device.port);
			delete it->second;
			VuoOscInPool.erase(it);
		}
	}
	dispatch_semaphore_signal(VuoOscInPool_semaphore);

	VuoOscInputDevice_release(oii->device);

	free(oi);
}

/**
 * Enable VuoOscOutputDevice to be used in `std::map`.
 */
class VuoOscOutputIdentifier
{
public:
	VuoOscOutputDevice device;	///< The output device.

	/**
	 * Creates an output device wrapper.
	 */
	VuoOscOutputIdentifier(VuoOscOutputDevice _device)
		: device(_device)
	{
	}

	/**
	 * Returns true if this device should be sorted before the `other` device.
	 * Enables using this class with `std::map`.
	 */
	bool operator < (const VuoOscOutputIdentifier &other) const
	{
		return VuoOscOutputDevice_isLessThan(device, other.device);
	}
};

/**
 * Private data for a VuoOscOut instance.
 */
typedef struct _VuoOscOut_internal
{
	VuoOscOutputDevice device;		///< The target device this instance represents.
	std::vector<UdpTransmitSocket *> sockets;
	std::vector<CFNetServiceRef> netServices;
	dispatch_queue_t queue;
} *VuoOscOut_internal;

/**
 * Returns an available UDP port.
 */
static int VuoOsc_findAvailableUdpPort(void)
{
	int socketFD = socket(PF_INET, SOCK_DGRAM, 0);
	if (socketFD == -1)
	{
		VUserLog("Couldn't open port: %s", strerror(errno));
		return 0;
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = PF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(socketFD, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	{
		VUserLog("Couldn't bind: %s", strerror(errno));
		close(socketFD);
		return 0;
	}

	struct sockaddr_in sin;
	socklen_t len = sizeof(sin);
	if (getsockname(socketFD, (struct sockaddr *)&sin, &len) == -1)
	{
		VUserLog("Couldn't get socket name: %s", strerror(errno));
		close(socketFD);
		return 0;
	}

	close(socketFD);

	return ntohs(sin.sin_port);
}

/// @{
VUOKEYEDPOOL(VuoOscOutputIdentifier, VuoOscOut_internal);
static void VuoOscOut_destroy(VuoOscOut_internal ai);
VuoOscOut_internal VuoOscOut_make(VuoOscOutputIdentifier device)
{
	if (VuoIsDebugEnabled())
	{
		char *summary = VuoOscOutputDevice_getSummary(device.device);
		VUserLog("%s", summary);
		free(summary);
	}

	VuoInteger actualPort = device.device.port;
	if (!actualPort)
		actualPort = VuoOsc_findAvailableUdpPort();

	std::vector<IpEndpointName *> endpoints;
	if (device.device.ipAddress)
		endpoints.push_back(new IpEndpointName(device.device.ipAddress, actualPort));
	else
	{
		// If we don't have a specific IP address,
		// then we should broadcast to all interfaces (not just the default interface).

		struct ifaddrs *interfaces;
		if (getifaddrs(&interfaces))
		{
			VUserLog("Using link-local broadcast, since I couldn't get a list of host interfaces: %s", strerror(errno));
			endpoints.push_back(new IpEndpointName("255.255.255.255", actualPort));
		}
		else
		{
			for (struct ifaddrs *address = interfaces; address; address = address->ifa_next)
			{
				if (address->ifa_addr->sa_family != AF_INET)
					continue;

				struct sockaddr_in *ip = (struct sockaddr_in *)address->ifa_dstaddr;
				endpoints.push_back(new IpEndpointName(ntohl(ip->sin_addr.s_addr), actualPort));
			}
			freeifaddrs(interfaces);
		}
	}


	std::vector<UdpTransmitSocket *> sockets;
	std::vector<CFNetServiceRef> netServices;
	for (std::vector<IpEndpointName *>::iterator endpoint = endpoints.begin(); endpoint != endpoints.end(); ++endpoint)
	{
		try
		{
			if (VuoIsDebugEnabled())
			{
				char s[IpEndpointName::ADDRESS_AND_PORT_STRING_LENGTH];
				(*endpoint)->AddressAndPortAsString(s);
				VUserLog("\t%s", s);
			}
			UdpTransmitSocket *socket = new UdpTransmitSocket(**endpoint);
			socket->SetEnableBroadcast(true);
			sockets.push_back(socket);

			netServices.push_back(VuoOsc_createNetService((char *)device.device.name, (*endpoint)->port, false));
		}
		catch (std::exception const &e)
		{
			char s[IpEndpointName::ADDRESS_AND_PORT_STRING_LENGTH];
			(*endpoint)->AddressAndPortAsString(s);
			VUserLog("Error: %s %s", s, e.what());
			delete *endpoint;
			return NULL;
		}
		delete *endpoint;
	}


	VuoOscOut_internal ai = new _VuoOscOut_internal;
	VuoRegister(ai, (DeallocateFunctionType)VuoOscOut_destroy);

	ai->netServices = netServices;

	ai->device = device.device;
	VuoOscOutputDevice_retain(ai->device);

	ai->sockets = sockets;

	ai->queue = dispatch_queue_create("org.vuo.osc.send", NULL);

	return ai;
}
static void VuoOscOut_destroy(VuoOscOut_internal ai)
{
	for (std::vector<CFNetServiceRef>::iterator netService = ai->netServices.begin(); netService != ai->netServices.end(); ++netService)
		VuoOsc_destroyNetService(*netService);

	for (std::vector<UdpTransmitSocket *>::iterator socket = ai->sockets.begin(); socket != ai->sockets.end(); ++socket)
		delete *socket;

	VuoOscOut_internalPool->removeSharedInstance(ai->device);

	VuoOscOutputDevice_release(ai->device);

	dispatch_sync(ai->queue, ^{});
	dispatch_release(ai->queue);

	delete ai;
}
VUOKEYEDPOOL_DEFINE(VuoOscOutputIdentifier, VuoOscOut_internal, VuoOscOut_make);
/// @}

/**
 * Returns the reference-counted object for the specified OSC output device.
 */
VuoOscOut VuoOscOut_getShared(const VuoOscOutputDevice device)
{
	return (VuoOscOut)VuoOscOut_internalPool->getSharedInstance(device);
}

/**
 * Sends a message to the specified OSC device.
 */
void VuoOscOut_sendMessages(VuoOscOut ao, VuoList_VuoOscMessage messages)
{
	if (!ao || !messages)
		return;

	VuoRetain(messages);
	VuoOscOut_internal aii = (VuoOscOut_internal)ao;
	dispatch_async(aii->queue, ^{
					   bool done = false;
					   int bufferSize = 256;
					   while (!done)
					   {
						   char *buffer = (char *)malloc(bufferSize);
						   try
						   {
							   osc::OutboundPacketStream p(buffer, bufferSize);

							   VuoInteger messageCount = VuoListGetCount_VuoOscMessage(messages);

							   if (messageCount > 1)
								   p << osc::BeginBundleImmediate;

							   for(VuoInteger i = 1; i <= VuoListGetCount_VuoOscMessage(messages); ++i)
							   {
								   VuoOscMessage message = VuoListGetValue_VuoOscMessage(messages, i);
								   if (!message)
									   continue;

								   p << osc::BeginMessage(message->address);

								   for (int i = 0; i < message->dataCount; ++i)
								   {
									   if (message->dataTypes[i] == VuoOscType_Auto)
									   {
										   json_object *datum = message->data[i];

										   json_type type = json_object_get_type(datum);

										   if (type == json_type_null)
											   p << osc::OscNil;
										   else if (type == json_type_boolean)
											   p << (bool)json_object_get_boolean(datum);
										   else if (type == json_type_double)
											   p << json_object_get_double(datum);
										   else if (type == json_type_int)
											   p << (osc::int64)json_object_get_int64(datum);
										   else if (type == json_type_string)
											   p << json_object_get_string(datum);
										   else
										   {
											   VUserLog("Error: Unknown type: %s", json_type_to_name(type));
											   p << osc::OscNil;
										   }
									   }
									   else if (message->dataTypes[i] == VuoOscType_Int32)
										   p << (osc::int32)json_object_get_int(message->data[i]);
									   else if (message->dataTypes[i] == VuoOscType_Float32)
										   p << (float)json_object_get_double(message->data[i]);
									   else
									   {
										   VUserLog("Error: Unknown type: %d", message->dataTypes[i]);
										   p << osc::OscNil;
									   }
								   }

								   p << osc::EndMessage;
							   }

							   if (messageCount > 1)
								   p << osc::EndBundle;

							   for (std::vector<UdpTransmitSocket *>::iterator socket = aii->sockets.begin(); socket != aii->sockets.end(); ++socket)
								   (*socket)->Send(p.Data(), p.Size());
							   done = true;
						   }
						   catch (osc::OutOfBufferMemoryException const &e)
						   {
							   bufferSize *= 2;
						   }
						   catch (std::exception const &e)
						   {
							   VUserLog("Error: %s", e.what());
							   done = true;
						   }
						   free(buffer);
					   }
					   VuoRelease(messages);
				   });
}

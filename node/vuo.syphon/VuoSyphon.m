/**
 * @file
 * VuoSyphon implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoImageRenderer.h"
#include "VuoApp.h"
#include "VuoSyphon.h"
#include "VuoSyphonListener.h"
#include "VuoSyphonSender.h"
#include <Syphon.h>

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoSyphon",
					 "dependencies" : [
						"VuoSyphonServerDescription",
						"VuoList_VuoSyphonServerDescription",
						"VuoImageRenderer",
						"VuoSyphonListener",
						"VuoSyphonSender",
						"Syphon.framework"
					 ]
				 });
#endif


/**
 * Returns a list of all available Syphon servers.
 */
VuoList_VuoSyphonServerDescription VuoSyphon_getAvailableServerDescriptions(void)
{
	NSArray *servers = [[SyphonServerDirectory sharedDirectory] serversMatchingName:nil appName:nil];
	VuoList_VuoSyphonServerDescription descriptions = VuoListCreate_VuoSyphonServerDescription();

	for (NSDictionary *server in servers)
	{
		VuoText serverUUID = VuoText_make([[server objectForKey:SyphonServerDescriptionUUIDKey] UTF8String]);
		VuoText serverName = VuoText_make([[server objectForKey:SyphonServerDescriptionNameKey] UTF8String]);
		VuoText applicationName = VuoText_make([[server objectForKey:SyphonServerDescriptionAppNameKey] UTF8String]);

		VuoSyphonServerDescription description = VuoSyphonServerDescription_make(serverUUID, serverName, applicationName, true);
		VuoListAppendValue_VuoSyphonServerDescription(descriptions, description);
	}

	return descriptions;
}

/**
 * Returns the subset of server descriptions that match the given partial description.
 *
 * A server description is considered a match if its UUID, name, and application name contain
 * the UUID, name, and application name in the partial description.
 */
VuoList_VuoSyphonServerDescription VuoSyphon_filterServerDescriptions(VuoList_VuoSyphonServerDescription allDescriptions,
																	  VuoSyphonServerDescription partialDescription)
{
	VuoList_VuoSyphonServerDescription filteredDescriptions = VuoListCreate_VuoSyphonServerDescription();

	unsigned long count = VuoListGetCount_VuoSyphonServerDescription(allDescriptions);
	for (int i = 1; i <= count; ++i)
	{
		VuoSyphonServerDescription description = VuoListGetValue_VuoSyphonServerDescription(allDescriptions, i);

		if (partialDescription.useWildcard)
		{
			if (VuoText_compare(description.serverUUID,      (VuoTextComparison){VuoTextComparison_MatchesWildcard, true}, partialDescription.serverUUID)
			 && VuoText_compare(description.serverName,      (VuoTextComparison){VuoTextComparison_MatchesWildcard, true}, partialDescription.serverName)
			 && VuoText_compare(description.applicationName, (VuoTextComparison){VuoTextComparison_MatchesWildcard, true}, partialDescription.applicationName))
				VuoListAppendValue_VuoSyphonServerDescription(filteredDescriptions, description);
		}
		else
		{
			/// @todo Handle UTF8 names (add VuoText function).
			if ((!partialDescription.serverUUID      || strstr(description.serverUUID,      partialDescription.serverUUID     ) != NULL)
			 && (!partialDescription.serverName      || strstr(description.serverName,      partialDescription.serverName     ) != NULL)
			 && (!partialDescription.applicationName || strstr(description.applicationName, partialDescription.applicationName) != NULL))
				VuoListAppendValue_VuoSyphonServerDescription(filteredDescriptions, description);
		}
	}

	return filteredDescriptions;
}


void VuoSyphonClient_free(void *syphonClient);

/**
 * Creates a Syphon client that is not yet connected to any server.
 */
VuoSyphonClient VuoSyphonClient_make(void)
{
	VuoSyphonListener *listener = [[VuoSyphonListener alloc] init];
	VuoRegister(listener, VuoSyphonClient_free);
	return (VuoSyphonClient)listener;
}

/**
 * Starts listening for frames from the Syphon server (or continues if already connected).
 *
 * Each time a frame is received, the given trigger function is called.
 */
void VuoSyphonClient_connectToServer(VuoSyphonClient syphonClient,
									 VuoSyphonServerDescription serverDescription,
									 VuoOutputTrigger(receivedFrame, VuoImage))
{
	VuoApp_init(false);
	VuoSyphonListener *listener = (VuoSyphonListener *)syphonClient;
	[listener startListeningWithServerDescription:serverDescription callback:receivedFrame];
}

/**
 * Stops the Syphon client from listening for frames from its server.
 */
void VuoSyphonClient_disconnectFromServer(VuoSyphonClient syphonClient)
{
	VuoSyphonListener *listener = (VuoSyphonListener *)syphonClient;
	[listener stopListening];
}

/**
 * Frees memory.
 */
void VuoSyphonClient_free(void *syphonClient)
{
	VuoSyphonListener *listener = (VuoSyphonListener *)syphonClient;
	[listener release];
}



void VuoSyphonServer_free(void *server);

/**
 * Creates and starts a Syphon server, making it available for clients to connect to.
 *
 * Retain the returned @ref VuoSyphonServer, then later release it to stop serving.
 *
 * @param serverName The server name.
 * @return The Syphon server.
 */
VuoSyphonServer VuoSyphonServer_make(const char *serverName)
{
	if (!serverName)
		serverName = "Vuo Syphon Server";

	VuoApp_init(false);
	VuoSyphonSender *server = [[VuoSyphonSender alloc] init];
	[server initServerWithName:[NSString stringWithUTF8String:serverName]];
	VuoRegister(server, VuoSyphonServer_free);
	return (VuoSyphonServer)server;
}

/**
 * Publishes a frame from the Syphon server.
 */
void VuoSyphonServer_publishFrame(VuoSyphonServer server, VuoImage frame)
{
	[(VuoSyphonSender*)server publishFrame:frame];
}

/**
 * Changes the name of the Syphon server.
 */
void VuoSyphonServer_setName(VuoSyphonServer server, const char *serverName)
{
	if (!serverName)
		return;

	[(VuoSyphonSender*)server setName:[NSString stringWithUTF8String:serverName]];
}

/**
 * Makes the Syphon server unavailable, and frees memory.
 */
void VuoSyphonServer_free(void *server)
{
	[(VuoSyphonSender*)server stop];
	[(VuoSyphonSender*)server release];
}

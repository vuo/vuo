/**
 * @file
 * VuoSyphon implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "node_header.h"
#include "VuoImage.h"
#include "VuoSyphonServerDescription.h"
#include "VuoList_VuoSyphonServerDescription.h"


VuoList_VuoSyphonServerDescription VuoSyphon_getAvailableServerDescriptions(void);
VuoList_VuoSyphonServerDescription VuoSyphon_filterServerDescriptions(VuoList_VuoSyphonServerDescription allDescriptions,
																	  VuoSyphonServerDescription partialDescription);


typedef void * VuoSyphonClient;  ///< Receives frames via Syphon from a Syphon server.
VuoSyphonClient VuoSyphonClient_make(void);
void VuoSyphonClient_connectToServer(VuoSyphonClient syphonClient,
									 VuoSyphonServerDescription serverDescription,
									 VuoOutputTrigger(receivedFrame, VuoImage));
void VuoSyphonClient_disconnectFromServer(VuoSyphonClient syphonClient);


typedef void * VuoSyphonServer;  ///< Sends frames via Syphon to a Syphon client.
VuoSyphonServer VuoSyphonServer_make(const char *serverName);
void VuoSyphonServer_publishFrame(VuoSyphonServer server, VuoImage frame);
void VuoSyphonServer_setName(VuoSyphonServer server, const char *serverName);

#ifdef __cplusplus

}
#endif

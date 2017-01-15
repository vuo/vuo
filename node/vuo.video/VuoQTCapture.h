/**
 * @file
 * Vuo QTKit implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "node.h"
#include "VuoVideoFrame.h"
#include "VuoVideoInputDevice.h"
#include "VuoList_VuoVideoInputDevice.h"

/**
 * An object for receiving video frames via QuickTime.
 */
typedef void * VuoQTCapture;

/**
 *	Return the available input devices at this moment.
 */
VuoList_VuoVideoInputDevice VuoQTCapture_getInputDevices(void);

void VuoQTCapture_addOnDevicesChangedCallback( VuoOutputTrigger(devicesDidChange, VuoList_VuoVideoInputDevice) );
void VuoQTCapture_removeOnDevicesChangedCallback( VuoOutputTrigger(devicesDidChange, VuoList_VuoVideoInputDevice) );


/**
 *	Create a new VuoQTCapture device.  Does not start recording, you must
 *	call VuoQTCapture_startListening() separately.
 */
VuoQTCapture VuoQTCapture_make(VuoVideoInputDevice device, VuoOutputTrigger(receivedFrame, VuoVideoFrame));

/**
 *	Did VuoQTCapture_make() initialize the capture session properly?
 */
VuoBoolean VuoQTCapture_isInitialized(VuoQTCapture movie);

/**
 *	Begin listening for video frames on the selected input device.  If an input device has been specified
 * 	but not found in the available devices list, this will wait for the device to appear and begin playing
 *	as soon as it's found.
 */
void VuoQTCapture_startListening(VuoQTCapture movie);
void VuoQTCapture_stopListening(VuoQTCapture movie);

void VuoQtCapture_setInputDevice(VuoQTCapture movie, VuoVideoInputDevice inputDevice);
void VuoQTCapture_setCallback(VuoQTCapture movie, VuoOutputTrigger(receivedFrame, VuoVideoFrame));

#ifdef __cplusplus
}
#endif

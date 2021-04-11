/**
 * @file
 * VuoVideoCapture interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
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
 * An object for receiving video frames via AV Foundation.
 */
typedef void *VuoVideoCapture;

/**
 * Returns the available input devices at this moment.
 */
VuoList_VuoVideoInputDevice VuoVideoCapture_getInputDevices(void);

void VuoVideoCapture_addOnDevicesChangedCallback( VuoOutputTrigger(devicesDidChange, VuoList_VuoVideoInputDevice) );
void VuoVideoCapture_removeOnDevicesChangedCallback( VuoOutputTrigger(devicesDidChange, VuoList_VuoVideoInputDevice) );


/**
 * Creates a new VuoVideoCapture device.  Does not start recording, you must
 * call VuoVideoCapture_startListening() separately.
 */
VuoVideoCapture VuoVideoCapture_make(VuoVideoInputDevice device, VuoOutputTrigger(receivedFrame, VuoVideoFrame));

/**
 * Begins listening for video frames on the selected input device.  If an input device has been specified
 * but not found in the available devices list, this will wait for the device to appear and begin playing
 * as soon as it's found.
 */
void VuoVideoCapture_startListening(VuoVideoCapture movie);
void VuoVideoCapture_stopListening(VuoVideoCapture movie);

void VuoVideoCapture_setCallback(VuoVideoCapture movie, VuoOutputTrigger(receivedFrame, VuoVideoFrame));
void VuoVideoCapture_setSize(VuoVideoCapture movie, VuoInteger width, VuoInteger height);

#ifdef __cplusplus
}
#endif

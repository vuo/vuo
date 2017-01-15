/**
 * @file
 * VuoAudioFile interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoInteger.h"
#include "VuoLoopType.h"
#include "VuoReal.h"
#include "VuoText.h"
#include "VuoAudioSamples.h"
#include "VuoList_VuoAudioSamples.h"

bool VuoAudioFile_getInfo(VuoText url, VuoReal *duration, VuoInteger *channelCount, VuoReal *sampleRate);

/**
 * A state object for firing display refresh events.
 */
typedef void *VuoAudioFile;

VuoAudioFile VuoAudioFile_make(VuoText url);
void VuoAudioFile_enableTriggers(VuoAudioFile af, void (*decodedChannels)(VuoList_VuoAudioSamples), void (*finishedPlayback)(void));
void VuoAudioFile_disableTriggers(VuoAudioFile af);
void VuoAudioFile_setLoopType(VuoAudioFile af, VuoLoopType loop);
void VuoAudioFile_setTime(VuoAudioFile af, VuoReal time);
void VuoAudioFile_play(VuoAudioFile af);
void VuoAudioFile_pause(VuoAudioFile af);
bool VuoAudioFile_isPlaying(VuoAudioFile af);

#ifdef __cplusplus
}
#endif

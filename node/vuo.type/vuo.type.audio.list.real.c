﻿/**
 * @file
 * vuo.type.audio.list.real node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					  "title": "Convert Real List to Audio",
					  "version": "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "vuo-example://vuo.audio/GenerateParametricAudio.vuo" ]
					  }
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoReal) reals,
	VuoOutputData(VuoAudioSamples) samples
)
{
	int size = VuoListGetCount_VuoReal(reals);
	VuoReal* vals = (VuoReal*) calloc( VuoAudioSamples_bufferSize, sizeof(VuoReal) );
	VuoRegister(vals, free);
	memcpy(vals, VuoListGetData_VuoReal(reals), MIN(size, VuoAudioSamples_bufferSize) * sizeof(VuoReal));
	*samples = (VuoAudioSamples) { VuoAudioSamples_bufferSize, vals, VuoAudioSamples_sampleRate };
}

/**
 * @file
 * vuo.data.record node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include <json-c/json.h>

VuoModuleMetadata({
					 "title" : "Record and Play Values",
					 "keywords" : [ "write", "file", "output", "store", "remember", "memory", "playback",
						 "value historian",
					 ],
					 "version" : "1.1.0",
					 "dependencies" : [
						 "VuoUrl"
					 ],
					 "node" : {
						 "exampleCompositions" : [ "RecordCameraDrags.vuo" ],
					 }
				 });

struct nodeInstanceData
{
	VuoGenericType1 lastRecordedValue;
	bool lastRecordedValueSet;

	VuoReal lastRecordedTime;

	struct json_object *recording;
	int lastReadIndex;
	bool atEnd;

	// Copies of port data, so it can be used by fini.
	VuoText url;
	bool overwriteUrl;
	bool mode;
};

static void vuo_data_record_clearRecording(struct nodeInstanceData *instance)
{
	json_object_put(instance->recording);

	instance->lastRecordedValueSet = false;

	instance->lastRecordedTime = -DBL_MAX;

	instance->recording = json_object_new_array();
	instance->lastReadIndex = -1;
}

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *instance = (struct nodeInstanceData *)calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->recording = NULL;

	return instance;
}

static bool vuo_data_record_isOfflineSerializable(json_object *valueJson)
{
	json_type type = json_object_get_type(valueJson);

	if (type == json_type_object)
	{
		json_object *o;
		if (json_object_object_get_ex(valueJson, "pointer", &o))
			return false;

		json_object_object_foreach(valueJson, key, val)
		{
			if (! vuo_data_record_isOfflineSerializable(val))
				return false;
		}
	}
	else if (type == json_type_array)
	{
		int length = json_object_array_length(valueJson);
		for (int i = 0; i < length; ++i)
		{
			json_object *val = json_object_array_get_idx(valueJson, i);
			if (! vuo_data_record_isOfflineSerializable(val))
				return false;
		}
	}

	return true;
}

static void vuo_data_record_writeFile(struct nodeInstanceData *instance)
{
	VuoUrl normalizedUrl = VuoUrl_normalize(instance->url, VuoUrlNormalize_forSaving);
	VuoLocal(normalizedUrl);
	VuoText path = VuoUrl_getPosixPath(normalizedUrl);
	VuoLocal(path);

	if (!instance->overwriteUrl)
	{
		FILE *fp = fopen(path, "r");
		if (fp)
		{
			fclose(fp);
			return;
		}
	}

	FILE *fp = fopen(path, "w");
	if (!fp)
	{
		VUserLog("Error: Can't write to file '%s'.", path);
		return;
	}

	const char *jsonString = json_object_to_json_string_ext(instance->recording, JSON_C_TO_STRING_PLAIN);
	fwrite(jsonString, 1, strlen(jsonString) + 1, fp);
	fclose(fp);
}

static void vuo_data_record_readFile(struct nodeInstanceData *instance)
{
	VuoUrl normalizedUrl = VuoUrl_normalize(instance->url, VuoUrlNormalize_forSaving);
	VuoLocal(normalizedUrl);
	VuoText path = VuoUrl_getPosixPath(normalizedUrl);
	VuoLocal(path);

	FILE *fp = fopen(path, "r");
	if (!fp)
	{
		VUserLog("Error: Can't read file '%s'.", path);
		return;
	}

	fseek(fp, 0, SEEK_END);
	size_t size = ftell(fp);
	rewind(fp);
	char *buffer = (char *)malloc(size);
	fread(buffer, 1, size, fp);
	fclose(fp);

	instance->recording = json_tokener_parse(buffer);
	if (! instance->recording)
		VUserLog("Error: Can't parse recording from file '%s'.", path);

	free(buffer);
}

static bool vuo_data_record_getFrame(struct json_object *recording, int *index, double *frameTime, json_object **frame)
{
	int frameCount = json_object_array_length(recording);
	for ( ; *index < frameCount; ++(*index))
	{
		*frame = json_object_array_get_idx(recording, *index);

		json_object *o;
		if (json_object_object_get_ex(*frame, "time", &o))
		{
			*frameTime = json_object_get_double(o);
			return true;
		}
	}

	return false;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoGenericType1) recordValue,
		VuoInputEvent({"eventBlocking":"wall", "data":"recordValue", "hasPortAction":true}) recordValueEvent,
		VuoInputData(VuoReal, {"default":0.0}) time,
		VuoInputEvent({"eventBlocking":"door", "data":"time"}) timeEvent,
		VuoInputData(VuoText, {"name":"URL", "default":"~/Desktop/recording.json"}) url,
		VuoInputEvent({"eventBlocking":"wall", "data":"url"}) urlEvent,
		VuoInputData(VuoBoolean, {"default":false, "name":"Overwrite URL"}) overwriteUrl,
		VuoInputEvent({"eventBlocking":"wall", "data":"overwriteUrl"}) overwriteUrlEvent,
		VuoInputData(VuoInteger, {"menuItems":[
			{"value":0, "name":"Record"},
			{"value":1, "name":"Playback"},
		], "default":0}) mode,
		VuoInputEvent({"eventBlocking":"wall", "data":"mode"}) modeEvent,
		VuoInputEvent({"eventBlocking":"wall"}) finalize,

		VuoOutputData(VuoGenericType1) value,
		VuoOutputEvent({"data":"value"}) valueEvent,
		VuoOutputEvent() finishedPlayback
)
{
	bool wasRecording = ((*instance)->mode == 0);
	bool modeChanged = ((*instance)->mode != mode);
	bool urlChanged = ! VuoText_areEqual((*instance)->url, url);

	VuoRetain(url);
	VuoRelease((*instance)->url);
	(*instance)->url = url;

	(*instance)->overwriteUrl = overwriteUrl;

	(*instance)->mode = mode;

	if (!(*instance)->recording || modeChanged || urlChanged)
	{
		if ((*instance)->recording && wasRecording)
			vuo_data_record_writeFile(*instance);

		vuo_data_record_clearRecording(*instance);
	}

	if (mode == 0) // record
	{
		if (recordValueEvent)
		{
			// Can't pass it through since recordValue is a wall.
			// Instead, save it for the next timeEvent.
			VuoGenericType1_retain(recordValue);
			VuoGenericType1_release((*instance)->lastRecordedValue);
			(*instance)->lastRecordedValue = recordValue;
			(*instance)->lastRecordedValueSet = true;
		}

		if (timeEvent && (*instance)->lastRecordedValueSet)
		{
			if ((*instance)->lastRecordedTime >= time)
			{
				VUserLog("Error: Time %f is not greater than the last recorded time; skipping this recorded value.", time);
				return;
			}

			struct json_object *valueJson = VuoGenericType1_getJson((*instance)->lastRecordedValue);

			if (! vuo_data_record_isOfflineSerializable(valueJson))
			{
				VUserLog("Error: Type VuoGenericType1 can't be saved to a file.");
				return;
			}

			struct json_object *frame = json_object_new_object();
			json_object_object_add(frame, "time",  json_object_new_double(time));
			json_object_object_add(frame, "value", valueJson);

			json_object_array_add((*instance)->recording, frame);

			(*instance)->lastRecordedTime = time;

			*value = (*instance)->lastRecordedValue;
			*valueEvent = true;
			(*instance)->lastRecordedValueSet = false;
		}

		if (finalize)
		{
			vuo_data_record_writeFile(*instance);
			json_object_put((*instance)->recording);
			(*instance)->recording = NULL;
		}
	}
	else if (timeEvent) // playback
	{
		if (modeChanged || urlChanged)
			vuo_data_record_readFile(*instance);

		if (!(*instance)->recording)
			return;

		json_object *valueJson;
		bool gotFrame;

		int i1 = MAX((*instance)->lastReadIndex, 0);
		double t1 = 0;
		json_object *frame1 = NULL;
		gotFrame = vuo_data_record_getFrame((*instance)->recording, &i1, &t1, &frame1);

		// If we failed to get a frame, we must have started at the first frame (not the last-read frame)
		// and seeked all the way through without getting any frames. There's no data to output.
		if (! gotFrame)
			return;

		// If `time` is before the time of `lastReadIndex`, seek back to the beginning.
		if (time < t1)
		{
			i1 = 0;
			gotFrame = vuo_data_record_getFrame((*instance)->recording, &i1, &t1, &frame1);

			// If `time` is before the first frame, don't output anything.
			if (time < t1)
				return;
		}

		// Seek forward to the frame that would be playing at `time`.
		while (true)
		{
			int i2 = i1 + 1;
			double t2 = 0;
			json_object *frame2 = NULL;
			gotFrame = vuo_data_record_getFrame((*instance)->recording, &i2, &t2, &frame2);

			// If we failed to get a frame, we must have seeked past the end. Output the last frame.
			if (! gotFrame)
			{
				if (! (*instance)->atEnd)
				{
					*finishedPlayback = true;
					(*instance)->atEnd = true;
				}
				goto found;
			}

			// If we found the frame, output it.
			else if (t1 <= time && time < t2)
			{
				(*instance)->atEnd = false;
				goto found;
			}

			i1 = i2;
			t1 = t2;
			frame1 = frame2;
		}

found:
		if (! json_object_object_get_ex(frame1, "value", &valueJson))
			valueJson = NULL;

		*value = VuoGenericType1_makeFromJson(valueJson);
		*valueEvent = (i1 != (*instance)->lastReadIndex);
		(*instance)->lastReadIndex = i1;
	}
}

void nodeInstanceFini
(
	VuoInstanceData(struct nodeInstanceData *) instance
)
{
	if ((*instance)->recording && (*instance)->mode == 0) // record
		vuo_data_record_writeFile(*instance);

	VuoGenericType1_release((*instance)->lastRecordedValue);

	json_object_put((*instance)->recording);
	VuoRelease((*instance)->url);
}

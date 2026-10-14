/**
 * @file
 * vuo.ableton.link node implementation.
 *
 * @copyright Copyright © 2012–2026 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
	"title": "Join Ableton Link Session",
	"keywords": [
		"BPM", "synchronize", "metronome", "clock", "timer", "timing", "phase", "speed", "rate",
		"timecode", "generator", "tracker", "follower", "broadcast", "multicast",
		"peer-to-peer", "p2p", "local network", "lan", "localnet", "ethernet", "wireless", "wifi", "wi-fi",
		"play music together", "notes", "bars", "jam session", "group", "rhythm",
		"pause", "resume", "begin", "end", "the articulation of time",
	],
	"version" : "1.0.0",
	"dependencies" : [
		"VuoAbletonLink",
	],
	"node": {
		"exampleCompositions" : [ "ShowAbletonLinkInfo.vuo" ],
	},
	"compatibility": {
		"macos": {
			"min": "11.0",
		},
	},
});

#include "VuoAbletonLink.h"

struct nodeInstanceData
{
	VuoAbletonLink link;

	VuoReal priorBeatPosition;
	VuoReal priorSessionPhase;
	VuoBoolean firstEvent;
	VuoBoolean wasPlaying;
	VuoBoolean preroll;
};

struct nodeInstanceData *nodeInstanceInit(
	VuoInputData(VuoReal) setTempo,
	VuoInputData(VuoReal) beatsPerMeasure)
{
	struct nodeInstanceData *context = calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	context->link = VuoAbletonLink_make(setTempo, beatsPerMeasure);
	context->priorBeatPosition = INFINITY;
	context->priorSessionPhase = INFINITY;
	context->firstEvent = true;

	return context;
}

void nodeInstanceEvent(
	VuoInputData(VuoReal) time,
	VuoInputEvent({"eventBlocking":"door", "data":"time"}) timeEvent,

	VuoInputData(VuoReal, {"default":120.0, "suggestedMin":20.0, "suggestedMax":200.0, "suggestedStep":1.0}) setTempo,
	VuoInputEvent({"eventBlocking":"wall", "hasPortAction":true, "data":"setTempo"}) setTempoEvent,

	VuoInputEvent({"eventBlocking":"wall"}) startTransport,
	VuoInputEvent({"eventBlocking":"wall"}) stopTransport,

	VuoInputData(VuoInteger, {"menuItems":[
		{"value":0, "name":"Always"},
		{"value":1, "name":"When Transport Running"},
	], "default":0}) allowEvents,
	VuoInputEvent({"eventBlocking":"wall", "data":"allowEvents"}) allowEventsEvent,

	VuoInputData(VuoReal, {"default":4.0, "suggestedMin":1.0, "suggestedMax":16.0, "suggestedStep":1.0, "name":"Beats per Measure"}) beatsPerMeasure,
	VuoInputEvent({"eventBlocking":"wall", "data":"beatsPerMeasure"}) beatsPerMeasureEvent,

	VuoInputData(VuoReal, {"default":-0.05, "suggestedMin":-0.1, "suggestedMax":0.1, "suggestedStep":0.01}) timeOffset,
	VuoInputEvent({"eventBlocking":"wall", "data":"timeOffset"}) timeOffsetEvent,

	VuoOutputData(VuoInteger) peers,
	VuoOutputEvent({"data":"peers"}) peersEvent,

	VuoOutputData(VuoReal) tempo,
	VuoOutputEvent({"data":"tempo"}) tempoEvent,

	VuoOutputEvent() started,
	VuoOutputEvent() stopped,

	VuoOutputEvent() beat,

	VuoOutputData(VuoReal) beats,
	VuoOutputEvent({"data":"beats"}) beatsEvent,

	VuoOutputEvent() measure,

	VuoOutputData(VuoReal) measures,
	VuoOutputEvent({"data":"measures"}) measuresEvent,

	VuoInstanceData(struct nodeInstanceData *) context)
{
	VuoAbletonLinkData linkData = VuoAbletonLink_getCurrent((*context)->link, -timeOffset * USEC_PER_SEC);

	if (setTempoEvent)
		VuoAbletonLink_setTempo((*context)->link, setTempo);

	VuoAbletonLink_setQuantum((*context)->link, beatsPerMeasure);

	if (startTransport)
		VuoAbletonLink_setPlaying((*context)->link, true);

	if (stopTransport)
		VuoAbletonLink_setPlaying((*context)->link, false);


	// Apply changes to the Ableton Link network session anytime,
	// but only allow data/events through when we receive a time event.
	if (!timeEvent)
		return;


	if ((*context)->firstEvent || linkData.peers != *peers)
	{
		*peers = linkData.peers;
		*peersEvent = true;
	}

	if ((*context)->firstEvent || linkData.tempo != *tempo)
	{
		*tempo = linkData.tempo;
		*tempoEvent = true;
	}

	if (linkData.isPlaying != (*context)->wasPlaying)
	{
		if (linkData.isPlaying)
			// We're in the period between
			// when a peer has requested to start transport-based playback
			// and when the next measure begins.
			(*context)->preroll = true;
		else
			*stopped = true;

		(*context)->wasPlaying = linkData.isPlaying;
	}


	VuoReal measureDelta = (linkData.sessionPhase - (*context)->priorSessionPhase) / beatsPerMeasure;
	if (measureDelta < 0 && (*context)->preroll)
	{
		(*context)->preroll = false;
		*started = true;
	}


	bool allowingBeatMeasureEvents =
		// `Always`:
		allowEvents == 0 ? true
		// For `When Transport Running`, only allow while the transport playback is active, excluding preroll.
		: (linkData.isPlaying && !(*context)->preroll);


	VuoReal newBeatPosition = fmod(linkData.beats, 1);
	VuoReal beatDelta = newBeatPosition - (*context)->priorBeatPosition;
	if (beatDelta < 0)
	{
		if (allowingBeatMeasureEvents)
			*beat = true;
		*beats += beatDelta + 1;
	}
	else
		*beats += beatDelta;


	if (measureDelta < 0)
	{
		// We're starting a new measure:
		if (allowingBeatMeasureEvents)
			*measure = true;

		if (!isfinite(measureDelta) || *started)
		{
			// We're starting the first measure (at the start of the composition or transport),
			// so reset the accumulators:
			*beats = newBeatPosition;
			*measures = *beats / beatsPerMeasure;
		}
		else
		{
			// We're starting a subsequent measure:
			*measures += measureDelta + 1;
		}
	}
	else
	{
		// We're continuing the current measure:
		*measures += measureDelta;
	}

	if (allowingBeatMeasureEvents)
	{
		*beatsEvent = true;
		*measuresEvent = true;
	}

	VDebugLog("link{bt=%6.3f snΦ=%5.3f}  ctx{bt₀=%5.3f snΦ₀=%5.3f%s}  msrΔ=%6.3f  out{bt=%6.3f msr=%7.3f%s%s%s%s%s%s}",
		linkData.beats,
		linkData.sessionPhase,
		(*context)->priorBeatPosition,
		(*context)->priorSessionPhase,
		(*context)->preroll ? " preroll" : "",
		measureDelta,
		*beats,
		*measures,
		*peersEvent ? " peers" : "",
		*tempoEvent ? " tempo" : "",
		*started    ? " start" : "",
		*stopped    ? " stop"  : "",
		*beat       ? " beat"  : "",
		*measure    ? " msr"   : "");

	(*context)->priorBeatPosition = newBeatPosition;
	(*context)->priorSessionPhase = linkData.sessionPhase;
	(*context)->firstEvent = false;
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) context)
{
	VuoAbletonLink_fini((*context)->link);
}

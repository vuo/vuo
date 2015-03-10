/**
 * @file
 * VuoInputEditorWave implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorWave.hh"

extern "C"
{
	#include "VuoWave.h"
}

/**
 * Constructs a VuoInputEditorWave object.
 */
VuoInputEditor * VuoInputEditorWaveFactory::newInputEditor()
{
	return new VuoInputEditorWave();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorWave::setUpMenuTree()
{
	VuoInputEditorMenuItem *waveTree = new VuoInputEditorMenuItem("root");

	json_object *sineAsJson = VuoWave_jsonFromValue(VuoWave_Sine);
	const char *sineSummary = VuoWave_summaryFromValue(VuoWave_Sine);

	json_object *triangleAsJson = VuoWave_jsonFromValue(VuoWave_Triangle);
	const char *triangleSummary = VuoWave_summaryFromValue(VuoWave_Triangle);

	json_object *sawtoothAsJson = VuoWave_jsonFromValue(VuoWave_Sawtooth);
	const char *sawtoothSummary = VuoWave_summaryFromValue(VuoWave_Sawtooth);


	VuoInputEditorMenuItem *sine = new VuoInputEditorMenuItem(sineSummary, sineAsJson);
	VuoInputEditorMenuItem *triangle = new VuoInputEditorMenuItem(triangleSummary, triangleAsJson);
	VuoInputEditorMenuItem *sawtooth = new VuoInputEditorMenuItem(sawtoothSummary, sawtoothAsJson);

	waveTree->addItem(sine);
	waveTree->addItem(triangle);
	waveTree->addItem(sawtooth);

	return waveTree;
}

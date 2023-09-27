/**
 * @file
 * VuoInputEditorWave implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoInputEditorWave.hh"

extern "C"
{
	#include "VuoWave.h"
}

#include "VuoInputEditorCurveRenderer.hh"

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

	waveTree->addItem(new VuoInputEditorMenuItem(VuoWave_getSummary(VuoWave_Sine),     VuoWave_getJson(VuoWave_Sine),     renderMenuIconWithWave(VuoWave_Sine,     isInterfaceDark())));
	waveTree->addItem(new VuoInputEditorMenuItem(VuoWave_getSummary(VuoWave_Triangle), VuoWave_getJson(VuoWave_Triangle), renderMenuIconWithWave(VuoWave_Triangle, isInterfaceDark())));
	waveTree->addItem(new VuoInputEditorMenuItem(VuoWave_getSummary(VuoWave_Sawtooth), VuoWave_getJson(VuoWave_Sawtooth), renderMenuIconWithWave(VuoWave_Sawtooth, isInterfaceDark())));

	return waveTree;
}

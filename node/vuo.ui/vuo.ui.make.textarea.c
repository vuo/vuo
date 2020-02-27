/**
 * @file
 * vuo.ui.textarea node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoUiTheme.h"
#include "VuoList_VuoUiTheme.h"
#include "VuoRenderedLayers.h"
#include "../vuo.keyboard/VuoKeyboard.h"
#include "VuoEventLoop.h"
#include <dispatch/dispatch.h>
#include "VuoTextField.h"

VuoModuleMetadata({
					  "title" : "Make Text Area",
					  "keywords" : [
						  "theme", "interface", "ui", "interact",
						  "input", "write", "submit", "type", "edit"
					  ],
					  "version" : "1.0.0",
					  "dependencies" : [ "VuoKeyboard", "VuoClipboard", "VuoTextField" ],
					  "node" : {
 						  "isDeprecated": true, // Not ready for release yet. https://b33p.net/kosada/node/11630
						  "exampleCompositions" : [ ]
					  }
				 });

typedef struct
{
	VuoTextField textField;
	VuoKeyboard* keyboardListener;
	bool isFirstEvent;
	void (*updatedLayer)(VuoLayer layer);
	void (*updatedText)(VuoText text);
} NodeInstanceData;

NodeInstanceData* nodeInstanceInit()
{
	NodeInstanceData* instance = (NodeInstanceData*)calloc(1, sizeof(NodeInstanceData));
	VuoRegister(instance, free);

	instance->isFirstEvent = true;
	instance->textField = VuoTextField_make(2);
	VuoRetain(instance->textField);
	instance->keyboardListener = VuoKeyboard_make();
	VuoRetain(instance->keyboardListener);

	return instance;
}

void nodeInstanceTriggerStart(  VuoInstanceData(NodeInstanceData*) instance,
								VuoInputData(VuoRenderedLayers) renderedLayers,
								VuoInputData(VuoInteger) lineCount,
								VuoOutputTrigger(updatedText, VuoText),
								VuoOutputTrigger(updatedLayer, VuoLayer))
{
	(*instance)->updatedLayer = updatedLayer;
	(*instance)->updatedText = updatedText;
	VuoTextField_setLineCount((*instance)->textField, lineCount);
	VuoWindowReference window = NULL;
	(void)VuoRenderedLayers_getWindow(renderedLayers, &window);

	VuoKeyboard_startListeningForTypingWithCallback((*instance)->keyboardListener,
										NULL,
										NULL,
										^(VuoText c, VuoModifierKey m)
										{
											VuoTextField_onTypedCharacter((*instance)->textField, c, m);
											(*instance)->updatedLayer(VuoTextField_createTextLayer((*instance)->textField));
											(*instance)->updatedText(VuoTextField_getText((*instance)->textField));
										},
										window);
}

void nodeInstanceTriggerStop(   VuoInstanceData(NodeInstanceData*) instance,
								VuoOutputTrigger(updatedLayer, VuoLayer))
{
	VuoKeyboard_stopListening((*instance)->keyboardListener);
}

void nodeInstanceEvent
(
	VuoInstanceData(NodeInstanceData*) instance,
	VuoInputData(VuoRenderedLayers) renderedLayers,
	VuoInputEvent({"eventBlocking":"wall", "data":"renderedLayers"}) renderedLayersEvent,
	VuoInputData(VuoText) setValue,
	VuoInputEvent({"data":"setValue"}) setValueEvent,
	VuoInputData(VuoText) placeholderText,
	VuoInputData(VuoColor, {"default":{"r":1, "g":1, "b":1, "a":1}}) cursorColor,
	VuoInputData(VuoAnchor, {"default":{"horizontalAlignment":"center", "verticalAlignment":"center"}}) anchor,
	VuoInputData(VuoPoint2d, {"default":{"x":0, "y":0}}) position,
	VuoInputData(VuoReal, {"default":1.0}) width,
	VuoInputData(VuoUiTheme) theme,
	VuoOutputTrigger(updatedValue, VuoText),
	VuoOutputTrigger(updatedLayer, VuoLayer)
)
{
	bool doRebuildLayer = false;
	bool textWasSet = (*instance)->isFirstEvent || setValueEvent;

	if(!renderedLayersEvent || textWasSet)
	{
		doRebuildLayer = true;

		if(setValueEvent || (*instance)->isFirstEvent)
		{
			(*instance)->isFirstEvent = false;
			VuoTextField_setText((*instance)->textField, setValue);
		}
		VuoTextField_setPlaceholderText((*instance)->textField, placeholderText);
		VuoTextField_setCursorColor((*instance)->textField, cursorColor);
		VuoTextField_setLayerPosition((*instance)->textField, position);
		VuoTextField_setLayerWidth((*instance)->textField, width);
		VuoTextField_setLayerAnchor((*instance)->textField, anchor);
		VuoTextField_setTheme((*instance)->textField, theme);
	}
	else
	{
		doRebuildLayer = VuoTextField_onRenderedLayers((*instance)->textField, &renderedLayers);
	}

	if(doRebuildLayer)
		updatedLayer( VuoTextField_createTextLayer((*instance)->textField) );

	if(textWasSet)
		updatedValue( VuoTextField_getText((*instance)->textField) );
}

void nodeInstanceFini( VuoInstanceData(NodeInstanceData*) instance )
{
	VuoRelease((*instance)->textField);
	VuoRelease((*instance)->keyboardListener);
}


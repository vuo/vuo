/**
 * @file
 * VuoWindow interface.
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
#include "VuoRenderedLayers.h"

#include <stdint.h>

/**
 * A window containing a text edit widget.
 */
typedef void * VuoWindowText;

/**
 * A window containing an OpenGL view.
 */
typedef void * VuoWindowOpenGl;

VuoWindowText VuoWindowText_make(void);
void VuoWindowText_enableTriggers
(
		VuoWindowText w,
		VuoOutputTrigger(typedLine, VuoText),
		VuoOutputTrigger(typedWord, VuoText),
		VuoOutputTrigger(typedCharacter, VuoText)
);
void VuoWindowText_disableTriggers(VuoWindowText w);
void VuoWindowText_appendLine(VuoWindowText w, const char *text);
void VuoWindowText_clear(VuoWindowText w);
void VuoWindowText_close(VuoWindowText w);

VuoWindowOpenGl VuoWindowOpenGl_make
(
		void (*initCallback)(void *, float backingScaleFactor),
		void (*updateBackingCallback)(void *, float backingScaleFactor),
		void (*resizeCallback)(void *, unsigned int, unsigned int),
		VuoIoSurface (*drawCallback)(void *),
		void *context
);
void VuoWindowOpenGl_enableTriggers
(
		VuoWindowOpenGl w,
		VuoOutputTrigger(updatedWindow, VuoRenderedLayers)
);
void VuoWindowOpenGl_enableTriggers_deprecated
(
		VuoWindowOpenGl w,
		VuoOutputTrigger(showedWindow, VuoWindowReference),
		VuoOutputTrigger(requestedFrame, VuoReal)
);
void VuoWindowOpenGl_disableTriggers(VuoWindowOpenGl w);
void VuoWindowOpenGl_redraw(VuoWindowOpenGl w);
void VuoWindowOpenGl_setProperties(VuoWindowOpenGl w, VuoList_VuoWindowProperty properties);
void VuoWindowOpenGl_setAspectRatio(VuoWindowOpenGl w, unsigned int pixelsWide, unsigned int pixelsHigh);
void VuoWindowOpenGl_unlockAspectRatio(VuoWindowOpenGl w);
void VuoWindowOpenGl_close(VuoWindowOpenGl w, void (^closedHandler)(void));

#ifdef __cplusplus
}
#endif

/**
 * @file
 * VuoWindow interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "node.h"

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

VuoWindowOpenGl VuoWindowOpenGl_make
(
		void (*initCallback)(void *),
		void (*resizeCallback)(void *, unsigned int, unsigned int),
		void (*drawCallback)(void *),
		void *context
);
void VuoWindowOpenGl_enableTriggers
(
		VuoWindowOpenGl w,
		VuoOutputTrigger(movedMouseTo, VuoPoint2d),
		VuoOutputTrigger(scrolledMouse, VuoPoint2d),
		VuoOutputTrigger(usedMouseButton, VuoMouseButtonAction)
);
void VuoWindowOpenGl_disableTriggers(VuoWindowOpenGl w);
void VuoWindowOpenGl_redraw(VuoWindowOpenGl w);
void VuoWindowOpenGl_setAspectRatio(VuoWindowOpenGl w, unsigned int pixelsWide, unsigned int pixelsHigh);

#ifdef __cplusplus
}
#endif

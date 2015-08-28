/**
 * @file
 * VuoInputEditorImageColorDepth interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORIMAGECOLORDEPTH_HH
#define VUOINPUTEDITORIMAGECOLORDEPTH_HH

#include "VuoInputEditorWithEnumMenu.hh"

/**
 * A VuoInputEditorImageColorDepth factory.
 */
class VuoInputEditorImageColorDepthFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorImageColorDepth.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays a menu of VuoImageColorDepth values.
 */
class VuoInputEditorImageColorDepth : public VuoInputEditorWithEnumMenu
{
	Q_OBJECT
};

#endif // VUOINPUTEDITORIMAGECOLORDEPTH_HH

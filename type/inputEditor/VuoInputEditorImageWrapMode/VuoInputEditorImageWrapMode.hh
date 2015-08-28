/**
 * @file
 * VuoInputEditorImageWrapMode interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORIMAGEWRAPMODE_HH
#define VUOINPUTEDITORIMAGEWRAPMODE_HH

#include "VuoInputEditorWithEnumMenu.hh"

/**
 * A VuoInputEditorImageWrapMode factory.
 */
class VuoInputEditorImageWrapModeFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorImageWrapMode.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays a menu of VuoImageWrapMode values.
 */
class VuoInputEditorImageWrapMode : public VuoInputEditorWithEnumMenu
{
	Q_OBJECT
};

#endif // VuoInputEditorImageWrapMode_HH

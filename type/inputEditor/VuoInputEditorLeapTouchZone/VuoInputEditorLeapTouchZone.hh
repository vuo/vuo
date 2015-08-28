/**
 * @file
 * VuoInputEditorLeapTouchZone interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORLEAPTOUCHZONE_HH
#define VUOINPUTEDITORLEAPTOUCHZONE_HH

#include "VuoInputEditorWithEnumMenu.hh"

/**
 * A VuoInputEditorLeapTouchZone factory.
 */
class VuoInputEditorLeapTouchZoneFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorLeapTouchZone.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays a menu of VuoLeapTouchZone values.
 */
class VuoInputEditorLeapTouchZone : public VuoInputEditorWithEnumMenu
{
	Q_OBJECT
};

#endif // VUOINPUTEDITORLEAPTOUCHZONE_HH

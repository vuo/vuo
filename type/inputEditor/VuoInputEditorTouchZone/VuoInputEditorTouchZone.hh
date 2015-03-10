/**
 * @file
 * VuoInputEditorPointableType interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORTOUCHZONE_HH
#define VUOINPUTEDITORTOUCHZONE_HH

#include "VuoInputEditorWithMenu.hh"

/**
 * A VuoInputEditorTouchZone factory.
 */
class VuoInputEditorTouchZoneFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorTouchZone.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays a menu of VuoLeapPointableType values.
 */
class VuoInputEditorTouchZone : public VuoInputEditorWithMenu
{
	Q_OBJECT

public:
	VuoInputEditorMenuItem * setUpMenuTree();
};

#endif // VUOINPUTEDITORTOUCHZONE_HH

/**
 * @file
 * VuoInputEditorCurve interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORCURVE_HH
#define VUOINPUTEDITORCURVE_HH

#include "VuoInputEditorWithMenu.hh"

/**
 * A VuoInputEditorCurve factory.
 */
class VuoInputEditorCurveFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorCurve.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays a menu of VuoCurve values.
 */
class VuoInputEditorCurve : public VuoInputEditorWithMenu
{
	Q_OBJECT

public:
	VuoInputEditorMenuItem * setUpMenuTree();
};

#endif // VUOINPUTEDITORCURVE_HH

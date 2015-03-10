/**
 * @file
 * VuoInputEditorCurveDomain interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORCURVEDOMAIN_HH
#define VUOINPUTEDITORCURVEDOMAIN_HH

#include "VuoInputEditorWithMenu.hh"

/**
 * A VuoInputEditorCurveDomain factory.
 */
class VuoInputEditorCurveDomainFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorCurveDomain.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays a menu of VuoCurveDomain values.
 */
class VuoInputEditorCurveDomain : public VuoInputEditorWithMenu
{
	Q_OBJECT

public:
	VuoInputEditorMenuItem * setUpMenuTree();
};

#endif // VUOINPUTEDITORCURVEDOMAIN_HH

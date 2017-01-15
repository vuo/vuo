/**
 * @file
 * VuoInputEditorRealRegulation interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORREALREGULATION_HH
#define VUOINPUTEDITORREALREGULATION_HH

#include "VuoInputEditorWithLineEditList.hh"

extern "C"
{
	#include "VuoText.h"
	#include "VuoList_VuoText.h"
}

/**
 * A VuoInputEditorRealRegulation factory.
 */
class VuoInputEditorRealRegulationFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorRealRegulation.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   VuoInputEditor *newInputEditor(void);
};

/**
 * An input editor for real regulations.
 */
class VuoInputEditorRealRegulation : public VuoInputEditorWithLineEditList
{
	Q_OBJECT

public:
	VuoInputEditorRealRegulation(void);

private:
	QLayout *setUpRow(QDialog &dialog, QLineEdit *lineEdit);
	QList<QString> convertToLineEditListFormat(json_object *value);
	json_object *convertFromLineEditListFormat(const QList<QString> &lineEditTexts);
	int row;
};

#endif

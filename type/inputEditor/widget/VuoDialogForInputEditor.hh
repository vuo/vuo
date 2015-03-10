/**
 * @file
 * VuoDialogForInputEditor interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUODIALOGFORINPUTEDITOR_HH
#define VUODIALOGFORINPUTEDITOR_HH

/**
 * A dialog that closes when the Enter key is pressed.
 */
class VuoDialogForInputEditor : public QDialog
{
	Q_OBJECT

public slots:
	virtual void keyPressEvent(QKeyEvent *e);
	virtual bool event(QEvent *e);
};

#endif // VUODIALOGFORINPUTEDITOR_HH

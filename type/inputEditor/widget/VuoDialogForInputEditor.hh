/**
 * @file
 * VuoDialogForInputEditor interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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

public:
	VuoDialogForInputEditor(bool isDark, bool showArrow);
	QMargins getPopoverContentsMargins(void) const;
	QSize sizeHint(void) const;

public slots:
	virtual void keyPressEvent(QKeyEvent *e);
	virtual bool event(QEvent *e);

private:
	QPainterPath getPopoverPath(void);
	void paintEvent(QPaintEvent *event);
	void showEvent(QShowEvent *event);
	static const int popoverArrowHalfWidth;
	int arrowPixelsFromTopOrLeft;
	bool _isDark;
	bool _showArrow;
};

#endif // VUODIALOGFORINPUTEDITOR_HH

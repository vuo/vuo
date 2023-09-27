/**
 * @file
 * VuoDocumentationSidebar interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * Inspector-style sidebar for displaying documentation. It can be shown and hidden.
 */
class VuoDocumentationSidebar : public QDockWidget
{
	Q_OBJECT

public:
	VuoDocumentationSidebar(QWidget *parent=nullptr);
	QString getSelectedText();

private:
	void updateColor();
	static QString generateGlslIsfTitle();
	static QString generateGlslIsfText();

	QScrollArea *scrollArea;
	QLabel *label;
};

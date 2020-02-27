/**
 * @file
 * VuoCodeEditorStages interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoShaderFile.hh"

class VuoCodeEditor;

/**
 * Presents a @ref VuoCodeEditor in each tab, for each shader stage.
 */
class VuoCodeEditorStages : public QTabWidget
{
	Q_OBJECT

public:
	explicit VuoCodeEditorStages(QWidget *parent, VuoShaderFile *shaderFile);

	VuoCodeEditor *someEditor();
	void updatePosition();

	VuoCodeEditor *currentEditor();
	VuoShaderFile::Stage currentStage();
	void switchToStage(VuoShaderFile::Stage stage);

	bool modified();
	void setUnmodified();

	void zoom11();
	void zoomIn();
	void zoomOut();

	VuoCodeEditor *vertex;    ///< The vertex shader widget.
	VuoCodeEditor *geometry;  ///< The geometry shader widget.
	VuoCodeEditor *fragment;  ///< The fragment shader widget.

signals:
	void modificationMayHaveChanged(void);  ///< Emitted when the content of any code editor changes in a way that affects its "modified" status (though not necessarily the "modified" status of this widget as a whole).

private:
	void keyPressEvent(QKeyEvent *event);
	void updateColor(bool isDark);
};

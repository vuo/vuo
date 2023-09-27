/**
 * @file
 * VuoCodeEditorStages implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCodeEditorStages.hh"

#include "VuoCodeEditor.hh"
#include "VuoCodeGutter.hh"
#include "VuoEditor.hh"
#include "VuoRendererColors.hh"

/**
 * Creates a stages widget.
 */
VuoCodeEditorStages::VuoCodeEditorStages(QWidget *parent, VuoShaderFile *shaderFile)
	: QTabWidget(parent)
{
//	vertex = new VuoCodeEditor(QString::fromStdString(shaderFile->vertexSource()));
//	addTab(vertex,   "Vertex Shader");
	vertex = NULL;

//	if (shaderFile->typeAllowsGeometryShader())
//	{
//		geometry = new VuoCodeEditor(QString::fromStdString(shaderFile->geometrySource()));
//		addTab(geometry, "Geometry Shader");
//	}
//	else
		geometry = NULL;

//	if (shaderFile->typeAllowsFragmentShader())
//	{
		fragment = new VuoCodeEditor(QString::fromStdString(shaderFile->fragmentSource()));
		addTab(fragment, "Fragment Shader");
//	}
//	else
//		fragment = NULL;

	VuoShaderFile::Type type = shaderFile->type();
	if (type == VuoShaderFile::GLSLImageFilter
	 || type == VuoShaderFile::GLSLImageGenerator
	 || type == VuoShaderFile::GLSLImageTransition
	 || type == VuoShaderFile::GLSLObjectRenderer)
	{
		setCurrentWidget(fragment);
		fragment->setFocus();
	}
	else // if (type == VuoShaderFile::GLSLObjectFilter)
	{
		setCurrentWidget(vertex);
		vertex->setFocus();
	}

	if (vertex)
		connect(vertex->document(), &QTextDocument::modificationChanged, this, &VuoCodeEditorStages::modificationMayHaveChanged);
	if (geometry)
		connect(geometry->document(), &QTextDocument::modificationChanged, this, &VuoCodeEditorStages::modificationMayHaveChanged);
	if (fragment)
		connect(fragment->document(), &QTextDocument::modificationChanged, this, &VuoCodeEditorStages::modificationMayHaveChanged);

	VuoEditor *editor = static_cast<VuoEditor *>(qApp);
	connect(editor, &VuoEditor::darkInterfaceToggled, this, &VuoCodeEditorStages::updateColor);
	updateColor(editor->isInterfaceDark());

	connect(this, &QTabWidget::currentChanged, this, &VuoCodeEditorStages::updatePosition);
}

/**
 * Returns any non-NULL code editor.  Useful if you just need colors.
 */
VuoCodeEditor *VuoCodeEditorStages::someEditor()
{
	return vertex ? vertex : (geometry ? geometry : fragment);
}

/**
 * Returns true if any code editor's contents have been modified by the user.
 */
bool VuoCodeEditorStages::modified()
{
	return (vertex && vertex->document()->isModified()) ||
			(geometry && geometry->document()->isModified()) ||
			(fragment && fragment->document()->isModified());
}

/**
 * Marks each code editor as not having been modified by the user.
 */
void VuoCodeEditorStages::setUnmodified()
{
	if (vertex)
		vertex->document()->setModified(false);
	if (geometry)
		geometry->document()->setModified(false);
	if (fragment)
		fragment->document()->setModified(false);
}

/**
 * Restores the default text size (100% aka 1:1).
 */
void VuoCodeEditorStages::zoom11()
{
	if (vertex)
		vertex->zoom11();
	if (geometry)
		geometry->zoom11();
	if (fragment)
		fragment->zoom11();
}

/**
 * Makes the text larger.
 */
void VuoCodeEditorStages::zoomIn()
{
	if (vertex)
		vertex->zoomIn();
	if (geometry)
		geometry->zoomIn();
	if (fragment)
		fragment->zoomIn();
}

/**
 * Makes the text smaller.
 */
void VuoCodeEditorStages::zoomOut()
{
	if (vertex)
		vertex->zoomOut();
	if (geometry)
		geometry->zoomOut();
	if (fragment)
		fragment->zoomOut();
}

void VuoCodeEditorStages::keyPressEvent(QKeyEvent *event)
{
	int key = event->key();
	Qt::KeyboardModifiers modifiers = event->modifiers();

	if ( (key == Qt::Key_BracketRight && modifiers & Qt::ControlModifier && modifiers & Qt::ShiftModifier)  // Safari: cmd-shift-]
	  || (key == Qt::Key_Right        && modifiers & Qt::ControlModifier && modifiers & Qt::AltModifier  )) // Chrome: cmd-option-rightarrow
		setCurrentIndex(currentIndex() + 1);
	else if ( (key == Qt::Key_BracketLeft && modifiers & Qt::ControlModifier && modifiers & Qt::ShiftModifier)
		   || (key == Qt::Key_Left        && modifiers & Qt::ControlModifier && modifiers & Qt::AltModifier  ))
		setCurrentIndex(currentIndex() - 1);
	else
		QTabWidget::keyPressEvent(event);
}

void VuoCodeEditorStages::updateColor(bool isDark)
{
	if (vertex)
		vertex->updateColor(isDark);
	if (geometry)
		geometry->updateColor(isDark);
	if (fragment)
		fragment->updateColor(isDark);

	VuoCodeEditor *e = someEditor();

	VuoRendererColors colors;
	colors.setDark(isDark);

	QColor tabBackground = e->gutterColor.lighter(isDark ? 90 : 105);
	QColor tabBorder = e->gutterColor.lighter(isDark ? 110 : 97);

	VuoCodeEditor *codeEditor = static_cast<VuoCodeEditor *>(currentWidget());
	int gutterWidth = codeEditor->gutter->width();

	setStyleSheet(VUO_QSTRINGIFY(
		QTabWidget::pane {
			background: %1;
		}

		QTabWidget::tab-bar {
			left: %8px;
		}

		QTabBar::tab {
			padding: 2px 12px 2px 12px;
			border: 1px solid %7;
			margin-right: -1px;
		}

		QTabBar::tab:selected {
			background: %3;
			color: %4;
			border-top: 2px solid %2;
			border-bottom: 1px solid %3;
			border-top-left-radius: 4px;
			border-top-right-radius: 4px;
		}

		QTabBar::tab:last {
			margin-right: 0;
		}

		QTabBar::tab:!selected {
			margin-top: 4px;
			background: %6;
			color: %5;
		}

		VuoCodeEditor {
			border: none;
			border-bottom: 1px solid %1;
		})
	.arg(e->gutterColor.name())
	.arg(colors.selection().name())
	.arg(colors.canvasFill().name())
	.arg(e->palette().text().color().name())
	.arg(colors.nodeFrame().name())
	.arg(tabBackground.name())
	.arg(tabBorder.name())
	.arg(gutterWidth - 1)
	);
}

/**
 * Updates the left margin to ensure it lines up with the gutter.
 */
void VuoCodeEditorStages::updatePosition()
{
	VuoEditor *editor = static_cast<VuoEditor *>(qApp);
	// To update part of the stylesheet we unfortunately need to update the whole stylesheet.
	updateColor(editor->isInterfaceDark());
}

/**
 * Returns the currently-visible code editor.
 */
VuoCodeEditor *VuoCodeEditorStages::currentEditor()
{
	return static_cast<VuoCodeEditor *>(currentWidget());
}

/**
 * Returns the currently-visible stage.
 */
VuoShaderFile::Stage VuoCodeEditorStages::currentStage()
{
	if (currentWidget() == vertex)
		return VuoShaderFile::Vertex;
	else if (currentWidget() == geometry)
		return VuoShaderFile::Geometry;
	else // if (currentWidget() == fragment)
		return VuoShaderFile::Fragment;
}

/**
 * Makes the specified stage visible.
 */
void VuoCodeEditorStages::switchToStage(VuoShaderFile::Stage stage)
{
	if (stage == VuoShaderFile::Vertex)
		setCurrentWidget(vertex);
	else if (stage == VuoShaderFile::Geometry)
		setCurrentWidget(geometry);
	else if (stage == VuoShaderFile::Fragment)
		setCurrentWidget(fragment);
}

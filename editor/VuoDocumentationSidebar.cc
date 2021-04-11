/**
 * @file
 * VuoDocumentationSidebar implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoDocumentationSidebar.hh"

#include "VuoEditor.hh"
#include "VuoNodePopover.hh"
#include "VuoRendererFonts.hh"
#include "VuoStringUtilities.hh"

/**
 * Creates a sidebar that can be added to a window. The sidebar displays the GLSL/ISF Quick Reference.
 */
VuoDocumentationSidebar::VuoDocumentationSidebar(QWidget *parent) :
	QDockWidget(parent)
{
	setFeatures(QDockWidget::DockWidgetClosable);

	setWindowTitle(generateGlslIsfTitle());

	scrollArea = new QScrollArea(this);
	scrollArea->setWidgetResizable(true);
	setWidget(scrollArea);

	label = new QLabel(this);
	label->setFont(VuoRendererFonts::getSharedFonts()->portPopoverFont());
	label->setMargin(8);
	label->setWordWrap(true);
	label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
	label->setOpenExternalLinks(true);
	scrollArea->setWidget(label);

	connect(static_cast<VuoEditor *>(qApp), &VuoEditor::darkInterfaceToggled, this, &VuoDocumentationSidebar::updateColor);
	updateColor();
}

/**
 * Sets the content and style of the sidebar based on the current app-wide settings.
 */
void VuoDocumentationSidebar::updateColor()
{
	bool isDark = static_cast<VuoEditor *>(qApp)->isInterfaceDark();

	/// @todo Cloned from VuoNodeLibrary::updateColor(), VuoPublishedPortSidebar::updateColor(), VuoCompositionMetadataPanel::paintEvent()
	QString titleTextColor            = isDark ? "#303030" : "#808080";
	QString titleBackgroundColor      = isDark ? "#919191" : "#efefef";
	QString contentBackgroundColor    = isDark ? "#282828" : "#f9f9f9";
	setStyleSheet(VUO_QSTRINGIFY(
					  QDockWidget {
						  titlebar-close-icon: url(:/Icons/dockwidget-close-%4.png);
						  font-size: 11px;
						  border: none;
						  color: %1;
					  }
					  QDockWidget::title {
						  text-align: left;
						  padding-left: 6px;
						  background-color: %2;
					  }
					  QScrollArea {
						  border-left: 1px solid %2;
						  border-right: none;
						  border-top: none;
						  border-bottom: none;
					  }
					  QLabel {
						  background-color: %3;
					  }
				  )
				  .arg(titleTextColor)
				  .arg(titleBackgroundColor)
				  .arg(contentBackgroundColor)
				  .arg(isDark ? "dark" : "light")
				  );

	QString content = VuoNodePopover::generateTextStyleString() +
					  generateGlslIsfText();
	label->setText(content);
}

/**
 * Returns the sidebar and action title.
 */
QString VuoDocumentationSidebar::generateGlslIsfTitle()
{
	return "GLSL/ISF Quick Reference";
}

/**
 * Returns the sidebar content.
 */
QString VuoDocumentationSidebar::generateGlslIsfText()
{
	QFile file(QApplication::applicationDirPath().append("/../Resources/GlslIsfQuickReference.md"));
	file.open(QIODevice::ReadOnly);
	QTextStream textStream(&file);
	string markdown = textStream.readAll().toStdString();
	string html = VuoStringUtilities::generateHtmlFromMarkdown(markdown);
	return QString::fromStdString(html);
}

/**
 * Returns the currently selected text within the sidebar (if any).
 */
QString VuoDocumentationSidebar::getSelectedText()
{
	return label->selectedText();
}

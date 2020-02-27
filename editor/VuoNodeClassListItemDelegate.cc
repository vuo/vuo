/**
 * @file
 * VuoNodeClassListItemDelegate implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoNodeClassListItemDelegate.hh"
#include "VuoNodeClassListItemHighlighter.hh"
#include "VuoEditor.hh"

const int VuoNodeClassListItemDelegate::humanReadableNameIndex = Qt::UserRole;
const int VuoNodeClassListItemDelegate::classNameIndex = Qt::UserRole+1;
const int VuoNodeClassListItemDelegate::nodeClassPointerIndex = Qt::UserRole+2;
const int VuoNodeClassListItemDelegate::baseTitleFontPt = 12;
const int VuoNodeClassListItemDelegate::disambiguatingTitleFontPt = 10;

/**
 * Creates a Node Library class list rendering delegate.
 */
VuoNodeClassListItemDelegate::VuoNodeClassListItemDelegate(QObject *parent) :
	QStyledItemDelegate(parent)
{
	humanReadable = false;
	filterText = "";
}

/**
 * Display a given node class name within the node library.
 */
void VuoNodeClassListItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	paintTextDocument(painter, option, index);
}

/**
 * Displays a node class name within the node library, making use of the QTextEdit,
 * QSyntaxHighlighter, and QTextDocument classes, and QTextDocument::drawContents().
 * This method supports emboldening of search tokens within matched node names.
 */
void VuoNodeClassListItemDelegate::paintTextDocument(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	painter->save();

	// Customize display for currently selected node classes.
	QColor baseTextColor, disambiguatingTitleFontColor;
	setUpSelectionHighlighting(painter, baseTextColor, disambiguatingTitleFontColor, option, index);

	// Generate a text document containing the node title.
	QTextDocument *doc = generateTextDocument(index, baseTextColor, disambiguatingTitleFontColor);

	// Display the node name.
	QAbstractTextDocumentLayout::PaintContext context;
	QRect r = option.rect;
	painter->setClipRect(r);

	// Center node name vertically within row.
	int h = /*doc->size().height()*/ baseTitleFontPt * 2;
	int yAdjusted = r.y() + r.size().height()/2 - h/2;

	painter->translate(r.x(), yAdjusted);
	doc->drawContents(painter);

	delete doc;
	painter->restore();
}

/**
 * Sets the node class text and background colors appropriately, depending whether
 * the node class is currently selected within the list.
 */
void VuoNodeClassListItemDelegate::setUpSelectionHighlighting(QPainter *painter, QColor &baseTextColor, QColor &disambiguatingTitleFontColor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	VuoEditor *editor = (VuoEditor *)qApp;
	bool isDark = editor->isInterfaceDark();
	if (option.state & QStyle::State_Selected)
	{
		bool isActive = option.state & QStyle::State_Active;
		painter->fillRect(option.rect, isDark ? QColor(isActive ? "#1d6ae5" : "#606060") : QColor(isActive ? "#74acec" : "#e0e0e0"));

		QPalette::ColorGroup cg = isActive ? QPalette::Normal : QPalette::Inactive;
		QColor textColor = option.palette.color(cg, QPalette::HighlightedText);
		baseTextColor                = isDark ? QColor("#dadada") : textColor;
		disambiguatingTitleFontColor = isDark ? QColor("#909090") : textColor;
	}
	else
	{
		// Highlight alternating rows.
		if (index.row() & 1)
			painter->fillRect(option.rect, isDark ? QColor("#2b2b2b") : QColor("#f8f8f8"));

		baseTextColor                = QColor(isDark ? "#a0a0a0" : "#606060");
		disambiguatingTitleFontColor = QColor(isDark ? "#606060" : "#b0b0b0");
	}
}

/**
 * Generates a QTextEdit object containing the display text for the node specified
 * by @c index; returns a pointer to the generated document.
 */
QTextDocument *VuoNodeClassListItemDelegate::generateTextDocument(const QModelIndex &index, QColor baseTextColor, QColor disambiguatingTitleFontColor) const
{
	const QColor baseTitleFontColor = baseTextColor;

	// Retrieve node name data.
	QString humanReadableName = index.data(VuoNodeClassListItemDelegate::humanReadableNameIndex).toString();
	QString className = index.data(VuoNodeClassListItemDelegate::classNameIndex).toString();
	QString baseTitle = (humanReadable? humanReadableName:className);

	QTextOption to;
	to.setWrapMode(QTextOption::NoWrap);
	QTextDocument *document = new QTextDocument();
	document->setDefaultTextOption(to);

	// Locate text filter tokens within the node name for highlighting.
	VuoNodeClassListItemHighlighter *highlighter = new VuoNodeClassListItemHighlighter(document);
	QStringList tokenizedTextFilter = filterText.split(QRegExp("[\\s\\.]+"));
	for (int tokenIndex = 0; tokenIndex < tokenizedTextFilter.size(); ++tokenIndex)
	{
		QString token = tokenizedTextFilter.at(tokenIndex);
		if (! token.isEmpty())
			highlighter->addTargetTerm(token);
	}

	// Indicate where the human-readable node name ends and the (optional) class name begins,
	// so the highlighter knows what symbol to tokenize on within each segment of the text.
	QString disambiguationSeparator = " ";
	highlighter->setNodeClassNameStartIndex(humanReadable? humanReadableName.length()+disambiguationSeparator.length():0);

	QString formattedBaseTitle = QString("<span style=\"color:%1; \"font-size:%2pt;\">%3</span>")
													.arg(baseTitleFontColor.name())
													.arg(baseTitleFontPt)
													.arg(baseTitle);
	QString fullText = formattedBaseTitle;

	// Display class names even in human-readable mode.
	if (humanReadable)
	{
		QString formattedDisambiguationText = QString("%1<span style=\"color:%2; font-size:%3pt;\">%4</span>")
																.arg(disambiguationSeparator)
																.arg(disambiguatingTitleFontColor.name())
																.arg(disambiguatingTitleFontPt)
																.arg(className);
		fullText.append(formattedDisambiguationText);
	}

	document->setHtml(fullText);

	return document;
}

/**
 * Returns the size needed to display the node name specified by @c index.
 */
QSize VuoNodeClassListItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QWidget *parentWidget = (QWidget *)parent();
	return QSize(parentWidget->width() - parentWidget->style()->pixelMetric(QStyle::PM_ScrollBarExtent) - 4,

				 // Instead of using `QStyledItemDelegate::sizeHint` (which inexplicably changes between app launches),
				 // calculate the line height based on the text size.
				 // https://b33p.net/kosada/node/13897
//				 QStyledItemDelegate::sizeHint(option, index).height()
				 baseTitleFontPt * 1.5);
}

/**
 * Returns a boolean indicating whether node class names are currently displayed in
 * "human-readable" mode (i.e., using default node titles rather than class names).
 */
bool VuoNodeClassListItemDelegate::getHumanReadable() const
{
	return humanReadable;
}

/**
 * Sets the boolean indicating whether node class names are currently displayed in
 * "human-readable" mode (i.e., using default node titles rather than class names).
 */
void VuoNodeClassListItemDelegate::setHumanReadable(bool humanReadable)
{
	this->humanReadable = humanReadable;
}

/**
 * Sets the filter text used to search for node classes within the library.
 */
void VuoNodeClassListItemDelegate::setFilterText(QString filterText)
{
	this->filterText = filterText;
}

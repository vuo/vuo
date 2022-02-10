/**
 * @file
 * VuoNodeClassListItemHighlighter implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoNodeClassListItemHighlighter.hh"

const QRegExp VuoNodeClassListItemHighlighter::nodeNameTokenDelimiter = QRegExp("\\s+");
const QRegExp VuoNodeClassListItemHighlighter::nodeClassNameTokenDelimiter = QRegExp("\\.");

/**
 * Creates a new Node Library class list highlighter.
 */
VuoNodeClassListItemHighlighter::VuoNodeClassListItemHighlighter(QObject *parent) :
	QSyntaxHighlighter(parent)
{
	nodeClassNameStartIndex = 0;
}

/**
 * Highlights target terms within the given text block.
 */
void VuoNodeClassListItemHighlighter::highlightBlock(const QString &text)
{
	QTextCharFormat highlightedTextFormat;
	highlightedTextFormat.setFontWeight(QFont::Bold);

	for (vector<QString>::iterator term = targetTerms.begin(); term != targetTerms.end(); ++term)
	{
		int length = (*term).length();
		QStringMatcher textFilterPattern((*term), Qt::CaseInsensitive);
		int index = textFilterPattern.indexIn(text);

		while (index >= 0)
		{
			// Only highlight matches that begin at token boundaries within the displayed node name.
			QRegExp currentDelimiter = (index < nodeClassNameStartIndex? nodeNameTokenDelimiter:nodeClassNameTokenDelimiter);
			if ((index == 0) ||
				(index == nodeClassNameStartIndex) ||
				(currentDelimiter.exactMatch(text.at(index-1))) ||
				((index > nodeClassNameStartIndex) &&
					// Transitions from lowercase to uppercase within the class name may be considered token boundaries.
					(QRegExp("([a-z0-9])").exactMatch(text.at(index-1)) && QRegExp("([A-Z])").exactMatch(text.at(index)))))
				setFormat(index, length, highlightedTextFormat);

			index = textFilterPattern.indexIn(text, index + length);
		 }
	}
}

/**
 * Adds @c term to the list of target terms to highlight.
 */
void VuoNodeClassListItemHighlighter::addTargetTerm(QString term)
{
	targetTerms.push_back(term);
}

/**
 * Designates @c index as the first position within the highlighted text that
 * corresponds to a node class name rather than a human-readable node title.
 * From this index onward, text will be tokenized on '.' rather than whitespace.
 */
void VuoNodeClassListItemHighlighter::setNodeClassNameStartIndex(int index)
{
	nodeClassNameStartIndex = index;
}

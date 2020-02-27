/**
 * @file
 * VuoNodeClassListItemHighlighter interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * Highlights (emboldens) target terms within the given text block.
 */
class VuoNodeClassListItemHighlighter : public QSyntaxHighlighter
{
	Q_OBJECT
public:
	explicit VuoNodeClassListItemHighlighter(QObject *parent = 0);
	void highlightBlock(const QString &text);
	void addTargetTerm(QString regExp);
	void setNodeClassNameStartIndex(int index);

signals:

public slots:

private:
	static const QRegExp nodeNameTokenDelimiter;
	static const QRegExp nodeClassNameTokenDelimiter;
	int nodeClassNameStartIndex;
	vector<QString> targetTerms;

};

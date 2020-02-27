/**
 * @file
 * VuoCompositionMetadataPanel interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoPanelDocumentation.hh"

class VuoComposition;

/**
 * A widget that shows information about the composition.
 */
class VuoCompositionMetadataPanel : public VuoPanelDocumentation
{
	Q_OBJECT
public:
	explicit VuoCompositionMetadataPanel(VuoComposition *composition, QWidget *parent = 0);
	~VuoCompositionMetadataPanel();
	QString generateCompositionMetadataText();
	QString getSelectedText();
	void update();
	void setIsUserComposition(bool userComposition);

public slots:
	void setTextWidth(int width);

signals:
	void metadataEditRequested(); ///< Emitted when the user has clicked the "Edit" link.

protected:
	void paintEvent(QPaintEvent *event);

private slots:
	void handleMetadataLinkClick(const QString &url);

private:
	VuoComposition *composition;
	QLabel *textLabel;
	bool isUserComposition;
	static const int defaultPopoverTextWidth; // @todo https://b33p.net/kosada/node/8613: Copied from VuoNodePopover.hh; re-factor.
	static const int margin; // @todo https://b33p.net/kosada/node/8613: Copied from VuoNodePopover.hh; re-factor.
	static const QString editLink; // The link to edit the composition metadata.
	QVBoxLayout *layout;

	// Style
	void setStyle();
};

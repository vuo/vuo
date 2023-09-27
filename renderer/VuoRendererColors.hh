/**
 * @file
 * VuoRendererColors interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoNode.hh"
#include "VuoRendererItem.hh"

/**
 * Provides colors for rendered items in a composition.
 */
class VuoRendererColors
{
public:
	/**
	 * Specifies the various levels of highlighting that may be applied to a given component.
	 */
	enum HighlightType
	{
		standardHighlight,   ///< Cable is being dragged, and port is eligible for direct connection.
		subtleHighlight,     ///< Cable is being dragged, and port is eligible for connection via typecast.
		ineligibleHighlight, ///< Cable is being dragged, but port is not eligible for connection.
		noHighlight          ///< Cable is not being dragged.
	};

	/**
	 * Specifies the various levels of selection coloring that may be applied to a given component.
	 */
	enum SelectionType
	{
		directSelection,
		indirectSelection,
		noSelection
	};

	static VuoRendererColors *getSharedColors(void);
	VuoRendererColors(VuoNode::TintColor tintColor = VuoNode::TintNone,
					  VuoRendererColors::SelectionType selectionType = VuoRendererColors::noSelection,
					  bool isHovered = false,
					  VuoRendererColors::HighlightType highlightType = VuoRendererColors::noHighlight,
					  qint64 timeOfLastActivity = VuoRendererItem::notTrackingActivity,
					  bool isMissingImplementation = false);
	static void setDark(bool isDark);
	static bool isDark(void);

	QColor canvasFill(void);

	QColor nodeFill(void);
	QColor nodeFrame(void);
	QColor nodeTitle(void);
	QColor nodeClass(void);

	QColor constantFill(void);
	QColor constantText(void);

	QColor animatedPortFill(void);
	QColor eventBlockingBarrier(void);
	QColor animatedeventBlockingBarrier(void);
	QColor actionIndicator(void);

	QColor publishedPortFill(void);
	QColor publishedPortTitle(void);
	QColor publishedProtocolPortTitle(void);

	QColor portFill(void);
	QColor portTitlebarFill(void);
	QColor portTitle(void);
	QColor portIcon(void);

	QColor cableMain(void);

	QColor errorMark(void);

	QColor commentFrame(void);
	QColor commentFill(void);
	QColor commentText(void);

	QColor selection(void);

	static qint64 getVirtualNodeExecutionOrigin(void);
	static qint64 getVirtualFiredEventOrigin(void);
	static qint64 getVirtualFiredEventOriginForAnimationFadePercentage(qreal percentage);
	static qint64 getVirtualPropagatedEventOrigin(void);

	static VuoNode::TintColor getActiveProtocolTint(int protocolIndex, bool isInput);

	static const int activityAnimationFadeDuration; ///< Time period, in ms, over which a 'Show Events'-mode animation (e.g., for trigger port firing) fades to its minimum alpha level.

private:
	static VuoRendererColors *sharedColors;

	VuoNode::TintColor tintColor;
	VuoRendererColors::SelectionType selectionType;
	bool isHovered;
	bool isMissingImplementation;
	VuoRendererColors::HighlightType highlightType;
	double currentFadePercentage;
	static bool _isDark;

	QColor baseColor(bool useTintColor = true);
	QColor applyHighlighting(QColor baseColor, qreal selectionIntensity = .66);

	QColor lerpColor(QColor v0, QColor v1, float t);

	qreal getCurrentAlpha(void);

	static qreal getNodeFillLightness(void);

	// 'Show Events' mode node alpha levels (range: 0.0-1.0)
	static const qreal minAlpha; ///< Minimum alpha level to which a node may fade following an execution while in 'Show Events' mode.
	static const int subtleHighlightingLighteningFactor; ///< The factor by which lightness is increased for components drawn in @c subtleHighlight as opposed to @c standardHighlight mode.
	static const int activityFadeDuration; ///< Time period, in ms, over which a component fades to its minimum alpha level following activity (e.g., node execution) while in 'Show Events' mode.
};

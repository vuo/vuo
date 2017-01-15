/**
 * @file
 * VuoRendererColors interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUORENDERERCOLORS_HH
#define VUORENDERERCOLORS_HH

#include "VuoNode.hh"
#include "VuoRendererNode.hh"

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
		standardHighlight,
		subtleHighlight,
		noHighlight
	};

	/**
	 * Specifies the various levels of selection coloring that may be applied to a given component.
	 */
	enum SelectionType
	{
		directSelection,
		indirectSelection,
		sidebarSelection,
		noSelection
	};

	static VuoRendererColors *getSharedColors(void);
	VuoRendererColors(VuoNode::TintColor tintColor = VuoNode::TintNone,
					  VuoRendererColors::SelectionType selectionType = VuoRendererColors::noSelection,
					  bool isHovered = false,
					  VuoRendererColors::HighlightType highlightType = VuoRendererColors::noHighlight,
					  qint64 timeOfLastActivity = VuoRendererItem::notTrackingActivity);
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

	QColor cableUpper(void);
	QColor cableMain(void);

	QColor errorMark(void);

	static qint64 getVirtualNodeExecutionOrigin(void);
	static qint64 getVirtualFiredEventOrigin(void);
	static qint64 getVirtualFiredEventOriginForAnimationFadePercentage(qreal percentage);
	static qint64 getVirtualPropagatedEventOrigin(void);

	static VuoNode::TintColor getActiveProtocolTint(int protocolIndex);

	static const int activityAnimationFadeDuration; ///< Time period, in ms, over which a 'Show Events'-mode animation (e.g., for trigger port firing) fades to its minimum alpha level.

private:
	static VuoRendererColors *sharedColors;

	VuoNode::TintColor tintColor;
	VuoRendererColors::SelectionType selectionType;
	bool isHovered;
	VuoRendererColors::HighlightType highlightType;
	double currentFadePercentage;
	static bool _isDark;

	QColor tint(QColor color, qreal amount = 1., int lighteningFactor = 100);
	QColor lerpColor(QColor v0, QColor v1, float t);

	qreal getCurrentAlphaForDefault(qreal defaultAlpha);
	static qreal getMinAlphaForDefault(qreal defaultAlpha);
	static qreal getMaxAlphaForDefault(qreal defaultAlpha);

	// 'Show Events' mode node alpha levels (range: 0.0-1.0)
	static const qreal minNodeFrameAndFillAlpha; ///< Minimum alpha level to which a node may fade following an execution while in 'Show Events' mode.
	static const qreal maxNodeFrameAndFillAlpha; ///< Maximum alpha level, assigned to each node during its execution while in 'Show Events' mode.
	static const qreal defaultNodeFrameAndFillAlpha; ///< Alpha level assigned to each node while in non-'Show Events' mode.
	static const qreal defaultCableMainAlpha; ///< Alpha level assigned to the main part of each cable while in non-'Show Events' mode.
	static const qreal defaultCableUpperAlpha; ///< Alpha level assigned to the overdrawn upper part of each cable while in non-'Show Events' mode.
	static const qreal defaultConstantAlphaLightMode; ///< Alpha level assigned to constant while in non-'Show Events'+'Light Interface' mode.
	static const qreal defaultConstantAlphaDarkMode; ///< Alpha level assigned to constant while in non-'Show Events'+'Dark Interface' mode.
	static const int subtleHighlightingLighteningFactor; ///< The factor by which lightness is increased for components drawn in @c subtleHighlight as opposed to @c standardHighlight mode.
	static const int activityFadeDuration; ///< Time period, in ms, over which a component fades to its minimum alpha level following activity (e.g., node execution) while in 'Show Events' mode.
};

#endif // VUORENDERERCOLORS_HH

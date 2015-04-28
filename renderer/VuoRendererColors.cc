/**
 * @file
 * VuoRendererColors implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoRendererColors.hh"

const qreal VuoRendererColors::minNodeFrameAndFillAlpha = 0.35;
const qreal VuoRendererColors::maxNodeFrameAndFillAlpha = 1.00;
const qreal VuoRendererColors::defaultNodeFrameAndFillAlpha = 0.75;
const qreal VuoRendererColors::defaultCableUpperAndMainAlpha = 0.5;
const qreal VuoRendererColors::defaultConstantAlpha = 3./8.;
const int VuoRendererColors::subtleHighlightingLighteningFactor = 100; // 100 means no change. @todo: Re-evaluate for https://b33p.net/kosada/node/6855 .
const int VuoRendererColors::activityFadeDuration = 400;
const int VuoRendererColors::activityAnimationFadeDuration = 950;

/**
 * Creates a new color scheme provider, optionally tinted with @c tintColor.
 * If @c selectionType is anything other than @c VuoRendererColors::noSelection, the colors are also tinted slightly blue and have their opacity increased to indicate selection.
 * If @c isHovered is true, the colors are also slightly tinted dark blue to indicate potential for selection.
 * If @c highlightType is anything other than @c VuoRendererColors::noHighlight, the colors are also tinted with light blue
 * (more easily visible at a distance) to indicate potential for cable connection.
 * If @c timeOfLastActivity is anything other than VuoRendererItem::notTrackingActivity, the alpha level
 * is modified to reflect the amount of time that has passed since the @c timeOfLastActivity (e.g., a node execution or event firing), in ms since epoch.
 */
VuoRendererColors::VuoRendererColors(VuoNode::TintColor tintColor, VuoRendererColors::SelectionType selectionType, bool isHovered, VuoRendererColors::HighlightType highlightType, qint64 timeOfLastActivity)
{
	this->tintColor = tintColor;
	this->selectionType = selectionType;
	this->isHovered = isHovered;
	this->highlightType = highlightType;

	// Turn composition components opaque as they execute or fire events, then fade them back to their minimum transparency.
	qint64 timeNow = QDateTime::currentMSecsSinceEpoch();
	const qreal defaultFadePercentage = (maxNodeFrameAndFillAlpha-defaultNodeFrameAndFillAlpha)/(maxNodeFrameAndFillAlpha-minNodeFrameAndFillAlpha);

	this->currentFadePercentage =	((timeOfLastActivity == VuoRendererItem::notTrackingActivity)? defaultFadePercentage :
									((timeOfLastActivity == VuoRendererItem::activityInProgress)? 0 :
									fmin(1, (timeNow - timeOfLastActivity)/(1.0*activityFadeDuration))));
}

VuoRendererColors *VuoRendererColors::sharedColors = NULL;

/**
 * Returns a shared, untinted color scheme provider.
 * Don't free this object.
 */
VuoRendererColors *VuoRendererColors::getSharedColors(void)
{
	if (!sharedColors)
		sharedColors = new VuoRendererColors();

	return sharedColors;
}

/**
 * Returns the color for the background of the composition canvas.
 * Can be overridden by @c VuoRendererComposition::setBackgroundTransparent.
 */
QColor VuoRendererColors::canvasFill(void)
{
	return QColor::fromHslF(0, 0, 1, 1);
}

/**
 * Returns the color for the background of the main section of the node (the background of the area the port labels are drawn on).
 * Also used for port fill, and the background of collapsed typecast ports.
 */
QColor VuoRendererColors::nodeFill(void)
{
	qreal adjustedAlpha = getCurrentAlphaForDefault(defaultNodeFrameAndFillAlpha);
	int lighteningFactor = (highlightType == VuoRendererColors::subtleHighlight? subtleHighlightingLighteningFactor : 100);
	QColor nodeFillColor(tint(QColor::fromHslF(0, 0, 3./4., adjustedAlpha), 1., lighteningFactor));

	return nodeFillColor;
}

/**
 * Returns the color for the background of ports in the main section of the node.
 */
QColor VuoRendererColors::portFill(void)
{
	// If there are to be no modifications for selection, highlight, or hovering,
	// use the same fill color as is used for the main section of the node.
	bool useNodeFill = ((selectionType == VuoRendererColors::noSelection) &&
						(highlightType == VuoRendererColors::noHighlight) &&
						!isHovered);
	
	if (useNodeFill)
		return nodeFill();
	
	// Otherwise, use the lighter constant flag fill color.
	else
	{
		qreal adjustedAlpha = getCurrentAlphaForDefault(defaultConstantAlpha);
		int lighteningFactor = (highlightType == VuoRendererColors::subtleHighlight? subtleHighlightingLighteningFactor : 100);
		QColor nodeFillColor(tint(QColor::fromHslF(0, 0, 3./4., adjustedAlpha), 1., lighteningFactor));
		
		return nodeFillColor;
	}
}

/**
 * Returns the color for the background of ports in the published port sidebars.
 */
QColor VuoRendererColors::publishedPortFill(void)
{
	return nodeFill();
}

/**
 * Returns the color for the node's outline and the background of its title bar.
 * Also used for collapsed typecast port borders.
 */
QColor VuoRendererColors::nodeFrame(void)
{
	qreal adjustedAlpha = getCurrentAlphaForDefault(defaultNodeFrameAndFillAlpha);
	QColor nodeFrameColor(tint(QColor::fromHslF(0, 0, 1./2., adjustedAlpha)));

	return nodeFrameColor;
}

/**
 * Returns the color for the node's title text in the node's title bar.
 */
QColor VuoRendererColors::nodeTitle(void)
{
	return tint(QColor::fromHslF(0, 0, 1, 7./8.));
}

/**
 * Returns the color for the node's class name text in the node's title bar.
 */
QColor VuoRendererColors::nodeClass(void)
{
	return tint(QColor::fromHslF(0, 0, 1, .5));
}

/**
 * Returns the color for the background of constants.
 */
QColor VuoRendererColors::constantFill(void)
{
	qreal adjustedAlpha = getCurrentAlphaForDefault(defaultConstantAlpha);
	int lighteningFactor = (highlightType == VuoRendererColors::subtleHighlight? subtleHighlightingLighteningFactor : 100);
	QColor color = tint(QColor::fromHslF(0, 0, 3./4., adjustedAlpha), 1., lighteningFactor);

	return color;
}

/**
 * Returns the color for the background of an animated copy of a port.
 */
QColor VuoRendererColors::animatedPortFill(void)
{
	qreal minAlpha = 0;
	qreal maxAlpha = getMaxAlphaForDefault(defaultNodeFrameAndFillAlpha);
	qreal adjustedAlpha = maxAlpha-currentFadePercentage*(maxAlpha-minAlpha);

	return tint(QColor::fromHslF(0, 0, 1./2., adjustedAlpha));
}

/**
 * Returns the color for the background of ports in the titlebar of the node.
 */
QColor VuoRendererColors::portTitlebarFill(void)
{
	// If any type of highlighting is to be applied, use the same color scheme
	// as is used for ports in the main body of the node.
	// Disabling for now. @todo: Re-evaluate for https://b33p.net/kosada/node/6855 .
	//if (highlightType != VuoRendererColors::noHighlight)
	//	return portFill();

	qreal adjustedAlpha = getCurrentAlphaForDefault(defaultNodeFrameAndFillAlpha);
	return tint(QColor::fromHslF(0, 0, 1./2., adjustedAlpha));
}

/**
 * Returns the color for a port's event wall or door.
 */
QColor VuoRendererColors::eventBlockingBarrier(void)
{
	return nodeFrame();
}

/**
 * Returns the color for the evnet wall or door of the animated copy of a port.
 */
QColor VuoRendererColors::animatedeventBlockingBarrier(void)
{
	qreal minAlpha = 0;
	qreal maxAlpha = getMaxAlphaForDefault(defaultNodeFrameAndFillAlpha);
	qreal adjustedAlpha = maxAlpha-currentFadePercentage*(maxAlpha-minAlpha);

	return tint(QColor::fromHslF(0, 0, 1./2., adjustedAlpha));
}

/**
 * Returns the color for the title text of ports in the main section of the node.
 * Also used for constant value flag text.
 */
QColor VuoRendererColors::portTitle(void)
{
	const qreal defaultPortTitleAlpha = 1.;
	qreal adjustedAlpha = getCurrentAlphaForDefault(defaultPortTitleAlpha);

	return tint(QColor::fromHslF(0, 0, 1./4., adjustedAlpha));
}

/**
 * Returns the color for the highlight on the upper side of left-to-right cables (and lower side of right-to-left cables).
 */
QColor VuoRendererColors::cableUpper(void)
{
	qreal adjustedAlpha = getCurrentAlphaForDefault(defaultCableUpperAndMainAlpha);
	return tint(QColor::fromHslF(0, 0, .75, adjustedAlpha), 2.);
}

/**
 * Returns the color for the main cable body.
 */
QColor VuoRendererColors::cableMain(void)
{
	qreal adjustedAlpha = getCurrentAlphaForDefault(defaultCableUpperAndMainAlpha);
	return tint(QColor::fromHslF(0, 0, .25, adjustedAlpha), 2.);
}

/**
 * Returns the linear interpolation in RGB colorspace at position @c t (0..1) between colors @c v0 and @c v1.
 * Uses alpha channel from @c v0.
 */
QColor VuoRendererColors::lerpColor(QColor v0, QColor v1, float t)
{
	qreal r0,g0,b0,a;
	v0.getRgbF(&r0,&g0,&b0,&a);

	qreal r1,g1,b1;
	v1.getRgbF(&r1,&g1,&b1);

	return QColor::fromRgbF(
				fmin(r0 + (r1 - r0)*t, 1.),
				fmin(g0 + (g1 - g0)*t, 1.),
				fmin(b0 + (b1 - b0)*t, 1.),
				a
				);
}

/**
 * Tints the specified color by @c tintColor.
 * @c amount is used to tint cables more strongly, since their small size makes the tint less apparent.
 * If @c amount is greater than 1 (the default), a lighter color is also used for highlighting.
 * @c lighteningFactor is used to lighten the color of highlighted components by the specified factor. This
 * should be supplied, e.g., when in VuoRendererColors::subtleHighlight mode, for components that
 * respect the subtle highlighting suggestion.
 */
QColor VuoRendererColors::tint(QColor color, qreal amount, int lighteningFactor)
{
	qreal h,s,l,a;
	color.getHslF(&h,&s,&l,&a);

	qreal hs;
	if (tintColor == VuoNode::TintYellow)
		hs = 45./360.;
	else if (tintColor == VuoNode::TintOrange)
		hs = 15./360.;
	else if (tintColor == VuoNode::TintMagenta)
		hs = 315./360.;
	else if (tintColor == VuoNode::TintViolet)
		hs = 255./360.;
	else if (tintColor == VuoNode::TintCyan)
		hs = 175./360.;
	else if (tintColor == VuoNode::TintGreen)
		hs = 90./360.;

	if (tintColor != VuoNode::TintNone)
	{
		// Factor in the source hue only if source saturation is nonzero
		if (s > .1)
			h = h + (hs - h) * 3./4.;
		else
			h = hs;

		s = s + (1. - s) * 3./8. * amount;
	}

	color = QColor::fromHslF(h,s,l,a);

	bool isHighlighted = (highlightType != VuoRendererColors::noHighlight);
	if (isHighlighted)
	{
		QColor highlight = QColor::fromHslF(235./360., 1., .7, 1.); // Light Blue
		QColor extraHighlight = QColor::fromHslF(235./360., 1., .8, 1.); // Lighter Blue

		color = lerpColor(color, (amount<=1? highlight:extraHighlight), 7./8. * amount);
		color = color.lighter(lighteningFactor);
	}

	bool isSelected = (selectionType != VuoRendererColors::noSelection);
	bool isDirectlySelected = (selectionType == VuoRendererColors::directSelection);
	if (isSelected || isHovered)
	{
		QColor selection = QColor::fromHslF(235./360., 1., .3, 1.); // Dark Blue
		color = lerpColor(color, selection, (isDirectlySelected * 1./8. + isHovered * 3./8.) * amount);

		if (isSelected)
		{
			qreal selectionOpacityFactor = 1.7;
			if (selectionType == VuoRendererColors::indirectSelection)
				selectionOpacityFactor = 1 + (selectionOpacityFactor - 1) * 0.75;

			color.setAlphaF(fmin(selectionOpacityFactor*color.alphaF(), 1.));
		}
	}

	return color;
}

/**
 * Returns the projected time (in ms since epoch) at which a node must have completed its
 * previous execution in order to have faded to the @c defaultNodeFrameAndFillAlpha level
 * at the current time.
 */
qint64 VuoRendererColors::getVirtualNodeExecutionOrigin(void)
{
	const qreal defaultFadePercentage = (maxNodeFrameAndFillAlpha-defaultNodeFrameAndFillAlpha)/(maxNodeFrameAndFillAlpha-minNodeFrameAndFillAlpha);
	qint64 timeNow = QDateTime::currentMSecsSinceEpoch();
	qint64 virtualNodeExecutionOrigin = timeNow - defaultFadePercentage*activityFadeDuration;

	return virtualNodeExecutionOrigin;
}

/**
 * Returns the projected time (in ms since epoch) at which a port must have fired its
 * most recent event in order to have faded to the @c defaultPortFillAlpha level
 * at the current time.
 */
qint64 VuoRendererColors::getVirtualFiredEventOrigin(void)
{
	qreal minPortFillAlpha = getMinAlphaForDefault(defaultNodeFrameAndFillAlpha);
	qreal maxPortFillAlpha = getMaxAlphaForDefault(defaultNodeFrameAndFillAlpha);

	const qreal defaultFadePercentage = (maxPortFillAlpha-defaultNodeFrameAndFillAlpha)/(maxPortFillAlpha-minPortFillAlpha);
	qint64 timeNow = QDateTime::currentMSecsSinceEpoch();
	qint64 virtualFiredEventOrigin = timeNow - defaultFadePercentage*activityFadeDuration;

	return virtualFiredEventOrigin;
}

/**
 * Returns the projected time (in ms since epoch) at which a port must have fired its
 * most recent event in order for its animation to have faded to the @c percentage provided.
 */
qint64 VuoRendererColors::getVirtualFiredEventOriginForAnimationFadePercentage(qreal percentage)
{
	qint64 timeNow = QDateTime::currentMSecsSinceEpoch();
	qint64 virtualFiredEventOrigin = timeNow - percentage*activityAnimationFadeDuration;

	return virtualFiredEventOrigin;
}

/**
 * Returns the projected time (in ms since epoch) at which a cable must have propagated its
 * most recent event in order to have faded to the @c defaultCableUpperAndMainAlpha level
 * at the current time.
 */
qint64 VuoRendererColors::getVirtualPropagatedEventOrigin(void)
{
	qreal minCableUpperAndMainAlpha = getMinAlphaForDefault(defaultCableUpperAndMainAlpha);
	qreal maxCableUpperAndMainAlpha = getMaxAlphaForDefault(defaultCableUpperAndMainAlpha);

	const qreal defaultFadePercentage = (maxCableUpperAndMainAlpha-defaultCableUpperAndMainAlpha)/(maxCableUpperAndMainAlpha-minCableUpperAndMainAlpha);
	qint64 timeNow = QDateTime::currentMSecsSinceEpoch();
	qint64 virtualFiredEventOrigin = timeNow - defaultFadePercentage*activityFadeDuration;

	return virtualFiredEventOrigin;
}

/**
 * Makes use of the primary {min,max,default}NodeFrameAndFillAlpha values to calculate a proportional
 * alpha range for the provided @c defaultAlpha level; based on that range and the
 * @c currentFadePercentage, returns the appropriate current alpha level.
 */
qreal VuoRendererColors::getCurrentAlphaForDefault(qreal defaultAlpha)
{
	qreal minAlpha = getMinAlphaForDefault(defaultAlpha);
	qreal maxAlpha = getMaxAlphaForDefault(defaultAlpha);
	qreal adjustedAlpha = maxAlpha-currentFadePercentage*(maxAlpha-minAlpha);

	return adjustedAlpha;
}

/**
 * Makes use of the primary {min,default}NodeFrameAndFillAlpha values to calculate a proportional
 * minimum alpha value for the provided default alpha level.
 */
qreal VuoRendererColors::getMinAlphaForDefault(qreal defaultAlpha)
{
	qreal primaryAlphaMinToDefaultRatio = minNodeFrameAndFillAlpha/defaultNodeFrameAndFillAlpha;
	return primaryAlphaMinToDefaultRatio*defaultAlpha;
}

/**
 * Makes use of the primary {default,max}NodeFrameAndFillAlpha values to calculate a proportional
 * maximum alpha value for the provided default alpha level.
 */
qreal VuoRendererColors::getMaxAlphaForDefault(qreal defaultAlpha)
{
	qreal primaryAlphaMaxToDefaultRatio = maxNodeFrameAndFillAlpha/defaultNodeFrameAndFillAlpha;
	return fmin(1.0, primaryAlphaMaxToDefaultRatio*defaultAlpha);
}

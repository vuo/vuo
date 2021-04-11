/**
 * @file
 * VuoRendererColors implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoRendererColors.hh"


const qreal VuoRendererColors::minAlpha = 0.35;

const int VuoRendererColors::subtleHighlightingLighteningFactor = 140; // 100 means no change. @todo: Re-evaluate for https://b33p.net/kosada/node/6855 .
const int VuoRendererColors::activityFadeDuration = 400;
const int VuoRendererColors::activityAnimationFadeDuration = 950;
bool VuoRendererColors::_isDark = false;

/**
 * Creates a new color scheme provider, optionally tinted with @c tintColor.
 * If @c selectionType is anything other than @c VuoRendererColors::noSelection,
 * the colors are also tinted slightly blue and have their opacity increased to indicate selection.
 * If @c isHovered is true, the colors are also slightly tinted dark blue to indicate potential for selection.
 * If @c highlightType is anything other than @c VuoRendererColors::noHighlight, the colors are also tinted with light blue
 * (more easily visible at a distance) to indicate potential for cable connection.
 * If @c timeOfLastActivity is anything other than VuoRendererItem::notTrackingActivity, the alpha level
 * is modified to reflect the amount of time that has passed since the @c timeOfLastActivity (e.g., a node execution or event firing), in ms since epoch.
 */
VuoRendererColors::VuoRendererColors(VuoNode::TintColor tintColor, VuoRendererColors::SelectionType selectionType, bool isHovered, VuoRendererColors::HighlightType highlightType, qint64 timeOfLastActivity, bool isMissingImplementation)
{
	this->tintColor = tintColor;
	this->selectionType = selectionType;
	this->isHovered = isHovered;
	this->highlightType = highlightType;
	this->isMissingImplementation = isMissingImplementation;

	// Turn composition components opaque as they execute or fire events, then fade them back to their minimum transparency.
	qint64 timeNow = QDateTime::currentMSecsSinceEpoch();

	this->currentFadePercentage =   ((timeOfLastActivity == VuoRendererItem::notTrackingActivity)? 0 :
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
 * Sets whether all instances return colors appropriate for a dark interface.
 */
void VuoRendererColors::setDark(bool isDark)
{
	_isDark = isDark;
}

/**
 * Returns whether the global interface is dark.
 */
bool VuoRendererColors::isDark(void)
{
	return _isDark;
}

/**
 * Returns the color for the background of the composition canvas.
 * Can be overridden by @c VuoRendererComposition::setBackgroundTransparent.
 */
QColor VuoRendererColors::canvasFill(void)
{
	return QColor::fromHslF(0, 0, _isDark ? .19 : 1, 1);
}

/**
 * Returns the color for the background of the main section of the node (the background of the area the port labels are drawn on).
 * Also used for port fill, and the background of collapsed typecast ports.
 */
QColor VuoRendererColors::nodeFill(void)
{
	return applyHighlighting(baseColor());
}

/**
 * Returns the color for the background of ports in the main section of the node.
 */
QColor VuoRendererColors::portFill(void)
{
	return nodeFill();
}

/**
 * Returns the color for the background of ports in the published port sidebars.
 */
QColor VuoRendererColors::publishedPortFill(void)
{
	return nodeFill();
}

/**
 * Returns the color for the title text of ports in the published port sidebars.
 */
QColor VuoRendererColors::publishedPortTitle(void)
{
	if (highlightType == ineligibleHighlight)
		return Qt::transparent;

	return _isDark ? nodeClass() : portTitle();
}

/**
 * Returns the color for the title text of protocol ports in the published port sidebars.
 */
QColor VuoRendererColors::publishedProtocolPortTitle(void)
{
	return portTitle();
}

/**
 * Returns the color for the node's title bar background.
 */
QColor VuoRendererColors::nodeFrame(void)
{
	if (isMissingImplementation)
		return nodeFill();
	else
		return applyHighlighting(baseColor().darker(_isDark ? 160 : 130));
}

/**
 * Returns the color for the node's title text in the node's title bar.
 */
QColor VuoRendererColors::nodeTitle(void)
{
	QColor c = applyHighlighting(QColor::fromHslF(0, 0, 1, 1), .2);
	if (isMissingImplementation)
		c.setAlphaF(_isDark ? .5 : 1);
	return c;
}

/**
 * Returns the color for the node's class name text in the node's title bar.
 */
QColor VuoRendererColors::nodeClass(void)
{
	QColor c = applyHighlighting(QColor::fromHslF(0, 0, 1, .6));
	if (isMissingImplementation)
		c.setAlphaF(_isDark ? .5 : 1);
	return c;
}

/**
 * Returns the color for the background of constants.
 */
QColor VuoRendererColors::constantFill(void)
{
	return portFill();
}

/**
 * Returns the color for the text on port constant flags.
 */
QColor VuoRendererColors::constantText(void)
{
	return portTitle();
}

/**
 * Returns the color for the background of an animated copy of a port.
 */
QColor VuoRendererColors::animatedPortFill(void)
{
	qreal adjustedAlpha = (1. - currentFadePercentage) * getCurrentAlpha();
	QColor c = portFill();
	c.setAlphaF(c.alphaF() * adjustedAlpha);
	return c;
}

/**
 * Returns the color for the background of ports in the titlebar of the node.
 */
QColor VuoRendererColors::portTitlebarFill(void)
{
	return nodeFrame();
}

/**
 * Returns the color for a port's event wall or door.
 */
QColor VuoRendererColors::eventBlockingBarrier(void)
{
	return nodeFrame();
}

/**
 * Returns the color for the event wall or door of the animated copy of a port.
 */
QColor VuoRendererColors::animatedeventBlockingBarrier(void)
{
	qreal adjustedAlpha = (1. - currentFadePercentage) * getCurrentAlpha();
	QColor c = nodeFrame();
	c.setAlphaF(c.alphaF() * adjustedAlpha);
	return c;
}

/**
 * Returns the color for the port action symbol.
 */
QColor VuoRendererColors::actionIndicator(void)
{
	return nodeFrame();
}

/**
 * Returns the color for the title text of ports in the main section of the node.
 */
QColor VuoRendererColors::portTitle(void)
{
	if (highlightType == ineligibleHighlight)
		return Qt::transparent;

	return applyHighlighting(QColor::fromHslF(0, 0, .2,
											  highlightType == standardHighlight ? 1. : .8),
							 0);
}

/**
 * Returns the color for the triangle on event-only ports.
 */
QColor VuoRendererColors::portIcon()
{
	return actionIndicator();
}

/**
 * Returns the color for the main cable body.
 */
QColor VuoRendererColors::cableMain(void)
{
	return nodeFill();
}

/**
 * Returns the color for marking nodes and cables involved in build errors.
 */
QColor VuoRendererColors::errorMark(void)
{
	return QColor::fromRgb(220, 50, 47, 160);
}

/**
 * Returns the color for the outline of a comment.
 */
QColor VuoRendererColors::commentFrame(void)
{
	QColor darkColor = QColor(0.5*(nodeFrame().red()+nodeFill().red()),
							  0.5*(nodeFrame().green()+nodeFill().green()),
							  0.5*(nodeFrame().blue()+nodeFill().blue()),
							  0.5*(nodeFrame().alpha()+nodeFill().alpha())
							  );

	QColor frameColor = (_isDark? darkColor : nodeFill());
	frameColor.setAlpha(_isDark? 0.5*frameColor.alpha() : 0.82*frameColor.alpha());

	return frameColor;
}

/**
 * Returns the color for the background of a comment.
 */
QColor VuoRendererColors::commentFill(void)
{
	QColor c = nodeFill();
	if (_isDark)
	{
		float darkenFactor = .8;
		QColor b = canvasFill();
		c.setRedF  (c.redF()   * (1-darkenFactor) + b.redF()   * darkenFactor);
		c.setGreenF(c.greenF() * (1-darkenFactor) + b.greenF() * darkenFactor);
		c.setBlueF (c.blueF()  * (1-darkenFactor) + b.blueF()  * darkenFactor);
	}
	else
	{
		int lightenFactor = 113;
		if (tintColor == VuoNode::TintYellow)
			lightenFactor = 140;
		else if (tintColor == VuoNode::TintCyan
			  || tintColor == VuoNode::TintGreen
			  || tintColor == VuoNode::TintLime)
			lightenFactor = 125;
		c = c.lighter(lightenFactor);
	}

	int h,s,v,a;
	c.getHsv(&h, &s, &v, &a);
	return (QColor::fromHsv(h, (_isDark? s*4/3 : s/3), v, a)); // Desaturate.
}

/**
 * Returns the color for the text of a comment.
 */
QColor VuoRendererColors::commentText(void)
{
	QColor c = applyHighlighting(QColor::fromHslF(0, 0, 1, .8));
	return (_isDark? c : portTitle());
}

/**
 * Returns the color for the background of user-selected widgets.
 * (This is for standard GUI widgets.  For canvas nodes/cables,
 * use @ref VuoRendererColors::SelectionType instead.)
 */
QColor VuoRendererColors::selection()
{
	return _isDark ? "#1d6ae5" : "#74acec";
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

QColor VuoRendererColors::baseColor(bool useTintColor)
{
	if (!useTintColor
		   || tintColor == VuoNode::TintNone     )  return QColor::fromHslF(  0./360., 0, getNodeFillLightness(), 1);
	else  if (tintColor == VuoNode::TintYellow   )  return QColor::fromHslF( 57./360., 1, .49, 1);
	else  if (tintColor == VuoNode::TintTangerine)  return QColor::fromHslF( 41./360., 1, .67, 1);
	else  if (tintColor == VuoNode::TintOrange   )  return QColor::fromHslF( 19./360., 1, .73, 1);
	else  if (tintColor == VuoNode::TintMagenta  )  return QColor::fromHslF(323./360., 1, .78, 1);
	else  if (tintColor == VuoNode::TintViolet   )  return QColor::fromHslF(275./360., 1, .81, 1);
	else  if (tintColor == VuoNode::TintBlue     )  return QColor::fromHslF(213./360., 1, .77, 1);
	else  if (tintColor == VuoNode::TintCyan     )  return QColor::fromHslF(165./360., 1, .68, 1);
	else  if (tintColor == VuoNode::TintGreen    )  return QColor::fromHslF( 99./360., 1, .70, 1);
	else/*if (tintColor == VuoNode::TintLime     )*/return QColor::fromHslF( 70./360., 1, .48, 1);
}

QColor VuoRendererColors::applyHighlighting(QColor baseColor, qreal selectionIntensity)
{
	qreal alpha = baseColor.alphaF() * getCurrentAlpha();
	int highlight = 0;

	if (isHovered)
		++highlight;

	if (selectionType != VuoRendererColors::noSelection)
		baseColor = lerpColor(baseColor, selection(), selectionIntensity);

	if (highlightType == VuoRendererColors::standardHighlight)
		++highlight;
	else if (highlightType == VuoRendererColors::subtleHighlight)
	{
		if (isHovered)
		{
			alpha *= .8;
			++highlight;
		}
		else
			alpha *= .6;
	}
	else if (highlightType == VuoRendererColors::ineligibleHighlight)
		alpha *= _isDark ? .1 : .2;

	if (tintColor != VuoNode::TintNone)
		highlight *= 2;

	baseColor = baseColor.lighter(100. + (_isDark ? 10. : -6.) * highlight);

	baseColor.setAlphaF(alpha);
	return baseColor;
}

/**
 * Returns the projected time (in ms since epoch) at which a node must have completed its
 * previous execution in order to have faded to the @c defaultNodeFrameAndFillAlpha level
 * at the current time.
 */
qint64 VuoRendererColors::getVirtualNodeExecutionOrigin(void)
{
	return QDateTime::currentMSecsSinceEpoch();
}

/**
 * Returns the projected time (in ms since epoch) at which a port must have fired its
 * most recent event in order to have faded to the @c defaultPortFillAlpha level
 * at the current time.
 */
qint64 VuoRendererColors::getVirtualFiredEventOrigin(void)
{
	return QDateTime::currentMSecsSinceEpoch();
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
	return QDateTime::currentMSecsSinceEpoch();
}

/**
 * Makes use of the primary {min,max,default}NodeFrameAndFillAlpha values to calculate a proportional
 * alpha range for the provided @c defaultAlpha level; based on that range and the
 * @c currentFadePercentage, returns the appropriate current alpha level.
 */
qreal VuoRendererColors::getCurrentAlpha(void)
{
	return 1 - currentFadePercentage * (1 - minAlpha);
}

/**
 * Returns the default HSL lightness for the node background, depending on whether light or dark mode is active.
 */
qreal VuoRendererColors::getNodeFillLightness(void)
{
	return _isDark ? .62 : .83;
}

/**
 * Returns the tint to be used for the heading of an active protocol, given the
 * index of the protocol within the sidebar.
 */
VuoNode::TintColor VuoRendererColors::getActiveProtocolTint(int protocolIndex, bool isInput)
{
	// @todo: Account for multiple simultaneous active protocols. https://b33p.net/kosada/node/9585
	return isInput ? VuoNode::TintBlue : VuoNode::TintLime;
}

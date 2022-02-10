/**
 * @file
 * VuoUiThemeBase class definition.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoSerializable.hh"
#include "VuoUiTheme.h"
#include "VuoImageText.h"

/**
 * Common ancestor of all VuoUiTheme objects.
 */
class VuoUiThemeBase : public VuoSerializable
{
};

VuoUiThemeBase *VuoUiTheme_getSpecificTheme(VuoUiTheme theme, std::string type);

/**
 * Theme for the button widget.
 */
class VuoUiThemeButton : public VuoUiThemeBase
{
public:
	/**
	 * Creates a layer tree representing a button with the specified theme and parameters.
	 */
	virtual VuoLayer render(	VuoRenderedLayers renderedLayers,
								VuoText label,
								VuoPoint2d position,
								VuoAnchor anchor,
								bool isHovered,
								bool isPressed) = 0;
};

/**
 * Theme for the toggle widget.
 */
class VuoUiThemeToggle : public VuoUiThemeBase
{
public:
	/**
	 * Creates a layer tree representing a toggle with the specified theme and parameters.
	 */
	virtual VuoLayer render(VuoRenderedLayers renderedLayers,
							VuoText label,
							VuoPoint2d position,
							VuoAnchor anchor,
							bool isHovered,
							bool isPressed,
							bool isToggled) = 0;
};

/**
 * Theme for the text/number widget.
 */
class VuoUiThemeTextField : public VuoUiThemeBase
{
public:
	/**
	 * Creates a layer tree representing a button with the specified theme and parameters.
	 */
	virtual VuoLayer render(	VuoPoint2d screenSize,
								VuoReal screenBackingScaleFactor,
								VuoText label,
								VuoText placeholderText,
								int numLines,
								int cursorIndex,
								int selectionStart,
								int selectionEnd,
								VuoPoint2d position,
								VuoReal width,
								VuoAnchor anchor,
								bool isHovered,
								bool isFocused,
								VuoImageTextData* textImageData) = 0;
};

/**
 * Theme for the slider widget.
 */
class VuoUiThemeSlider : public VuoUiThemeBase
{
public:
	/**
	 * Returns the minimum length, in local Vuo Coordinates, that the track can be.
	 */
	virtual VuoReal minimumTrackLength() = 0;

	/**
	 * Creates a layer for the background of a slider widget.
	 */
	virtual VuoLayer render(VuoRenderedLayers renderedLayers,
							VuoText label,
							VuoReal trackLength,
							VuoReal normalizedProgress,
							VuoPoint2d position,
							VuoAnchor anchor,
							VuoOrientation orientation,
							bool isHovered,
							bool isPressed) = 0;

	/**
	 * Returns the length, in local Vuo Coordinates, that the drag handle can move
	 * (typically somewhat less than trackLength).
	 */
	virtual VuoReal handleMovementLength(VuoReal trackLength) = 0;

	/**
	 * Returns true if `pointToTest` is inside the drag handle.
	 */
	virtual bool isPointInsideSliderHandle(VuoRenderedLayers renderedLayers,
										   VuoReal trackLength,
										   VuoReal normalizedProgress,
										   VuoPoint2d position,
										   VuoAnchor anchor,
										   VuoOrientation orientation,
										   VuoPoint2d pointToTest) = 0;
};

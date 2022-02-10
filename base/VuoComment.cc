/**
 * @file
 * VuoComment implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoComment.hh"

/// The type name of the comment, for use in Graphviz.
const string VuoComment::commentTypeName = "vuo.comment";

/**
 * Creates a base Comment instance.
 */
VuoComment::VuoComment(string content, int x, int y, int width, int height, VuoNode::TintColor tintColor)
	: VuoBase<VuoCompilerComment,VuoRendererComment>("VuoComment")
{
	this->content = content;
	this->x = x;
	this->y = y;
	this->width = width;
	this->height = height;
	this->tintColor = tintColor;
}

/**
 * Returns the text content of the comment.
 */
string VuoComment::getContent(void)
{
	return content;
}

/**
 * Sets the text content of the comment.
 */
void VuoComment::setContent(string content)
{
	this->content = content;
}

/**
 * Returns the x coordinate at which the top left corner of this comment instance is drawn in the editor.
 */
int VuoComment::getX(void)
{
	return x;
}

/**
 * Sets the x coordinate at which the top left corner of this comment instance is drawn in the editor.
 */
void VuoComment::setX(int x)
{
	this->x = x;
}

/**
 * Returns the y coordinate at which the top left corner of this comment instance is drawn in the editor.
 */
int VuoComment::getY(void)
{
	return y;
}

/**
 * Sets the y coordinate at which the top left corner of this comment instance is drawn in the editor.
 */
void VuoComment::setY(int y)
{
	this->y = y;
}

/**
 * Returns the width of the comment instance as drawn in the editor.
 */
int VuoComment::getWidth(void)
{
	return width;
}

/**
 * Sets the width of the comment instance as drawn in the editor.
 */
void VuoComment::setWidth(int width)
{
	this->width = width;
}

/**
 * Returns the height of the comment instance as drawn in the editor.
 */
int VuoComment::getHeight(void)
{
	return height;
}

/**
 * Sets the height of the comment instance as drawn in the editor.
 */
void VuoComment::setHeight(int height)
{
	this->height = height;
}

/**
 * Returns this comment's tint color.
 */
enum VuoNode::TintColor VuoComment::getTintColor(void)
{
	return tintColor;
}

/**
 * Returns this comment's tint color as a Graphviz color name, or emptystring if there is no tint.
 */
string VuoComment::getTintColorGraphvizName(void)
{
	return VuoNode::getGraphvizNameForTint(tintColor);
}

/**
 * Sets this comment's tint color.
 */
void VuoComment::setTintColor(enum VuoNode::TintColor tintColor)
{
	this->tintColor = tintColor;
}

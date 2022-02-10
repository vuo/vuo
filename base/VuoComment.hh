/**
 * @file
 * VuoComment interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoBase.hh"
#include "VuoNode.hh"

class VuoCompilerComment;
class VuoRendererComment;

/**
 * Creates a comment.
 *
 * @see VuoCompilerComment
 * @see VuoRendererComment
 */
class VuoComment : public VuoBase<VuoCompilerComment,VuoRendererComment>
{
public:
	VuoComment(string content="", int x=0, int y=0, int width=240, int height=120, VuoNode::TintColor tintColor=VuoNode::TintNone);

	static const string commentTypeName;

	string getContent(void);
	void setContent(string content);
	int getX(void);
	void setX(int x);
	int getY(void);
	void setY(int y);

	int getWidth(void);
	void setWidth(int width);
	int getHeight(void);
	void setHeight(int height);

	VuoNode::TintColor getTintColor(void);
	string getTintColorGraphvizName(void);
	void setTintColor(enum VuoNode::TintColor tintColor);

private:
	string content;
	int x, y;
	int width, height;
	VuoNode::TintColor tintColor;
};

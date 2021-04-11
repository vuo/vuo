/**
 * @file
 * VuoCompilerComment implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <sstream>

#include "VuoCompilerComment.hh"
#include "VuoComment.hh"
#include "VuoStringUtilities.hh"

/**
 * Creates a compiler detail for the specified @c baseComment.
 */
VuoCompilerComment::VuoCompilerComment(VuoComment *baseComment)
	: VuoBaseDetail<VuoComment>("VuoComment", baseComment)
{
	getBase()->setCompiler(this);
	this->graphvizIdentifier = getGraphvizIdentifierPrefix();
}

/**
 * Returns a suggested UpperCamelCase prefix for the comment's Graphviz identifier.
 */
string VuoCompilerComment::getGraphvizIdentifierPrefix(void)
{
	return "Comment";
}

/**
 * Gets the identifier that will appear in .vuo (Graphviz dot format) files.
 */
string VuoCompilerComment::getGraphvizIdentifier(void)
{
	return graphvizIdentifier;
}

/**
 * Sets the identifier that will appear in .vuo (Graphviz dot format) files.
 */
void VuoCompilerComment::setGraphvizIdentifier(string graphvizIdentifier)
{
	this->graphvizIdentifier = graphvizIdentifier;
}

/**
 * Returns a string containing the declaration for this comment
 * as it would appear in a .vuo (Graphviz dot format) file.
 */
string VuoCompilerComment::getGraphvizDeclaration(double xPositionOffset, double yPositionOffset)
{
	ostringstream declaration;

	// name
	declaration << getGraphvizIdentifier();

	// type
	declaration << " [type=\"" << VuoComment::commentTypeName << "\"";

	// label
	declaration << " label=\"" << VuoStringUtilities::transcodeToGraphvizIdentifier(getBase()->getContent()) << "\"";

	// position
	declaration << " pos=\"" << getBase()->getX()+xPositionOffset << "," << getBase()->getY()+yPositionOffset << "\"";

	// width
	declaration << " width=\"" << getBase()->getWidth() << "\"";

	// height
	declaration << " height=\"" << getBase()->getHeight() << "\"";

	// tint color
	if (getBase()->getTintColor() != VuoNode::TintNone)
		declaration << " fillcolor=\"" << getBase()->getTintColorGraphvizName() << "\"";

	declaration << "];";

	return declaration.str();
}

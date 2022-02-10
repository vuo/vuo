/**
 * @file
 * VuoCompilerComment interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoBaseDetail.hh"

class VuoComment;

/**
 * The compiler detail class for @c VuoComment.
 */
class VuoCompilerComment : public VuoBaseDetail<VuoComment>
{
private:
	string graphvizIdentifier;  ///< The identifier that will appear in .vuo (Graphviz dot format) files. Defaults to the suggested Graphviz identifier prefix.

public:
	VuoCompilerComment(VuoComment *baseComment);

	string getGraphvizIdentifierPrefix(void);
	string getGraphvizIdentifier(void);
	void setGraphvizIdentifier(string graphvizIdentifier);
	string getGraphvizDeclaration(double xPositionOffset = 0, double yPositionOffset = 0);
};

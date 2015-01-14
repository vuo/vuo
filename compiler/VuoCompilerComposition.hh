/**
 * @file
 * VuoCompilerComposition interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERCOMPOSITION_HH
#define VUOCOMPILERCOMPOSITION_HH

#include "VuoBaseDetail.hh"
#include "VuoComposition.hh"

#include "VuoCompilerGraphvizParser.hh"

/**
 * A collection of nodes and the cables connecting them.
 */
class VuoCompilerComposition : public VuoBaseDetail<VuoComposition>
{
public:
	VuoCompilerComposition(VuoComposition *baseComposition, VuoCompilerGraphvizParser *parser);
	static VuoCompilerComposition * newCompositionFromGraphvizDeclaration(const string &compositionGraphvizDeclaration, VuoCompiler *compiler);
	VuoNode * getPublishedInputNode(void);
	VuoNode * getPublishedOutputNode(void);
	void setPublishedInputNode(VuoNode *node);
	void setPublishedOutputNode(VuoNode *node);
	string getGraphvizDeclaration(string header = "", string footer = "");
	string getGraphvizDeclarationForComponents(set<VuoNode *> nodeSet, set<VuoCable *> cableSet, string header="", string footer="", double xPositionOffset=0, double yPositionOffset=0);
	string diffAgainstOlderComposition(string oldCompositionGraphvizDeclaration, VuoCompiler *compiler);

	static const string defaultGraphDeclaration; ///< The default graph type and ID to be generated for new .vuo (Graphviz dot format) composition files.

private:
	VuoNode *publishedInputNode;
	VuoNode *publishedOutputNode;
	static bool compareGraphvizIdentifiersOfNodes(VuoNode *lhs, VuoNode *rhs);
	static bool compareGraphvizIdentifiersOfCables(VuoCable *lhs, VuoCable *rhs);
};

#endif // VUOCOMPILERCOMPOSITION_HH

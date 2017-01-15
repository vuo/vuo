/**
 * @file
 * VuoNode interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUONODE_HH
#define VUONODE_HH

#include "VuoBase.hh"

class VuoCompilerNode;
class VuoRendererNode;
class VuoNodeClass;
class VuoPort;

/**
 * This class represents an instance of a @c VuoNodeClass.  There may exist 0 to many @c VuoNodes for each @c VuoNodeClass, depending on the composition being processed.
 *
 * @see VuoCompilerNode
 * @see VuoRendererNode
 */
class VuoNode : public VuoBase<VuoCompilerNode,VuoRendererNode>
{
public:
	/**
	 * Possible colors with which the user can tint a node.
	 * (Red is reserved for error reporting; Blue is reserved for selection/hovering/highlighting.)
	 */
	enum TintColor
	{
		TintNone,
		TintYellow,
		TintTangerine,
		TintOrange,
		TintMagenta,
		TintViolet,
		TintBlue,
		TintCyan,
		TintGreen,
		TintLime,
	};

	VuoNode(VuoNodeClass * nodeClass, string title, VuoPort * refreshPort, vector<VuoPort *>inputPorts, vector<VuoPort *> outputPorts, double x=0, double y=0, bool collapsed=false, VuoNode::TintColor tintColor=TintNone);

	VuoNodeClass * getNodeClass(void);

	VuoPort * getInputPortWithName(string portName);
	VuoPort * getOutputPortWithName(string portName);

	vector<VuoPort *> getInputPorts(void);
	vector<VuoPort *> getOutputPorts(void);

	VuoPort * getRefreshPort(void);

	string getTitle(void);
	void setTitle(string title);

	bool isTypecastNode(void);

	int getX(void);
	void setX(int x);
	int getY(void);
	void setY(int y);

	bool isCollapsed(void);
	void setCollapsed(bool collapsed);

	enum TintColor getTintColor(void);
	string getTintColorGraphvizName(void);
	void setTintColor(enum TintColor tintColor);

	void setRawGraphvizDeclaration(string declaration);
	string getRawGraphvizDeclaration(void);
	string getRawGraphvizIdentifier(void);

	void print(void);

private:
	VuoNodeClass *nodeClass;
	vector<VuoPort *> inputPorts;
	vector<VuoPort *> outputPorts;
	VuoPort *refreshPort;
	string title;
	int x,y;
	bool collapsed;
	enum TintColor tintColor;
	string rawGraphvizDeclaration;
};

#endif // VUONODE_HH

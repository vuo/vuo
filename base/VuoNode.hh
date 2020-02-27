/**
 * @file
 * VuoNode interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

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
	 * (Red is reserved for error reporting.)
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

	static string getGraphvizNameForTint(enum TintColor tintColor);
	static TintColor getTintWithGraphvizName(string name);

	VuoNode(VuoNodeClass * nodeClass, string title, VuoPort * refreshPort, vector<VuoPort *>inputPorts, vector<VuoPort *> outputPorts, double x=0, double y=0, bool collapsed=false, VuoNode::TintColor tintColor=TintNone);

	VuoNodeClass * getNodeClass(void);

	VuoPort * getInputPortWithName(string portName);
	VuoPort * getOutputPortWithName(string portName);

	vector<VuoPort *> getInputPorts(void);
	vector<VuoPort *> getInputPortsBeforePort(VuoPort *port);
	vector<VuoPort *> getInputPortsAfterPort(VuoPort *port);

	vector<VuoPort *> getOutputPorts(void);
	vector<VuoPort *> getOutputPortsBeforePort(VuoPort *port);
	vector<VuoPort *> getOutputPortsAfterPort(VuoPort *port);

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

	bool isForbidden(void);
	void setForbidden(bool forbidden);

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
	bool forbidden;
	string rawGraphvizDeclaration;
};

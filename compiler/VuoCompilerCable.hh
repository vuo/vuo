/**
 * @file
 * VuoCompilerCable interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERCABLE_H
#define VUOCOMPILERCABLE_H

#include "VuoBaseDetail.hh"
#include "VuoCable.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerNodeArgument.hh"

/**
 * Represents a connection from a node's output port to a node's input port.
 */
class VuoCompilerCable : public VuoBaseDetail<VuoCable>
{
public:
	VuoCompilerCable(VuoCompilerNode * fromNode, VuoCompilerPort * fromPort, VuoCompilerNode * toNode, VuoCompilerPort * toPort);
	string getGraphvizDeclaration(void);
	bool carriesData(void);
};

#endif

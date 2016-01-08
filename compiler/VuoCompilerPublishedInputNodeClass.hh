/**
 * @file
 * VuoCompilerPublishedInputNodeClass interface.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERPUBLISHEDINPUTNODECLASS_H
#define VUOCOMPILERPUBLISHEDINPUTNODECLASS_H

#include "VuoCompilerNodeClass.hh"

/**
 * A node class used by the compiler to provide trigger ports corresponding to published input ports.
 */
class VuoCompilerPublishedInputNodeClass : public VuoCompilerNodeClass
{
private:
	VuoCompilerPublishedInputNodeClass(Module *module);
	VuoCompilerPublishedInputNodeClass(VuoCompilerPublishedInputNodeClass *nodeClass);

public:
	static VuoNodeClass * newNodeClass(VuoNodeClass *dummyNodeClass);
};

#endif

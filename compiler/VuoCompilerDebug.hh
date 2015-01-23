/**
 * @file
 * VuoCompilerDebug interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERDEBUG_H
#define VUOCOMPILERDEBUG_H

#include "VuoCompilerEdge.hh"
#include "VuoCompilerChain.hh"


/**
 * Methods for compile-time debugging and for generating code for run-time debugging.
 */
class VuoCompilerDebug
{
public:
	static void print(Type *t);
	static void printType(Value *v);
	static void printTypes(vector<Value *> vs);
	static void printArgTypes(Function *f);
	static void printParamTypes(FunctionType *f);
	static string edgeToString(VuoCompilerEdge *edge);
	static string chainToString(VuoCompilerChain *chain);
	static void printCurrentTime(void);
};

#endif

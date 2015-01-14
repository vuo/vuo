/**
 * @file
 * VuoCompilerDebug implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <sys/time.h>
#include "VuoCompilerDebug.hh"
#include "VuoCompilerTriggerEdge.hh"
#include "VuoCompilerPassiveEdge.hh"


/**
 * Prints information about the specified type @c t.  Recursively deferences pointers.
 */
void VuoCompilerDebug::print(Type *t)
{
	fprintf(stderr, "%p %d", t, t->getTypeID());
	switch (t->getTypeID())
	{
		case Type::IntegerTyID:
			fprintf(stderr, " %d", ((IntegerType *)t)->getBitWidth());
			break;
		case Type::PointerTyID:
			fprintf(stderr, " -> ");
			print(((PointerType *)t)->getElementType());
			break;
		case Type::StructTyID:
			fprintf(stderr, " {\n");
			for (unsigned i = 0; i < ((StructType *)t)->getNumElements(); ++i)
			{
				fprintf(stderr, "\t%u: ", i);
				print(((StructType *)t)->getElementType(i));
			}
			fprintf(stderr, "}");
			break;
		default:
			// Do nothing. This prevents compiler warnings about enumeration value not handled in switch.
			break;
	}
	if (t->getTypeID() != Type::PointerTyID)
		fprintf(stderr, "\n");
}

/**
 * Prints information about the specified value @c v's type.
 */
void VuoCompilerDebug::printType(Value *v)
{
	print(v->getType());
}

/**
 * Prints the name and type of the specified list of values @c vs.
 */
void VuoCompilerDebug::printTypes(vector<Value *> vs)
{
	for (vector<Value *>::iterator i = vs.begin(); i != vs.end(); ++i)
	{
		fprintf(stderr, "%s: ", string((*i)->getName()).c_str());
		printType(*i);
	}
}

/**
 * Prints the name and type of the specified function @c f's arguments.
 */
void VuoCompilerDebug::printArgTypes(Function *f)
{
	for (Function::arg_iterator i = f->arg_begin(); i != f->arg_end(); ++i)
	{
		fprintf(stderr, "%s: ", string(i->getName()).c_str());
		printType(i);
	}
}

/**
 * Prints the types of the specified function type @c f's parameters.
 */
void VuoCompilerDebug::printParamTypes(FunctionType *f)
{
	for (FunctionType::param_iterator i = f->param_begin(); i != f->param_end(); ++i)
	{
		print(*i);
	}
}

/**
 * Returns a string describing the specified @c edge.
 */
string VuoCompilerDebug::edgeToString(VuoCompilerEdge *edge)
{
	return
	edge->getFromNode()->getBase()->getTitle() +
	(dynamic_cast<VuoCompilerTriggerEdge *>(edge) ?
	 ":" + ((VuoCompilerTriggerEdge *)edge)->getTrigger()->getClass()->getBase()->getName() : "") + " -> " +
	string(edge->getToNode()->getBase()->getTitle());
}

/**
 * Returns a string describing the specified @c chain.
 */
string VuoCompilerDebug::chainToString(VuoCompilerChain *chain)
{
	string s;
	vector<VuoCompilerNode *> nodes = chain->getNodes();
	for (vector<VuoCompilerNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
		s += (*i)->getBase()->getTitle() + (i+1 == nodes.end() ? "" : " ");
	return s;
}

/**
 * Prints a timestamp.
 */
void VuoCompilerDebug::printCurrentTime(void)
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	printf("%ld.%6d\n", tv.tv_sec, tv.tv_usec);
}

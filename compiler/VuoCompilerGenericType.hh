/**
 * @file
 * VuoCompilerGenericType interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERGENERICTYPE_HH
#define VUOCOMPILERGENERICTYPE_HH

#include "VuoCompilerType.hh"

class VuoCompiler;
class VuoCompilerNodeClass;
class VuoGenericType;

/**
 * The compiler detail class for VuoGenericType.
 *
 * This class associates a generic type with a "backing type" — a non-generic type to be substituted in for
 * the generic type when compiling/linking a node class or composition.
 */
class VuoCompilerGenericType : public VuoCompilerType
{
public:
	static VuoCompilerGenericType * newGenericType(VuoGenericType *baseType, map<string, VuoCompilerType *> types);
	static VuoCompilerGenericType * newGenericType(VuoGenericType *baseType, VuoCompiler *compiler);
	static string chooseBackingTypeName(string genericTypeName, vector<string> compatibleTypeNames);

	string getBackingTypeName(void);

private:
	VuoCompilerGenericType(VuoGenericType *baseType, VuoCompilerType *backingType);

	static const string defaultBackingTypeName;
};

#endif

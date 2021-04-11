/**
 * @file
 * VuoCompilerEventPortClass interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCompilerPortClass.hh"

class VuoCompilerDataClass;
class VuoType;

/**
 * A passive (non-trigger) port type, optionally with data.
 */
class VuoCompilerEventPortClass : public VuoCompilerPortClass
{
protected:
	VuoCompilerDataClass *dataClass; ///< This port's optional data class.

	VuoCompilerEventPortClass(string name);

public:
	~VuoCompilerEventPortClass(void);
	VuoCompilerDataClass * getDataClass(void);
	void setDataClass(VuoCompilerDataClass *dataClass);
	VuoType * getDataVuoType(void);
	void setDataVuoType(VuoType *type);
	string getDisplayName(void);
};

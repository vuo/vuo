/**
 * @file
 * VuoCompilerDataClass interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERDATACLASS_H
#define VUOCOMPILERDATACLASS_H

#include "VuoCompilerNodeArgumentClass.hh"

class VuoCompilerData;
class VuoType;

/**
 * The data type for a data-and-event port.
 */
class VuoCompilerDataClass : public VuoCompilerNodeArgumentClass
{
private:
	VuoType *vuoType;

protected:
	struct json_object *details;  ///< Metadata specified in the node class implementation, such as the default value.

	VuoCompilerDataClass(string name, Type *type);

public:
	/**
	 * Factory method for constructing a @c VuoCompilerData that instantiates this data class.
	 */
	virtual VuoCompilerData * newData(void) = 0;
	~VuoCompilerDataClass(void);

	VuoType * getVuoType(void);
	void setVuoType(VuoType *vuoType);
	Type * getType(void);
	void setDetails(struct json_object *details);
	json_object * getDetails(void);
};


#endif

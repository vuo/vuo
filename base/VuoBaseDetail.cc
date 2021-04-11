/**
 * @file
 * VuoBaseDetail implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoBaseDetail.hh"

/**
 * Creates a detail class.
 *
 * @param description The name of the detail class inheriting VuoBaseDetail.
 * @param base The base class instance this detail class belongs to.
 */
template<class BaseClass>
VuoBaseDetail<BaseClass>::VuoBaseDetail(string description, BaseClass *base)
{
//	printf("VuoBaseDetail(%p)::VuoBaseDetail(%s, %p)\n",this,description.c_str(),base);
	this->description = description;
	this->base = base;
}

/**
 * Returns the detail class instance's base class instance.
 */
template<class BaseClass>
BaseClass * VuoBaseDetail<BaseClass>::getBase(void) const
{
#ifdef DEBUG
	if (!base)
	{
		fprintf(stderr, "VuoBaseDetail<%s>(%p)::getBase() is null\n", description.c_str(), this);
		fflush(stderr);
		VuoLog_backtrace();
	}
#endif
	return base;
}

/**
 * Sets the detail class instance's base class instance.
 */
template<class BaseClass>
void VuoBaseDetail<BaseClass>::setBase(BaseClass * base)
{
	this->base = base;
}


// Realm of Template Voodoo

class VuoNode;
template class VuoBaseDetail<VuoNode>;

class VuoNodeClass;
template class VuoBaseDetail<VuoNodeClass>;

class VuoType;
template class VuoBaseDetail<VuoType>;

class VuoCable;
template class VuoBaseDetail<VuoCable>;

class VuoPort;
template class VuoBaseDetail<VuoPort>;

class VuoPortClass;
template class VuoBaseDetail<VuoPortClass>;

class VuoPublishedPort;
template class VuoBaseDetail<VuoPublishedPort>;

class VuoComment;
template class VuoBaseDetail<VuoComment>;

class VuoComposition;
template class VuoBaseDetail<VuoComposition>;

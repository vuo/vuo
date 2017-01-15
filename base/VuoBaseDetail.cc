/**
 * @file
 * VuoBaseDetail implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoBaseDetail.hh"

#include <stdio.h>


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
/** \fn VuoBaseDetail<VuoNode>::VuoBaseDetail
 * Creates a VuoNode detail class.
 */
/** \fn VuoBaseDetail<VuoNode>::getBase
 * Returns the VuoNode detail class instance's base class instance.
 */
/** \fn VuoBaseDetail<VuoNode>::setBase
 * Sets the VuoNode detail class instance's base class instance.
 */

class VuoNodeClass;
template class VuoBaseDetail<VuoNodeClass>;
/** \fn VuoBaseDetail<VuoNodeClass>::VuoBaseDetail
 * Creates a VuoNodeClass detail class.
 */
/** \fn VuoBaseDetail<VuoNodeClass>::getBase
 * Returns the VuoNodeClass detail class instance's base class instance.
 */
/** \fn VuoBaseDetail<VuoNodeClass>::setBase
 * Sets the VuoNodeClass detail class instance's base class instance.
 */

class VuoType;
template class VuoBaseDetail<VuoType>;
/** \fn VuoBaseDetail<VuoType>::VuoBaseDetail
 * Creates a VuoType detail class.
 */
/** \fn VuoBaseDetail<VuoType>::getBase
 * Returns the VuoType detail class instance's base class instance.
 */
/** \fn VuoBaseDetail<VuoType>::setBase
 * Sets the VuoType detail class instance's base class instance.
 */

class VuoCable;
template class VuoBaseDetail<VuoCable>;
/** \fn VuoBaseDetail<VuoCable>::VuoBaseDetail
 * Creates a VuoCable detail class.
 */
/** \fn VuoBaseDetail<VuoCable>::getBase
 * Returns the VuoCable detail class instance's base class instance.
 */
/** \fn VuoBaseDetail<VuoCable>::setBase
 * Sets the VuoCable detail class instance's base class instance.
 */

class VuoPort;
template class VuoBaseDetail<VuoPort>;
/** \fn VuoBaseDetail<VuoPort>::VuoBaseDetail
 * Creates a VuoNode detail class.
 */
/** \fn VuoBaseDetail<VuoPort>::getBase
 * Returns the VuoNode detail class instance's base class instance.
 */
/** \fn VuoBaseDetail<VuoPort>::setBase
 * Sets the VuoNode detail class instance's base class instance.
 */

class VuoPortClass;
template class VuoBaseDetail<VuoPortClass>;
/** \fn VuoBaseDetail<VuoPortClass>::VuoBaseDetail
 * Creates a VuoPortClass detail class.
 */
/** \fn VuoBaseDetail<VuoPortClass>::getBase
 * Returns the VuoPortClass detail class instance's base class instance.
 */
/** \fn VuoBaseDetail<VuoPortClass>::setBase
 * Sets the VuoPortClass detail class instance's base class instance.
 */

class VuoPublishedPort;
template class VuoBaseDetail<VuoPublishedPort>;
/** \fn VuoBaseDetail<VuoPublishedPort>::VuoBaseDetail
 * Creates a VuoPublishedPort detail class.
 */
/** \fn VuoBaseDetail<VuoPublishedPort>::getBase
 * Returns the VuoPublishedPort detail class instance's base class instance.
 */
/** \fn VuoBaseDetail<VuoPublishedPort>::setBase
 * Sets the VuoPublishedPort detail class instance's base class instance.
 */

class VuoComposition;
template class VuoBaseDetail<VuoComposition>;
/** \fn VuoBaseDetail<VuoComposition>::VuoBaseDetail
 * Creates a VuoComposition detail class.
 */
/** \fn VuoBaseDetail<VuoComposition>::getBase
 * Returns the VuoComposition detail class instance's base class instance.
 */
/** \fn VuoBaseDetail<VuoComposition>::setBase
 * Sets the VuoComposition detail class instance's base class instance.
 */

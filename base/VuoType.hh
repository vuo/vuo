/**
 * @file
 * VuoType interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOTYPE_HH
#define VUOTYPE_HH

#include "VuoBase.hh"
#include "VuoModule.hh"

class VuoCompilerType;

/**
 * This base class represents the metadata of one type.
 */
class VuoType : public VuoBase<VuoCompilerType,void>, public VuoModule
{
public:
	VuoType(string typeName);
};

#endif // VUOTYPE_HH

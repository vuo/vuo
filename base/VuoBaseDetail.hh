/**
 * @file
 * VuoBaseDetail interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * This class is intended to be inherited by a detail class attached to a base class.
 *
 * @see VuoBase
 */
template<class BaseClass>
class VuoBaseDetail
{
public:
	VuoBaseDetail(string description, BaseClass *base);

	BaseClass * getBase(void) const;
	void setBase(BaseClass * base);

private:
	string description;
	BaseClass * base; ///< The detail class instance's base class instance.
};

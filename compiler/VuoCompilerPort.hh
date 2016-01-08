/**
 * @file
 * VuoCompilerPort interface.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERPORT_H
#define VUOCOMPILERPORT_H

#include "VuoCompilerNodeArgument.hh"
#include "VuoCompilerPortClass.hh"

/**
 * A port.
 */
class VuoCompilerPort : public VuoCompilerNodeArgument
{
public:
	bool hasConnectedCable(bool includePublishedCables) const;
	bool hasConnectedDataCable(bool includePublishedCables) const;
	VuoType * getDataVuoType(void);
	void setDataVuoType(VuoType *dataType);

	/**
	 * Returns a unique, consistent identifier for this port.
	 */
	virtual string getIdentifier(void) = 0;

protected:
	VuoCompilerPort(VuoPort * basePort);

private:
	VuoType *dataType;
};

#endif

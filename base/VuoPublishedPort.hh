/**
 * @file
 * VuoPublishedPort interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoPort.hh"

/**
 * This class represents a published port in a composition. Published ports are visible to clients
 * of the composition, for example to a @c VuoRunner running the composition or to another composition in which
 * this composition is a subcomposition.
 */
class VuoPublishedPort : public VuoPort
{
public:
	VuoPublishedPort(VuoPortClass *portClass);
	void setProtocolPort(bool protocolPort);
	bool isProtocolPort(void);

private:
	bool protocolPort;
};

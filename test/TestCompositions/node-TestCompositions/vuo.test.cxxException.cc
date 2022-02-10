/**
 * @file
 * vuo.test.cxxException node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

extern "C" {
VuoModuleMetadata({
	"title" : "C++ Exception",
	"description" : "",
	"version" : "1.0.0",
});
}

extern "C" void nodeEvent(
	VuoInputEvent() go,
	VuoOutputData(VuoInteger) value)
{
	*value = 1;
	try
	{
		*value = 2;
		throw std::runtime_error("a test exception");
		*value = 3;
	}
	catch (...)
	{
		*value = 4;
	}
}

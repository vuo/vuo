/**
 * @file
 * vuo.test.compatibleWith107And108 node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "compatibleOperatingSystems": {
						"macosx" : { "min": "10.7", "max": "10.8" }
					 }
				 });

void nodeEvent() { }

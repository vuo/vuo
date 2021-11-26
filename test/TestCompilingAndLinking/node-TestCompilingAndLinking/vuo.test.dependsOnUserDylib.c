/**
 * @file
 * vuo.test.dependsOnUserDylib node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#import "node.h"

VuoModuleMetadata({
    "title": "Depends on User Dylib",
	"dependencies": [
        "UserDylib",
	],
});

extern int64_t calculate(int64_t x);

void nodeEvent(
    VuoInputData(VuoInteger) in,
    VuoOutputData(VuoInteger) out
)
{
	*out = calculate(in);
}

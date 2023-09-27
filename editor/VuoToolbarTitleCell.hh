/**
 * @file
 * VuoToolbarTitleCell interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoMacOSSDKWorkaround.h"
#include <AppKit/AppKit.h>

/**
 * Renders text vertically centered in the cell.
 *
 * From https://stackoverflow.com/a/33788973 .
 */
@interface VuoToolbarTitleCell : NSTextFieldCell
@end

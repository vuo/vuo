/**
 * @file
 * VuoInputEditorCurveRenderer interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoInputEditorIcon.hh"

extern "C"
{
#include "VuoCurve.h"
#include "VuoLoopType.h"
#include "VuoSizingMode.h"
#include "VuoWave.h"
#include "VuoWrapMode.h"
}

QIcon *renderMenuIconWithCurve(VuoCurve curve, VuoCurveEasing easing = VuoCurveEasing_In, bool isDark = false);
QIcon *renderMenuIconWithWave(VuoWave wave, bool isDark = false);
QIcon *renderMenuIconWithWrapMode(VuoWrapMode wrapMode, bool isDark = false);
QIcon *renderMenuIconWithLoopType(VuoLoopType loopType, bool isDark = false);
QIcon *renderMenuIconWithSizingMode(VuoSizingMode sizingMode, bool isDark = false);

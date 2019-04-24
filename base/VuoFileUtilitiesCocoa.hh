/**
 * @file
 * VuoFileUtilitiesCocoa interface.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#import <string>
using namespace std;

void VuoFileUtilitiesCocoa_moveFileToTrash(string filePath);
string VuoFileUtilitiesCocoa_makeTmpDirOnSameVolumeAsPath(string onSameVolumeAsPath);
size_t VuoFileUtilitiesCocoa_getAvailableSpaceOnVolumeContainingPath(string path);
void VuoFileUtilitiesCocoa_focusProcess(pid_t pid, bool force);

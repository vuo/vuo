/**
 * @file
 * VuoFileUtilitiesCocoa interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#import <string>
using namespace std;

void VuoFileUtilitiesCocoa_moveFileToTrash(string filePath);
string VuoFileUtilitiesCocoa_makeTmpDirOnSameVolumeAsPath(string onSameVolumeAsPath);
size_t VuoFileUtilitiesCocoa_getAvailableSpaceOnVolumeContainingPath(string path);
void VuoFileUtilitiesCocoa_focusProcess(pid_t pid, bool force);
int VuoFileUtilitiesCocoa_getOSVersionMajor(void);
int VuoFileUtilitiesCocoa_getOSVersionMinor(void);
string VuoFileUtilitiesCocoa_getArchitecture(void);
void VuoFileUtilitiesCocoa_setBundle(string filePath);
bool VuoFileUtilitiesCocoa_isMacAlias(const string &path);
string VuoFileUtilitiesCocoa_resolveMacAlias(const string& path);

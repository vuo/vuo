/**
 * @file
 * VuoFileUtilitiesCocoa implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoFileUtilities.hh"
#include "VuoFileUtilitiesCocoa.hh"

#define NS_RETURNS_INNER_POINTER
#include <Cocoa/Cocoa.h>

#include <stdexcept>

/**
 * Moves the specified file to the user's trash folder.
 *
 * @throw std::runtime_error The file couldn't be moved.
 */
void VuoFileUtilitiesCocoa_moveFileToTrash(string filePath)
{
	SInt32 macMinorVersion;
	Gestalt(gestaltSystemVersionMinor, &macMinorVersion);
	bool success;
	if (macMinorVersion >= 8)
	{
		NSURL *url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:filePath.c_str()]];
		NSFileManager *fm = [NSFileManager defaultManager];
		SEL trashItemSel = @selector(trashItemAtURL:resultingItemURL:error:);
		IMP trashItem = [fm methodForSelector:trashItemSel];
		success = trashItem(fm, trashItemSel, url, nil, nil);
	}
	else
	{
		string dir, file, ext;
		VuoFileUtilities::splitPath(filePath.c_str(), dir, file, ext);
		string fileWithExt = file + "." + ext;
		success = [[NSWorkspace sharedWorkspace] performFileOperation:NSWorkspaceRecycleOperation
															   source:[NSString stringWithUTF8String:dir.c_str()]
														  destination:@""
																files:[NSArray arrayWithObject:[NSString stringWithUTF8String:fileWithExt.c_str()]]
																  tag:nil];
	}

	if (!success)
		throw std::runtime_error("Couldn't move file '" + filePath + "' to the trash.");
}

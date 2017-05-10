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

/**
 * Creates a new temporary directory, avoiding any name conflicts with existing files.
 * The temporary directory will be on the same filesystem as the specified path,
 * to facilitate using `rename()` (for example).
 * `onSameVolumeAsPath` needn't already exist.
 *
 * Returns the path of the directory (without a trailing slash).
 *
 * @throw std::runtime_error
 */
string VuoFileUtilitiesCocoa_makeTmpDirOnSameVolumeAsPath(string path)
{
	NSString *pathNS = [NSString stringWithUTF8String:path.c_str()];
	if (!pathNS)
		throw std::runtime_error("Path \"" + path + "\" isn't a valid UTF-8 string.");

	NSURL *baseURL = [NSURL fileURLWithPath:pathNS isDirectory:YES];
	if (!baseURL)
		throw std::runtime_error("Path \"" + path + "\" isn't a valid directory.");

	NSError *error = nil;
	NSURL *temporaryDirectoryURL = [[NSFileManager defaultManager] URLForDirectory:NSItemReplacementDirectory
																		  inDomain:NSUserDomainMask
																 appropriateForURL:baseURL
																			create:YES
																			 error:&error];
	if (error)
		throw std::runtime_error([[error localizedDescription] UTF8String]);
	if (!temporaryDirectoryURL)
		throw std::runtime_error("Couldn't get a temp folder for path \"" + path + "\".");

	return [[temporaryDirectoryURL path] UTF8String];
}

/**
 * Returns the available space, in bytes, on the volume containing the specified path.
 *
 * `path` needn't exist.
 *
 * @throw std::runtime_error
 */
size_t VuoFileUtilitiesCocoa_getAvailableSpaceOnVolumeContainingPath(string path)
{
	NSString *pathNS = [NSString stringWithUTF8String:path.c_str()];
	if (!pathNS)
		throw std::runtime_error("Path \"" + path + "\" isn't a valid UTF-8 string.");

	NSDictionary *fileAttributes;
	while (true)
	{
		NSError *error = nil;
		fileAttributes = [[NSFileManager defaultManager] attributesOfFileSystemForPath:pathNS error:&error];
		if (error)
		{
			NSError *underlyingError = [[error userInfo] objectForKey:NSUnderlyingErrorKey];
			if ([[underlyingError domain] isEqualToString:NSPOSIXErrorDomain]
			  && [underlyingError code] == ENOENT)
			{
				// File not found; go up a directory and try again.
				pathNS = [pathNS stringByDeletingLastPathComponent];
			}
			else
				throw std::runtime_error([[error localizedDescription] UTF8String]);
		}
		else if (!fileAttributes)
			throw std::runtime_error("Couldn't get information about path \"" + path + "\".");
		else
			break;
	}

	unsigned long long freeSpace = [[fileAttributes objectForKey:NSFileSystemFreeSize] longLongValue];
	return freeSpace;
}

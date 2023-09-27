/**
 * @file
 * VuoFileUtilitiesCocoa implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoFileUtilities.hh"
#include "VuoFileUtilitiesCocoa.hh"
#include "VuoException.hh"
#include "VuoStringUtilities.hh"

#include <AppKit/AppKit.h>
#include <sys/utsname.h>

/**
 * Moves the specified file to the user's trash folder.
 *
 * @throw VuoException The file couldn't be moved.
 */
void VuoFileUtilitiesCocoa_moveFileToTrash(string filePath)
{
	NSURL *url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:filePath.c_str()]];
	if (!url)
		throw VuoException("Couldn't move file '" + filePath + "' to the trash: Couldn't create NSURL.");

	NSError *error = nil;
	bool success = [[NSFileManager defaultManager] trashItemAtURL:url resultingItemURL:nil error:&error];
	if (!success)
		throw VuoException(string("Couldn't move file '" + filePath + "' to the trash: ") + [[error localizedDescription] UTF8String]);
}

/**
 * Creates a new temporary directory, avoiding any name conflicts with existing files.
 * The temporary directory will be on the same filesystem as the specified path,
 * to facilitate using `rename()` (for example).
 * `onSameVolumeAsPath` needn't already exist.
 *
 * Returns the path of the directory (without a trailing slash).
 *
 * @throw VuoException
 */
string VuoFileUtilitiesCocoa_makeTmpDirOnSameVolumeAsPath(string path)
{
	NSString *pathNS = [NSString stringWithUTF8String:path.c_str()];
	if (!pathNS)
		throw VuoException("Path \"" + path + "\" isn't a valid UTF-8 string.");

	NSURL *baseURL = [NSURL fileURLWithPath:pathNS isDirectory:YES];
	if (!baseURL)
		throw VuoException("Path \"" + path + "\" isn't a valid directory.");

	NSURL *temporaryDirectoryURL = nil;
	while (true)
	{
		NSError *error = nil;
		temporaryDirectoryURL = [[NSFileManager defaultManager] URLForDirectory:NSItemReplacementDirectory
																	   inDomain:NSUserDomainMask
															  appropriateForURL:baseURL
																		 create:YES
																		  error:&error];
		if (error)
		{
			NSError *underlyingError = [[error userInfo] objectForKey:NSUnderlyingErrorKey];
			if ([[underlyingError domain] isEqualToString:NSPOSIXErrorDomain]
			  && [underlyingError code] == ENOENT)
			{
				// File not found; go up a directory and try again.
				baseURL = [baseURL URLByDeletingLastPathComponent];
			}
			else
				throw VuoException((string("Couldn't get a temp folder: ") + [[error localizedDescription] UTF8String]).c_str());
		}
		else if (!temporaryDirectoryURL)
			throw VuoException("Couldn't get a temp folder for path \"" + path + "\".");
		else
			break;
	}

	return [[temporaryDirectoryURL path] UTF8String];
}

/**
 * Returns the available space, in bytes, on the volume containing the specified path.
 *
 * `path` should be an absolute POSIX path.  Its last few path components needn't exist.
 *
 * @throw VuoException
 */
size_t VuoFileUtilitiesCocoa_getAvailableSpaceOnVolumeContainingPath(string path)
{
	NSString *pathNS = [NSString stringWithUTF8String:path.c_str()];
	if (!pathNS)
		throw VuoException("Path \"" + path + "\" isn't a valid UTF-8 string.");

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
				throw VuoException((string("Couldn't get information about path: ") + [[error localizedDescription] UTF8String]).c_str());
		}
		else if (!fileAttributes)
			throw VuoException("Couldn't get information about path \"" + path + "\".");
		else
			break;
	}

	unsigned long long freeSpace = [fileAttributes[NSFileSystemFreeSize] longLongValue];
	return freeSpace;
}

/**
 * Attempts to focus the specified `pid` (i.e., bring all its windows to the front and make one of them key).
 *
 * @threadMain
 */
void VuoFileUtilitiesCocoa_focusProcess(pid_t pid, bool force)
{
	if ([NSApp respondsToSelector:@selector(yieldActivationToApplication:)])  // macOS 14+
	{
		NSRunningApplication *compositionApp = [NSRunningApplication runningApplicationWithProcessIdentifier:pid];
		[NSApp performSelector:@selector(yieldActivationToApplication:) withObject:compositionApp];
	}
	else
	{
		// On macOS 13, `-[NSRunningApplication activateWithOptions:]` often fails,
		// but the older `SetFrontProcess()` API works, so use that instead.

		// NSRunningApplication *compositionApp = [NSRunningApplication runningApplicationWithProcessIdentifier:pid];
		// BOOL ret = [compositionApp activateWithOptions: NSApplicationActivateAllWindows
		//     | (force ? NSApplicationActivateIgnoringOtherApps : 0)];
		// if (!ret)
		//     VUserLog("-[NSRunningApplication activateWithOptions:] failed for pid %d", pid);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		ProcessSerialNumber psn;
		OSErr ret = GetProcessForPID(pid, &psn);
		if (ret != noErr)
		{
			VUserLog("GetProcessForPID(%d) failed: %d", pid, ret);
			return;
		}

		ret = SetFrontProcess(&psn);
		if (ret != noErr)
		{
			VUserLog("SetFrontProcess(%d) failed: %d", pid, ret);
			return;
		}
#pragma clang diagnostic pop
	}
}

/**
 * Returns the major component of the OS version.
 */
int VuoFileUtilitiesCocoa_getOSVersionMajor(void)
{
	// NSProcessInfo.processInfo.operatingSystemVersion.majorVersion returns 10.16
	// (instead of the actual version) on macOS versions newer than 10.15,
	// since Vuo uses an older macOS SDK for compatibility with older macOS versions.
	// This prevents distinguishing between e.g., macOS 12 and 13.
	// However, NSProcessInfo.processInfo.operatingSystemVersionString returns the actual version,
	// so try to extract it from there.

	int majorVersion = -1;
	sscanf(NSProcessInfo.processInfo.operatingSystemVersionString.UTF8String, "Version %d", &majorVersion);

	if (majorVersion != -1)
		return majorVersion;
	else
		return NSProcessInfo.processInfo.operatingSystemVersion.majorVersion;
}

/**
 * Returns the minor component of the OS version.
 */
int VuoFileUtilitiesCocoa_getOSVersionMinor(void)
{
	// See VuoFileUtilitiesCocoa_getOSVersionMajor.

	int majorVersion = -1;
	int minorVersion = -1;
	sscanf(NSProcessInfo.processInfo.operatingSystemVersionString.UTF8String, "Version %d.%d", &majorVersion, &minorVersion);

	if (majorVersion != -1 && minorVersion != -1)
		return minorVersion;
	else
		return NSProcessInfo.processInfo.operatingSystemVersion.minorVersion;
}

/**
 * Returns the current system's processor architecture.
 */
string VuoFileUtilitiesCocoa_getArchitecture(void)
{
	struct utsname un;
	if (uname(&un) < 0)
		throw VuoException("Couldn't look up the system's architecture: %s", strerror(errno));

	return un.machine;
}

/**
 * Indicates that Finder should treat this folder as a bundle.
 * (It automatically does this for folders ending in `.app`, but not `.fxplug`.)
 */
void VuoFileUtilitiesCocoa_setBundle(string filePath)
{
	CFStringRef path = CFStringCreateWithCString(NULL, filePath.c_str(), kCFStringEncodingUTF8);
	CFURLRef url = CFURLCreateWithFileSystemPath(NULL, path, kCFURLPOSIXPathStyle, true);
	CFErrorRef error;
	if (!CFURLSetResourcePropertyForKey(url, kCFURLIsPackageKey, kCFBooleanTrue, &error))
	{
		CFStringRef errorString = CFErrorCopyDescription(error);
		VUserLog("Warning: Couldn't set kCFURLIsPackageKey on this bundle: %s", VuoStringUtilities::makeFromCFString(errorString).c_str());
		CFRelease(errorString);
	}
	CFRelease(url);
	CFRelease(path);
}

/**
 * Returns macOS's opaque data for an alias at `path`, or NULL if it isn't an alias.
 */
static CFURLRef VuoFileUtilitiesCocoa_getCFURL(const string &path)
{
	if (path.empty())
		return nullptr;

	CFStringRef cfpath = CFStringCreateWithCString(nullptr, path.c_str(), kCFStringEncodingUTF8);
	if (!cfpath)
		throw VuoException("Path \"" + path + "\" isn't a valid UTF-8 string.");
	VuoDefer(^{ CFRelease(cfpath); });

	CFURLRef url = CFURLCreateWithFileSystemPath(nullptr, cfpath, kCFURLPOSIXPathStyle, true);
	if (!url)
		throw VuoException("Path \"" + path + "\" isn't a valid POSIX path.");

	return url;
}

/**
 * Returns macOS's opaque data for an alias at `path`, or NULL if it isn't an alias.
 */
static CFDataRef VuoFileUtilitiesCocoa_getMacAliasData(const string &path)
{
	CFURLRef url = VuoFileUtilitiesCocoa_getCFURL(path);
	if (!url)
		return nullptr;
	VuoDefer(^{ CFRelease(url); });

	return CFURLCreateBookmarkDataFromFile(nullptr, url, nullptr);
}

/**
 * Returns true if `path` is a macOS Alias (not to be confused with a POSIX symlink).
 *
 * @throw VuoException
 */
bool VuoFileUtilitiesCocoa_isMacAlias(const string &path)
{
	CFDataRef aliasData = VuoFileUtilitiesCocoa_getMacAliasData(path);
	if (!aliasData)
		return false;

	CFRelease(aliasData);
	return true;
}

/**
 * If `path` is a macOS Alias, returns its target path.
 *
 * @throw VuoException
 */
string VuoFileUtilitiesCocoa_resolveMacAlias(const string &path)
{
	CFDataRef aliasData = VuoFileUtilitiesCocoa_getMacAliasData(path);
	if (!aliasData)
		throw VuoException("Path \"" + path + "\" isn't a macOS Alias.");
	VuoDefer(^{ CFRelease(aliasData); });

	CFURLRef url = VuoFileUtilitiesCocoa_getCFURL(path);
	VuoDefer(^{ CFRelease(url); });

	Boolean isStale = false;
	CFErrorRef error = nullptr;
	CFURLRef resolvedURL = CFURLCreateByResolvingBookmarkData(nullptr, aliasData, kCFBookmarkResolutionWithoutUIMask, url, nullptr, &isStale, &error);
	if (!resolvedURL)
	{
		CFStringRef desc = CFErrorCopyDescription(error);
		VuoDefer(^{ CFRelease(desc); });
		throw VuoException("Path \"" + path + "\" is a macOS Alias, but it couldn't be resolved: \"" + VuoStringUtilities::makeFromCFString(desc) + "\" (isStale=" + std::to_string(isStale) + ")");
	}
	VuoDefer(^{ CFRelease(resolvedURL); });

	CFStringRef resolvedPath = CFURLCopyFileSystemPath(resolvedURL, kCFURLPOSIXPathStyle);
	if (!resolvedPath)
		throw VuoException("Path \"" + path + "\" is a macOS Alias, but it resolved to something that isn't a POSIX path (\"" + VuoStringUtilities::makeFromCFString(((NSURL *)resolvedURL).description) + "\").");
	VuoDefer(^{ CFRelease(resolvedPath); });

	return VuoStringUtilities::makeFromCFString(resolvedPath);
}

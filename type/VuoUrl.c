/**
 * @file
 * VuoUrl implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "type.h"
#include "VuoUrl.h"
#include "VuoOsStatus.h"

#include <regex.h>
#include <mach-o/dyld.h> // for _NSGetExecutablePath()
#include <libgen.h> // for dirname()
#include <sys/syslimits.h> // for PATH_MAX
#include <CoreServices/CoreServices.h>
#include "VuoUrlParser.h"


/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "URL",
					 "description" : "Uniform Resource Locator.",
					 "keywords" : [ "link" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"CoreServices.framework",
						"VuoInteger",
						"VuoOsStatus",
						"VuoText",
						"VuoUrlParser"
					 ]
				 });
#endif
/// @}

/**
 * Decodes the JSON object `js`, expected to contain a UTF-8 string, to create a new value.
 */
VuoUrl VuoUrl_makeFromJson(json_object *js)
{
	const char *textString = "";
	if (json_object_get_type(js) == json_type_string)
		textString = json_object_get_string(js);

	VuoUrl url;
	if (textString)
		url = strdup(textString);
	else
		url = strdup("");
	VuoRegister(url, free);

	return url;
}

/**
 * Encodes `value` as a JSON object.
 */
json_object *VuoUrl_getJson(const VuoUrl value)
{
	if (!value)
		return json_object_new_string("");

	return json_object_new_string(value);
}

/**
 * Creates a new UTF-8 C string from `value`, or, if it's more than 50 Unicode characters long, creates an aposiopesis.
 */
char *VuoUrl_getSummary(const VuoUrl value)
{
	VuoText t = VuoText_truncateWithEllipsis(value, 50);
	VuoRetain(t);
	char *summary = strdup(t);
	VuoRelease(t);
	return summary;
}

/**
 * Attempts to parse `url` and outputs its parts.
 *
 * Returns true if parsing succeeded (which implies all output parameters have been populated, though possibly with NULL or zero-length strings).
 */
bool VuoUrl_getParts(const VuoUrl url, VuoText *scheme, VuoText *user, VuoText *host, VuoInteger *port, VuoText *path, VuoText *query, VuoText *fragment)
{
	if (!url || url[0] == 0)
		return false;

	struct http_parser_url parsedUrl;
	if (http_parser_parse_url(url, strlen(url), false, &parsedUrl))
		return false;

	if (scheme)
	{
		if (parsedUrl.field_set & (1 << UF_SCHEMA))
			*scheme = VuoText_makeWithMaxLength(url + parsedUrl.field_data[UF_SCHEMA  ].off, parsedUrl.field_data[UF_SCHEMA  ].len);
		else
			*scheme = NULL;
	}

	if (user)
	{
		if (parsedUrl.field_set & (1 << UF_USERINFO))
			*user   = VuoText_makeWithMaxLength(url + parsedUrl.field_data[UF_USERINFO].off, parsedUrl.field_data[UF_USERINFO].len);
		else
			*user   = NULL;
	}

	if (host)
	{
		if (parsedUrl.field_set & (1 << UF_HOST))
			*host   = VuoText_makeWithMaxLength(url + parsedUrl.field_data[UF_HOST    ].off, parsedUrl.field_data[UF_HOST    ].len);
		else
			*host   = NULL;
	}

	if (port)
	{
		if (parsedUrl.field_set & (1 << UF_PORT))
			// Explicitly-specified port
			*port = parsedUrl.port;
		else
		{
			// Guess the port from the scheme
			*port = 0;
			if (strcmp(*scheme, "http") == 0)
				*port = 80;
			else if (strcmp(*scheme, "https") == 0)
				*port = 443;
		}
	}

	if (path)
	{
		if (parsedUrl.field_set & (1 << UF_PATH))
			*path     = VuoText_makeWithMaxLength(url + parsedUrl.field_data[UF_PATH    ].off, parsedUrl.field_data[UF_PATH    ].len);
		else
			*path     = NULL;
	}

	if (query)
	{
		if (parsedUrl.field_set & (1 << UF_QUERY))
			*query    = VuoText_makeWithMaxLength(url + parsedUrl.field_data[UF_QUERY   ].off, parsedUrl.field_data[UF_QUERY   ].len);
		else
			*query    = NULL;
	}

	if (fragment)
	{
		if (parsedUrl.field_set & (1 << UF_FRAGMENT))
			*fragment = VuoText_makeWithMaxLength(url + parsedUrl.field_data[UF_FRAGMENT].off, parsedUrl.field_data[UF_FRAGMENT].len);
		else
			*fragment = NULL;
	}

	return true;
}

/**
 * Attempts to parse `url` as a `file:///` URL, and outputs its parts.
 *
 * Returns true if parsing succeeded (which implies all output parameters have been populated, though possibly with NULL or zero-length strings).
 *
 * @see VuoFileUtilities::splitPath
 */
bool VuoUrl_getFileParts(const VuoUrl url, VuoText *path, VuoText *folder, VuoText *filename, VuoText *extension)
{
	if (!url || url[0] == 0)
		return false;

	*path = VuoUrl_getPosixPath(url);
	if (!*path)
		return false;

	size_t separatorIndex = VuoText_findLastOccurrence(*path, "/");
	VuoText fileAndExtension;
	if (separatorIndex)
	{
		*folder = VuoText_substring(*path, 1, separatorIndex);
		size_t length = VuoText_length(*path);
		if (separatorIndex < length)
			fileAndExtension = VuoText_substring(*path, separatorIndex + 1, length);
		else
			fileAndExtension = NULL;
	}
	else
	{
		*folder = NULL;
		fileAndExtension = VuoText_make(*path);
	}
	VuoRetain(fileAndExtension);

	size_t dotIndex = VuoText_findLastOccurrence(fileAndExtension, ".");
	if (dotIndex)
	{
		*filename = VuoText_substring(fileAndExtension, 1, dotIndex - 1);
		*extension = VuoText_substring(fileAndExtension, dotIndex + 1, VuoText_length(fileAndExtension));
	}
	else
	{
		*filename = VuoText_make(fileAndExtension);
		*extension = NULL;
	}

	VuoRelease(fileAndExtension);

	return true;
}

/**
 * Returns true if a == b.
 */
bool VuoUrl_areEqual(const VuoText a, const VuoText b)
{
	return VuoText_areEqual(a,b);
}

/**
 * Returns true if a < b.
 */
bool VuoUrl_isLessThan(const VuoText a, const VuoText b)
{
	return VuoText_isLessThan(a,b);
}

/**
 * Returns a boolean indicating whether the input @c url contains a scheme.
 */
static bool VuoUrl_urlContainsScheme(const char *url)
{
	const char *urlWithSchemePattern = "^[a-zA-Z][a-zA-Z0-9+-\\.]+://";
	regex_t    urlWithSchemeRegExp;
	size_t     nmatch = 0;
	regmatch_t pmatch[0];

	regcomp(&urlWithSchemeRegExp, urlWithSchemePattern, REG_EXTENDED);
	bool matchFound = !regexec(&urlWithSchemeRegExp, url, nmatch, pmatch, 0);
	regfree(&urlWithSchemeRegExp);

	return matchFound;
}

/**
 * Returns a boolean indicating whether the input @c url is an absolute file path.
 */
static bool VuoUrl_urlIsAbsoluteFilePath(const char *url)
{
	return ((strlen(url) >= 1) && (url[0] == '/'));
}

/**
 * Returns a boolean indicating whether the input `url` is relative to the user's home folder.
 */
static bool VuoUrl_urlIsUserRelativeFilePath(const char *url)
{
	return ((strlen(url) >= 1) && (url[0] == '~'));
}

/**
 * Returns a boolean indicating whether the provided `url` is a relative file path.
 */
bool VuoUrl_isRelativePath(const VuoUrl url)
{
	return !((VuoText_length(url)==0) ||
			 VuoUrl_urlContainsScheme(url) ||
			 VuoUrl_urlIsAbsoluteFilePath(url) ||
			 VuoUrl_urlIsUserRelativeFilePath(url));
}

/**
 * http://www.blooberry.com/indexdot/html/topics/urlencoding.htm#whatwhy
 */
static const char VuoUrl_reservedCharacters[] =
{
		  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	'"', '$', '&', '+', ',', ':', ';', '=', '?', '@', '#', ' ',
	// Percent must be last, so we don't escape the escapes.
	'%'
};

/**
 * Convert between hex and decimal.
 */
static const char VuoUrl_decToHex[] = "0123456789abcdef";

/**
 * URL-escapes characters in `path` to make it a valid URL path.
 */
VuoText VuoUrl_escapePosixPath(const VuoText path)
{
	// Figure out how many characters we need to allocate for the escaped string.
	unsigned long inLength = strlen(path);
	unsigned long escapedLength = 0;
	for (unsigned long i = 0; i < inLength; ++i)
	{
		char c = path[i];
		for (int j = 0; j < sizeof(VuoUrl_reservedCharacters); ++j)
			if (c == VuoUrl_reservedCharacters[j])
			{
				escapedLength += 2;	// Expanding 1 character to "%xx"
				break;
			}
		++escapedLength;
	}

	// Escape the string.
	char *escapedUrl = (char *)malloc(escapedLength + 1);
	unsigned long outIndex = 0;
	const char *hexCharSet = "0123456789abcdef";
	for (unsigned long inIndex = 0; inIndex < inLength; ++inIndex)
	{
		char c = path[inIndex];
		bool foundEscape = false;
		for (int j = 0; j < sizeof(VuoUrl_reservedCharacters); ++j)
			if (c == VuoUrl_reservedCharacters[j])
			{
				escapedUrl[outIndex++] = '%';
				escapedUrl[outIndex++] = hexCharSet[c >> 4];
				escapedUrl[outIndex++] = hexCharSet[c & 0x0f];
				foundEscape = true;
				break;
			}

		if (!foundEscape)
			escapedUrl[outIndex++] = c;
	}
	escapedUrl[outIndex] = 0;

	VuoText escapedUrlVT = VuoText_make(escapedUrl);
	free(escapedUrl);

	return escapedUrlVT;
}

/**
 * `file://`
 */
static const char *VuoUrl_fileScheme = "file://";

/**
 * Resolves `url` (which could be an absolute URL, an absolute Unix file path, a relative Unix file path, or a user-relative Unix file path)
 * into an absolute URL.
 *
 * If `url` contains a URL schema, this function assumes that all necessary characters are already URL-encoded.
 * Otherwise, this function assumes that _no_ characters in `url` are URL-encoded, and encodes the characters necessary to make it a valid URL.
 *
 * If `isSave` is true and this function is called from an exported app, relative file paths will be resolved to Desktop instead
 * of the app resources folder.
 *
 * If `url` is NULL, returns NULL.
 *
 * If `url` is emptystring, returns a file URL for the current working path (Desktop if `isSave` is true).
 */
VuoUrl VuoUrl_normalize(const VuoText url, bool isSave)
{
	if (!url)
		return NULL;

	char *resolvedUrl;

	// Case: The url contains a scheme.
	if (VuoUrl_urlContainsScheme(url))
	{
		// Some URLs have literal spaces, which we need to transform into '%20' before passing to cURL.
		size_t urlLen = strlen(url);
		size_t spaceCount = 0;
		for (size_t i = 0; i < urlLen; ++i)
			if (url[i] == ' ')
				++spaceCount;
		if (spaceCount)
		{
			resolvedUrl = (char *)malloc(strlen(url) + spaceCount*2);
			size_t p = 0;
			for (size_t i = 0; i < urlLen; ++i)
				if (url[i] == ' ')
				{
					resolvedUrl[p++] = '%';
					resolvedUrl[p++] = '2';
					resolvedUrl[p++] = '0';
				}
				else
					resolvedUrl[p++] = url[i];
			resolvedUrl[p] = 0;
		}
		else
			resolvedUrl = strdup(url);
	}

	// Case: The url contains an absolute file path.
	else if (VuoUrl_urlIsAbsoluteFilePath(url))
	{
		VuoText escapedPath = VuoUrl_escapePosixPath(url);
		VuoRetain(escapedPath);

		resolvedUrl = (char *)malloc(strlen(VuoUrl_fileScheme) + strlen(escapedPath) + 1);
		strcpy(resolvedUrl, VuoUrl_fileScheme);
		strcat(resolvedUrl, escapedPath);

		VuoRelease(escapedPath);
	}

	// Case: The url contains a user-relative file path.
	else if (VuoUrl_urlIsUserRelativeFilePath(url))
	{
		VuoText escapedPath = VuoUrl_escapePosixPath(url);
		VuoRetain(escapedPath);

		VuoText escapedHomeDir;
		{
			char *homeDir = getenv("HOME");
			escapedHomeDir = VuoUrl_escapePosixPath(homeDir);
			VuoRetain(escapedHomeDir);
		}

		resolvedUrl = (char *)malloc(strlen(VuoUrl_fileScheme) + strlen(escapedHomeDir) + strlen(escapedPath) - 1 + 1);
		strcpy(resolvedUrl, VuoUrl_fileScheme);
		strcat(resolvedUrl, escapedHomeDir);
		strcat(resolvedUrl, escapedPath + 1);

		VuoRelease(escapedPath);
		VuoRelease(escapedHomeDir);
	}

	// Case: The url contains a relative file path.
	else
	{
		VuoText escapedPath = VuoUrl_escapePosixPath(url);
		VuoRetain(escapedPath);

		bool compositionIsExportedApp = false;

		VuoText escapedCurrentWorkingDir;
		{
			char currentWorkingDir[PATH_MAX+1];
			getcwd(currentWorkingDir, PATH_MAX+1);
			escapedCurrentWorkingDir = VuoUrl_escapePosixPath(currentWorkingDir);
			VuoRetain(escapedCurrentWorkingDir);
		}

		// If the current working directory is "/", assume that we are working with an exported app;
		// resolve resources relative to the app bundle's "Resources" directory, which can
		// be derived from its executable path.
		if (!strcmp(escapedCurrentWorkingDir, "/"))
		{
			// Get the exported executable path.
			char rawExecutablePath[PATH_MAX+1];
			uint32_t size = sizeof(rawExecutablePath);
			_NSGetExecutablePath(rawExecutablePath, &size);

			char cleanedExecutablePath[PATH_MAX+1];
			realpath(rawExecutablePath, cleanedExecutablePath);

			// Derive the path of the app bundle's "Resources" directory from its executable path.
			char executableDir[PATH_MAX+1];
			strcpy(executableDir, dirname(cleanedExecutablePath));

			const char *resourcesPathFromExecutable = "/../Resources";
			char rawResourcesPath[strlen(executableDir)+strlen(resourcesPathFromExecutable)+1];
			strcpy(rawResourcesPath, executableDir);
			strcat(rawResourcesPath, resourcesPathFromExecutable);

			char cleanedResourcesPath[PATH_MAX+1];
			realpath(rawResourcesPath, cleanedResourcesPath);

			// If the "Resources" directory does not exist, we must not be dealing with an exported app after all.
			// If it does, proceed under the assumption that we are.
			if (access(cleanedResourcesPath, 0) == 0)
			{
				compositionIsExportedApp = true;

				// make relative to desktop if isSave
				if(isSave)
				{
					VuoText escapedHomeDir;
					{
						char *homeDir = getenv("HOME");
						escapedHomeDir = VuoUrl_escapePosixPath(homeDir);
						VuoRetain(escapedHomeDir);
					}

					const char* desktop = "/Desktop/";
					resolvedUrl = (char *)malloc(strlen(VuoUrl_fileScheme) + strlen(escapedHomeDir) + strlen(desktop) + strlen(escapedPath) + 1);
					strcpy(resolvedUrl, VuoUrl_fileScheme);
					strcat(resolvedUrl, escapedHomeDir);
					strcat(resolvedUrl, desktop);
					strcat(resolvedUrl, escapedPath);

					VuoRelease(escapedHomeDir);
				}
				else
				{
					VuoText escapedCleanedResourcesPath;
					{
						escapedCleanedResourcesPath = VuoUrl_escapePosixPath(cleanedResourcesPath);
						VuoRetain(escapedCleanedResourcesPath);
					}

					// Include the scheme and absolute file path in the url passed to cURL.
					resolvedUrl = (char *)malloc(strlen(VuoUrl_fileScheme) + strlen(escapedCleanedResourcesPath) + strlen("/") + strlen(escapedPath) + 1);
					strcpy(resolvedUrl, VuoUrl_fileScheme);
					strcat(resolvedUrl, escapedCleanedResourcesPath);
					strcat(resolvedUrl, "/");
					strcat(resolvedUrl, escapedPath);

					VuoRelease(escapedCleanedResourcesPath);
				}
			}
		}

		// If we are not working with an exported app, resolve resources relative to the current working directory.
		if (!compositionIsExportedApp)
		{
			// Include the scheme and absolute file path in the url passed to cURL.
			resolvedUrl = (char *)malloc(strlen(VuoUrl_fileScheme) + strlen(escapedCurrentWorkingDir) + strlen("/") + strlen(escapedPath) + 1);
			strcpy(resolvedUrl, VuoUrl_fileScheme);
			strcat(resolvedUrl, escapedCurrentWorkingDir);
			strcat(resolvedUrl, "/");
			strcat(resolvedUrl, escapedPath);
		}

		VuoRelease(escapedCurrentWorkingDir);
		VuoRelease(escapedPath);
	}

	// Remove trailing slash, if any.
	size_t lastIndex = strlen(resolvedUrl) - 1;
	if (resolvedUrl[lastIndex] == '/')
		resolvedUrl[lastIndex] = 0;

	VuoText resolvedUrlVT = VuoText_make(resolvedUrl);
	free(resolvedUrl);

	return resolvedUrlVT;
}

/**
 * If `url` uses the `file://` scheme, returns the corresponding (URL-decoded) POSIX path.
 *
 * If `url` does not use the `file://` scheme, returns NULL.
 */
VuoText VuoUrl_getPosixPath(const VuoUrl url)
{
	if (!url)
		return NULL;

	unsigned long fileSchemeLength = strlen(VuoUrl_fileScheme);
	if (strncmp(url, VuoUrl_fileScheme, fileSchemeLength) != 0)
		return NULL;


	// Figure out how many characters we need to allocate for the unescaped string.
	unsigned long inLength = strlen(url);
	long unescapedLength = 0;
	for (unsigned long inIndex = fileSchemeLength; inIndex < inLength; ++inIndex)
	{
		char c = url[inIndex];
		if (c == '%')
			unescapedLength -= 2; // Collapse "%xx" to 1 character.
		++unescapedLength;
	}

	// Unescape the string.
	char *unescapedUrl = (char *)malloc(unescapedLength + 1);
	unsigned long outIndex = 0;
	for (unsigned long inIndex = fileSchemeLength; inIndex < inLength; ++inIndex)
	{
		char c = url[inIndex];
		if (c == '%')
		{
			char highNibbleASCII = url[++inIndex];
			char lowNibbleASCII = url[++inIndex];
			unescapedUrl[outIndex++] = (VuoInteger_makeFromHexByte(highNibbleASCII) << 4) + VuoInteger_makeFromHexByte(lowNibbleASCII);
		}
		else
			unescapedUrl[outIndex++] = c;
	}
	unescapedUrl[outIndex] = 0;

	VuoText unescapedUrlVT = VuoText_make(unescapedUrl);
	free(unescapedUrl);

	return unescapedUrlVT;
}

/**
 * Returns true if the `url` refers to an OS X bundle folder.
 *
 * This function mirrors the behavior of Finder —
 * `.app` bundles return true (in Finder, double-clicking launches the app);
 * `.framework` bundles return false (in Finder, double-clicking opens the folder).
 *
 * Returns false if the URL is non-local or is local but doesn't exist.
 */
bool VuoUrl_isBundle(const VuoUrl url)
{
	if (!url || url[0] == 0)
		return false;

	{
		VuoText path = VuoUrl_getPosixPath(url);
		if (!path)
			return false;
		VuoRetain(path);
		VuoRelease(path);
	}

	CFStringRef urlCFS = CFStringCreateWithCString(NULL, url, kCFStringEncodingUTF8);
	CFURLRef cfurl = CFURLCreateWithString(NULL, urlCFS, NULL);
	CFRelease(urlCFS);
	if (!cfurl)
	{
		VUserLog("Error: Couldn't check '%s': Invalid URL.", url);
		return false;
	}

	LSItemInfoRecord outItemInfo;
	OSStatus ret = LSCopyItemInfoForURL(cfurl, kLSRequestAllFlags, &outItemInfo);
	CFRelease(cfurl);
	if (ret)
	{
		char *errorString = VuoOsStatus_getText(ret);
		VUserLog("Error: Couldn't check '%s': %s", url, errorString);
		free(errorString);
		return false;
	}
	return outItemInfo.flags & kLSItemInfoIsPackage;
}

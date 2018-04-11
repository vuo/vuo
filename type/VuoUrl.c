/**
 * @file
 * VuoUrl implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
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
	VuoText t = VuoText_truncateWithEllipsis(value, 1024, VuoTextTruncation_End);
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
	if (VuoText_isEmpty(url))
		return false;

	struct http_parser_url parsedUrl;
	if (http_parser_parse_url(url, strlen(url), false, &parsedUrl))
	{
		// Maybe this is a "data:" URI (which http_parser_parse_url can't parse).
		if (strncmp(url, "data:", 5) == 0)
		{
			if (scheme)
				*scheme = VuoText_make("data");
			if (user)
				*user = NULL;
			if (host)
				*host = NULL;
			if (port)
				*port = 0;
			if (path)
				*path = NULL;
			if (query)
				*query = NULL;
			if (fragment)
				*fragment = NULL;
			return true;
		}

		return false;
	}

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
	if (VuoText_isEmpty(url))
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
	const char *urlWithSchemePattern = "^[a-zA-Z][a-zA-Z0-9+-\\.]+:";
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
		unsigned char c = path[inIndex];
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

		// https://b33p.net/kosada/node/12361
		// If it's a UTF-8 "Modifier Letter Colon", translate it back into an escaped ASCII-7 colon
		// (since URLs can handle colons, unlike POSIX paths on macOS).
		if (inIndex+2 < inLength)
			if (c == 0xea && (unsigned char)path[inIndex+1] == 0x9e && (unsigned char)path[inIndex+2] == 0x89)
			{
				escapedUrl[outIndex++] = '%';
				escapedUrl[outIndex++] = hexCharSet[':' >> 4];
				escapedUrl[outIndex++] = hexCharSet[':' & 0x0f];
				foundEscape = true;
				inIndex += 2;
			}

		if (!foundEscape)
			escapedUrl[outIndex++] = c;
	}
	escapedUrl[outIndex] = 0;

	VuoText escapedUrlVT = VuoText_make(escapedUrl);
	free(escapedUrl);

	return escapedUrlVT;
}

static const char *VuoUrl_fileScheme = "file://"; ///< URL scheme for local files.
static const char *VuoUrl_httpScheme = "http://"; ///< URL scheme for HTTP.

/**
 * Resolves `url` (which could be an absolute URL, an absolute Unix file path, a relative Unix file path, or a user-relative Unix file path)
 * into an absolute URL.
 *
 * If `url` contains a URL schema, this function assumes that all necessary characters are already URL-encoded.
 * Otherwise, this function assumes that _no_ characters in `url` are URL-encoded, and encodes the characters necessary to make it a valid URL.
 *
 * If `url` is a Unix file path, and if all components of that path exist,
 * this function attempts to resolve it into a canonical path
 * by removing ".", "..", and extra slashes, and dereferencing symlinks.
 *
 * If `url` is NULL, returns NULL.
 *
 * If `url` is emptystring, returns a file URL for the current working path (or Desktop if VuoUrlNormalize_forSaving is set).
 */
VuoUrl VuoUrl_normalize(const VuoText url, enum VuoUrlNormalizeFlags flags)
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
		char *realPath = realpath(url, NULL);
		// If realpath() fails, it's probably because the path doesn't exist, so just use the non-realpath()ed path.
		VuoText escapedPath = VuoUrl_escapePosixPath(realPath ? realPath : url);
		VuoRetain(escapedPath);
		if (realPath)
			free(realPath);

		resolvedUrl = (char *)malloc(strlen(VuoUrl_fileScheme) + strlen(escapedPath) + 1);
		strcpy(resolvedUrl, VuoUrl_fileScheme);
		strcat(resolvedUrl, escapedPath);

		VuoRelease(escapedPath);
	}

	// Case: The url contains a user-relative file path.
	else if (VuoUrl_urlIsUserRelativeFilePath(url))
	{
		// 1. Expand the tilde into an absolute path.
		VuoText absolutePath;
		{
			char *homeDir = getenv("HOME");
			VuoText paths[2] = { homeDir, url+1 };
			absolutePath = VuoText_append(paths, 2);
		}
		VuoLocal(absolutePath);

		// 2. Try to canonicalize the absolute path, and URL-escape it.
		char *realPath = realpath(absolutePath, NULL);
		// If realpath() fails, it's probably because the path doesn't exist, so just use the non-realpath()ed path.
		VuoText escapedPath = VuoUrl_escapePosixPath(realPath ? realPath : absolutePath);
		VuoLocal(escapedPath);
		if (realPath)
			free(realPath);

		// 3. Prepend the URL scheme.
		resolvedUrl = (char *)malloc(strlen(VuoUrl_fileScheme) + strlen(escapedPath) + 1);
		strcpy(resolvedUrl, VuoUrl_fileScheme);
		strcat(resolvedUrl, escapedPath);
	}

	// Case: The url contains a web link without a protocol/scheme.
	else if (flags & VuoUrlNormalize_assumeHttp)
	{
		// Prepend the URL scheme.
		resolvedUrl = (char *)malloc(strlen(VuoUrl_httpScheme) + strlen(url) + 1);
		strcpy(resolvedUrl, VuoUrl_httpScheme);
		strcat(resolvedUrl, url);
	}

	// Case: The url contains a relative file path.
	else
	{
		const char *currentWorkingDir = VuoGetWorkingDirectory();

		bool compositionIsExportedApp = false;

		// If the current working directory is "/", assume that we are working with an exported app;
		// resolve loaded resources relative to the app bundle's "Resources" directory, and
		// resolve saved resources relative to the user's Desktop.
		char *absolutePath;
		if (!strcmp(currentWorkingDir, "/"))
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

				if (flags & VuoUrlNormalize_forSaving)
				{
					char *homeDir = getenv("HOME");
					const char *desktop = "/Desktop/";
					absolutePath = (char *)malloc(strlen(homeDir) + strlen(desktop) + strlen(url) + 1);
					strcpy(absolutePath, homeDir);
					strcat(absolutePath, desktop);
					strcat(absolutePath, url);
				}
				else
				{
					absolutePath = (char *)malloc(strlen(cleanedResourcesPath) + strlen("/") + strlen(url) + 1);
					strcpy(absolutePath, cleanedResourcesPath);
					strcat(absolutePath, "/");
					strcat(absolutePath, url);
				}
			}
		}

		// If we are not working with an exported app, resolve resources relative to the current working directory.
		if (!compositionIsExportedApp)
		{
			absolutePath = (char *)malloc(strlen(currentWorkingDir) + strlen("/") + strlen(url) + 1);
			strcpy(absolutePath, currentWorkingDir);
			strcat(absolutePath, "/");
			strcat(absolutePath, url);
		}

		char *realPath = realpath(absolutePath, NULL);
		// If realpath() fails, it's probably because the path doesn't exist, so just use the non-realpath()ed path.
		VuoText escapedPath = VuoUrl_escapePosixPath(realPath ? realPath : absolutePath);
		VuoLocal(escapedPath);
		if (realPath)
			free(realPath);
		free(absolutePath);

		// Prepend the URL scheme.
		resolvedUrl = (char *)malloc(strlen(VuoUrl_fileScheme) + strlen(escapedPath) + 1);
		strcpy(resolvedUrl, VuoUrl_fileScheme);
		strcat(resolvedUrl, escapedPath);
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
 * Decodes a RFC 3986 section 2.1 URI-encoded string.
 *
 * For example, `Hello%20world` becomes `Hello world`.
 */
VuoText VuoUrl_decodeRFC3986(const VuoUrl url)
{
	unsigned long inLength = strlen(url);
	char *unescapedUrl = (char *)malloc(inLength + 1);
	unsigned long outIndex = 0;
	for (unsigned long inIndex = 0; inIndex < inLength; ++inIndex, ++outIndex)
	{
		char c = url[inIndex];
		if (c == '%')
		{
			if (inIndex + 2 >= inLength)
				break;
			char highNibbleASCII = url[++inIndex];
			char lowNibbleASCII = url[++inIndex];
			unescapedUrl[outIndex] = (VuoInteger_makeFromHexByte(highNibbleASCII) << 4) + VuoInteger_makeFromHexByte(lowNibbleASCII);
		}
		else
			unescapedUrl[outIndex] = c;
	}
	unescapedUrl[outIndex] = 0;

	VuoText unescapedUrlVT = VuoText_make(unescapedUrl);
	free(unescapedUrl);
	return unescapedUrlVT;
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

	// Unescape the string.
	unsigned long inLength = strlen(url);
	// Make room for Unicode colons.
	char *unescapedUrl = (char *)malloc(inLength*3 + 1);
	unsigned long outIndex = 0;
	for (unsigned long inIndex = fileSchemeLength; inIndex < inLength; ++inIndex, ++outIndex)
	{
		char c = url[inIndex];
		if (c == '%')
		{
			char highNibbleASCII = url[++inIndex];
			char lowNibbleASCII = url[++inIndex];
			unescapedUrl[outIndex] = (VuoInteger_makeFromHexByte(highNibbleASCII) << 4) + VuoInteger_makeFromHexByte(lowNibbleASCII);
		}
		else
			unescapedUrl[outIndex] = c;

		// https://b33p.net/kosada/node/12361
		// macOS presents colons as forward-slashes (https://developer.apple.com/library/mac/qa/qa1392/).
		// To avoid confusion with dates, change ASCII-7 colon to UTF-8 "Modifier Letter Colon"
		// (which looks visually identical to ASCII-7 colon).
		if (unescapedUrl[outIndex] == ':')
		{
			unescapedUrl[outIndex++] = 0xea;
			unescapedUrl[outIndex++] = 0x9e;
			unescapedUrl[outIndex]   = 0x89;
		}
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
	if (VuoText_isEmpty(url))
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

/**
 * Given a filename and set of valid extensions, returns a new VuoUrl guaranteed to have the extension.
 */
VuoUrl VuoUrl_appendFileExtension(const char *filename, const char** validExtensions, const unsigned int extensionsLength)
{
	char* fileSuffix = strrchr(filename, '.');
	char* curExtension = fileSuffix != NULL ? strdup(fileSuffix+1) : NULL;

	if(curExtension != NULL)
		for(char *p = &curExtension[0]; *p; p++) *p = tolower(*p);

	// if the string already has one of the valid file extension suffixes, return.
	for(int i = 0; i < extensionsLength; i++)
	{
		if(curExtension != NULL && strcmp(curExtension, validExtensions[i]) == 0)
		{
			free(curExtension);
			return VuoText_make(filename);
		}
	}

	free(curExtension);

	size_t buf_size = strlen(filename) + strlen(validExtensions[0]) + 2;
	char* newfilepath = (char*)malloc(buf_size * sizeof(char));
	snprintf(newfilepath, buf_size, "%s.%s", filename, validExtensions[0]);

	VuoText text = VuoText_make(newfilepath);
	free(newfilepath);

	return text;
}


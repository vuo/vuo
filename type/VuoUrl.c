/**
 * @file
 * VuoUrl implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "type.h"
#include "VuoUrl.h"

#include <regex.h>
#include <mach-o/dyld.h> // for _NSGetExecutablePath()
#include <libgen.h> // for dirname()


/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "URL",
					 "description" : "Uniform Resource Locator.",
					 "keywords" : [ "link" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoText"
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
 * Resolves `url` (which could be an absolute URL, an absolute Unix file path, a relative Unix file path, or a user-relative Unix file path)
 * into an absolute URL.
 */
VuoUrl VuoUrl_normalize(const VuoUrl url, bool shouldEscapeSpaces)
{
	const char *fileScheme = "file://";
	char *resolvedUrl;

	// Case: The url contains a scheme.
	if (VuoUrl_urlContainsScheme(url))
		resolvedUrl = strdup(url);

	// Case: The url contains an absolute file path.
	else if (VuoUrl_urlIsAbsoluteFilePath(url))
	{
		resolvedUrl = (char *)malloc(strlen(fileScheme)+strlen(url)+1);
		strcpy(resolvedUrl, fileScheme);
		strcat(resolvedUrl, url);
	}

	// Case: The url contains a user-relative file path.
	else if (VuoUrl_urlIsUserRelativeFilePath(url))
	{
		char *homeDir = getenv("HOME");
		resolvedUrl = (char *)malloc(strlen(fileScheme) + strlen(homeDir) + strlen(url) - 1 + 1);
		strcpy(resolvedUrl, fileScheme);
		strcat(resolvedUrl, homeDir);
		strcat(resolvedUrl, url + 1);
	}

	// Case: The url contains a relative file path.
	else
	{
		bool compositionIsExportedApp = false;

		char currentWorkingDir[PATH_MAX+1];
		getcwd(currentWorkingDir, PATH_MAX+1);

		// If the current working directory is "/", assume that we are working with an exported app;
		// resolve resources relative to the app bundle's "Resources" directory, which can
		// be derived from its executable path.
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

				// Include the scheme and absolute file path in the url passed to cURL.
				resolvedUrl = (char *)malloc(strlen(fileScheme)+strlen(cleanedResourcesPath)+strlen("/")+strlen(url)+1);
				strcpy(resolvedUrl, fileScheme);
				strcat(resolvedUrl, cleanedResourcesPath);
				strcat(resolvedUrl, "/");
				strcat(resolvedUrl, url);
			}
		}

		// If we are not working with an exported app, resolve resources relative to the current working directory.
		if (!compositionIsExportedApp)
		{
			// Include the scheme and absolute file path in the url passed to cURL.
			resolvedUrl = (char *)malloc(strlen(fileScheme)+strlen(currentWorkingDir)+strlen("/")+strlen(url)+1);
			strcpy(resolvedUrl, fileScheme);
			strcat(resolvedUrl, currentWorkingDir);
			strcat(resolvedUrl, "/");
			strcat(resolvedUrl, url);
		}
	}

	// Remove trailing slash, if any.
	size_t lastIndex = strlen(resolvedUrl) - 1;
	if (resolvedUrl[lastIndex] == '/')
		resolvedUrl[lastIndex] = 0;

	// Escape spaces, if needed.
	VuoText escapedResolvedUrl = (shouldEscapeSpaces ?
									  VuoText_replace(resolvedUrl, " ", "%20") :
									  VuoText_make(resolvedUrl));
	free(resolvedUrl);

	return escapedResolvedUrl;
}

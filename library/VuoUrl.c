/**
 * @file
 * VuoUrl implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <string.h>
#include <regex.h>
#include <unistd.h>
#include <mach-o/dyld.h> // for _NSGetExecutablePath()
#include <libgen.h> // for dirname()

#include <curl/curl.h>

#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoUrl",
					 "dependencies" : [
						 "curl",
						 "crypto",
						 "ssl"
					 ]
				 });
#endif

/**
 * Initializes the cURL library.
 */
__attribute__((constructor)) static void VuoUrl_init(void)
{
	curl_global_init(CURL_GLOBAL_DEFAULT);
}

/**
 * A memory buffer, filled by @c VuoUrl_curlCallback().
 */
struct VuoUrl_curlBuffer
{
	char *memory;
	size_t size;
};

/**
 * Reads the contents of the URL into a memory buffer.
 * A callback function for use by @c curl_easy_setopt().
 */
static size_t VuoUrl_curlCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct VuoUrl_curlBuffer *mem = (struct VuoUrl_curlBuffer *)userp;

	mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
	if(mem->memory == NULL)
	{
		fprintf(stderr, "VuoUrl_curlCallback() Error: realloc() returned NULL (out of memory?).\n");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
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
 * Resolves @c url (which could be an absolute URL, an absolute Unix file path, or a relative Unix file path)
 * into an absolure URL.
 *
 * The caller is responsible for freeing the returned string.
 */
char *VuoUrl_normalize(const char *url)
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

	return resolvedUrl;
}

/**
 * Receives the data at the specified @c url.
 *
 * @return true upon success, false upon failure.
 * @todo Better error handling per https://b33p.net/kosada/node/4724
 */
bool VuoUrl_get(const char *url, void **data, unsigned int *dataLength)
{
	char *resolvedUrl = VuoUrl_normalize(url);

	struct VuoUrl_curlBuffer buffer = {NULL, 0};
	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();
	if (!curl)
	{
		fprintf(stderr, "VuoUrl_get() Error: cURL initialization failed.\n");
		free(resolvedUrl);
		return false;
	}

	curl_easy_setopt(curl, CURLOPT_URL, resolvedUrl);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, VuoUrl_curlCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buffer);

	res = curl_easy_perform(curl);
	if(res != CURLE_OK)
	{
		if (res == CURLE_FILE_COULDNT_READ_FILE)
		{
			VLog("Error: Could not read path: \"%s\"", resolvedUrl);
		}
		else
		{
			VLog("Error: cURL request failed: %s (%d)\n", curl_easy_strerror(res), res);
		}
		return false;
	}

	free(resolvedUrl);

	curl_easy_cleanup(curl);

	*data = buffer.memory;
	*dataLength = buffer.size;
	return true;
}

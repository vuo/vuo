/**
 * @file
 * VuoUrlFetch implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <string.h>
#include <sys/errno.h>
#include <unistd.h>

#include <curl/curl.h>

#include "module.h"
#include "VuoBase64.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoUrlFetch",
					 "dependencies" : [
						 "VuoBase64",
						 "VuoText",
						 "VuoUrl",
						 "curl",
						 "crypto",
						 "ssl",
						 "z"
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
		VUserLog("Error: realloc() returned NULL (out of memory?).");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

/**
 * Receives the data at the specified @c url.
 *
 * The caller is responsible for `free()`ing the data.
 *
 * `data` includes an extra trailing NULL terminator byte (not counted in `dataLength`),
 * to make it easier to work with URLs containing plain text.
 *
 * @return true upon success, false upon failure.
 * @todo Better error handling per https://b33p.net/kosada/node/4724
 */
bool VuoUrl_fetch(const char *url, void **data, unsigned int *dataLength)
{
	if (VuoText_isEmpty(url))
		return false;

	if (strncmp(url, "data:", 5) == 0)
	{
		// https://tools.ietf.org/html/rfc2397
		// data:[<mediatype>][;base64],<data>

		const char *urlData = url + 5;

		// Skip past the <mediatype> tag(s); we only care about the <data> part.
		urlData = strchr(urlData, ',');
		if (!urlData)
			return false;

		// Skip past the comma.
		++urlData;

		// Does the pre-<data> part of the URI end with `;base64`?
		bool isBase64 = (urlData - url >= 5 /* data: */ + 7 /* ;base64 */ + 1)
					 && (strncmp(urlData - 7 - 1, ";base64", 7) == 0);

		VuoText decoded = VuoUrl_decodeRFC3986(urlData);
		VuoRetain(decoded);

		if (isBase64)
		{
			long long outputLength;
			*data = VuoBase64_decode(decoded, &outputLength);
			*dataLength = outputLength;
		}
		else
		{
			*data = strdup(decoded);
			*dataLength = strlen(decoded);
		}

		VuoRelease(decoded);
		return true;
	}

	struct VuoUrl_curlBuffer buffer = {NULL, 0};
	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();
	if (!curl)
	{
		VUserLog("Error: cURL initialization failed.");
		return false;
	}
	VuoDefer(^{ curl_easy_cleanup(curl); });

	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);  // Don't use signals for the timeout logic, since they're not thread-safe.

	VuoText resolvedUrl = VuoUrl_normalize(url, VuoUrlNormalize_default);
	VuoLocal(resolvedUrl);
	curl_easy_setopt(curl, CURLOPT_URL, resolvedUrl);

	curl_easy_setopt(curl, CURLOPT_USERAGENT, "Vuo/" VUO_VERSION_STRING " (https://vuo.org/)");

	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, VuoUrl_curlCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buffer);

	res = curl_easy_perform(curl);
	if(res != CURLE_OK)
	{
		if (res == CURLE_FILE_COULDNT_READ_FILE)
		{
			// If the path has colons (which aren't valid path characters on macOS),
			// maybe they've been escaped as UTF-8 "Modifier Letter Colon".
			// Try escaping them and see if that leads us to the file.
			// https://b33p.net/kosada/node/14924
			VuoText posixPath = VuoUrl_getPosixPath(resolvedUrl);
			VuoLocal(posixPath);
			if (!posixPath)
			{
				VUserLog("Error: Could not read URL \"%s\"", resolvedUrl);
				return false;
			}

			FILE *fp = fopen(posixPath, "rb");
			if (!fp)
			{
				VUserLog("Error: Could not read file \"%s\"", posixPath);
				return false;
			}
			VuoDefer(^{ fclose(fp); });

			fseek(fp, 0, SEEK_END);
			*dataLength = ftell(fp);
			rewind(fp);
			*data = (char *)malloc(*dataLength);
			if (fread(*data, 1, *dataLength, fp) != *dataLength)
			{
				VUserLog("Error: Could not read all the data from file \"%s\": %s", posixPath, strerror(errno));
				return false;
			}
			return true;
		}
		else
			VUserLog("Error: cURL request failed: %s (%d)", curl_easy_strerror(res), res);
		return false;
	}

	*data = buffer.memory;
	*dataLength = buffer.size;
	return true;
}

/**
 * @file
 * VuoUrlFetch implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <string.h>
#include <unistd.h>

#include <curl/curl.h>

#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoUrlFetch",
					 "dependencies" : [
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
 * @return true upon success, false upon failure.
 * @todo Better error handling per https://b33p.net/kosada/node/4724
 */
bool VuoUrl_fetch(const char *url, void **data, unsigned int *dataLength)
{
	if (!url || url[0] == 0)
		return false;

	struct VuoUrl_curlBuffer buffer = {NULL, 0};
	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();
	if (!curl)
	{
		VUserLog("Error: cURL initialization failed.");
		return false;
	}

	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);  // Don't use signals for the timeout logic, since they're not thread-safe.

	VuoText resolvedUrl = VuoUrl_normalize(url, false);
	VuoRetain(resolvedUrl);
	curl_easy_setopt(curl, CURLOPT_URL, resolvedUrl);

	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, VuoUrl_curlCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buffer);

	res = curl_easy_perform(curl);
	if(res != CURLE_OK)
	{
		if (res == CURLE_FILE_COULDNT_READ_FILE)
			VUserLog("Error: Could not read path: \"%s\"", resolvedUrl);
		else
			VUserLog("Error: cURL request failed: %s (%d)\n", curl_easy_strerror(res), res);
		VuoRelease(resolvedUrl);
		curl_easy_cleanup(curl);
		return false;
	}
	VuoRelease(resolvedUrl);

	curl_easy_cleanup(curl);

	*data = buffer.memory;
	*dataLength = buffer.size;
	return true;
}

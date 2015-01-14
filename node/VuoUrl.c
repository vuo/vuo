/**
 * @file
 * VuoUrl implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <string.h>

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
 * Receives the data at the specified @c url.
 *
 * @return true upon success, false upon failure.
 * @todo Better error handling per https://b33p.net/kosada/node/4724
 */
bool VuoUrl_get(const char *url, void **data, unsigned int *dataLength)
{
	struct VuoUrl_curlBuffer buffer = {NULL, 0};
	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();
	if (!curl)
	{
		fprintf(stderr, "VuoUrl_get() Error: cURL initialization failed.\n");
		return false;
	}

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, VuoUrl_curlCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buffer);

	res = curl_easy_perform(curl);
	if(res != CURLE_OK)
	{
		fprintf(stderr, "VuoUrl_get() Error: cURL request failed: %s\n", curl_easy_strerror(res));
		return false;
	}

	curl_easy_cleanup(curl);

	*data = buffer.memory;
	*dataLength = buffer.size;
	return true;
}

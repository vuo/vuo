/**
 * @file
 * VuoUrlFetch implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <string.h>
#include <sys/errno.h>
#include <unistd.h>

#include "module.h"

#include <CoreFoundation/CoreFoundation.h>

#include <curl/curl.h>

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
 * Converts `buffer` from `sourceEncoding` to UTF-8.
 */
void VuoUrlFetch_convertToUTF8(struct VuoUrl_curlBuffer *buffer, CFStringEncoding sourceEncoding)
{
	CFStringRef cf = CFStringCreateWithBytesNoCopy(kCFAllocatorDefault, (const UInt8 *)buffer->memory, strlen(buffer->memory), sourceEncoding, true, kCFAllocatorMalloc);
	if (!cf)
	{
		VuoText sourceEncodingName = VuoText_makeFromCFString(CFStringGetNameOfEncoding(sourceEncoding));
		VuoLocal(sourceEncodingName);
		VUserLog("Error: Couldn't convert response from %s to UTF-8.", sourceEncodingName);
		return;
	}

	CFIndex maxBytes = CFStringGetMaximumSizeForEncoding(CFStringGetLength(cf), kCFStringEncodingUTF8) + 1;
	char *outBuffer = calloc(1, maxBytes);
	CFStringGetCString(cf, outBuffer, maxBytes, kCFStringEncodingUTF8);
	CFRelease(cf);

	buffer->memory = outBuffer;
	buffer->size = strlen(outBuffer);

	{
		VuoText sourceEncodingName = VuoText_makeFromCFString(CFStringGetNameOfEncoding(sourceEncoding));
		VuoLocal(sourceEncodingName);
		VUserLog("Converted response from %s to UTF-8.", sourceEncodingName);
	}
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

	VuoText resolvedUrl = VuoUrl_normalize(url, VuoUrlNormalize_default);
	VuoLocal(resolvedUrl);

	VuoText posixPath = VuoUrl_getPosixPath(resolvedUrl);
	VuoLocal(posixPath);
	if (posixPath)
	{
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
		*data = (char *)malloc(*dataLength + 1);
		if (fread(*data, 1, *dataLength, fp) != *dataLength)
		{
			free(*data);
			VUserLog("Error: Could not read all the data from file \"%s\": %s", posixPath, strerror(errno));
			return false;
		}
		((char *)*data)[*dataLength] = 0;
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

	VDebugLog("GET %s", resolvedUrl);
	curl_easy_setopt(curl, CURLOPT_URL, resolvedUrl);

	curl_easy_setopt(curl, CURLOPT_USERAGENT, "Vuo/" VUO_VERSION_STRING " (https://vuo.org/)");

	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, VuoUrl_curlCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buffer);

	res = curl_easy_perform(curl);
	if(res != CURLE_OK)
	{
		VUserLog("Error: cURL request failed: %s (%d)", curl_easy_strerror(res), res);
		return false;
	}

	VDebugLog("Received %zu bytes.", buffer.size);
	if (buffer.size == 0 || !buffer.memory)
		return false;

	// Convert non-UTF-8 charsets to UTF-8.
	char *contentType = NULL;
	res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &contentType);
	if (res == CURLE_OK && contentType)
	{
		// https://tools.ietf.org/html/rfc7231#section-3.1.1.5
		VDebugLog("Content-Type: %s", contentType);

		// UTF-8 is a superset of US-ASCII, so no need to convert.
		if (strcasecmp(contentType, "text/html; charset=utf-8") == 0
		 || strcasecmp(contentType, "text/html; charset=us-ascii") == 0)
			;

		// According to https://www.ietf.org/rfc/rfc2854.txt, the default charset for `text/html` is `ISO-8859-1`, so fall back to that if no charset is specified.
		else if (strcasecmp(contentType, "text/html; charset=iso-8859-1") == 0
			  || strcasecmp(contentType, "text/html") == 0)
			VuoUrlFetch_convertToUTF8(&buffer, kCFStringEncodingISOLatin1);

		// We're not currently attempting to handle charsets other than UTF-8, US-ASCII, and ISO-8859-1.
	}

	*data = buffer.memory;
	*dataLength = buffer.size;
	return true;
}

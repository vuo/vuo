/**
 * @file
 * VuoNumberFormat implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "type.h"
#include "VuoNumberFormat.h"
#include "VuoList_VuoNumberFormat.h"
#include <CoreFoundation/CoreFoundation.h>

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({	"title" : "Number Format",
					"description" : "How to convert a number into text.",
					"keywords" : [ ],
					"version" : "1.0.0",
					"dependencies" : [
						"VuoText",
					"VuoList_VuoNumberFormat",
					"CoreFoundation.framework"
					]
				});
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "percentage"
 * }
 */
VuoNumberFormat VuoNumberFormat_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoNumberFormat value = VuoNumberFormat_Decimal;

	if (strcmp(valueAsString, "percentage") == 0)
		value = VuoNumberFormat_Percentage;
	else if (strcmp(valueAsString, "currency") == 0)
		value = VuoNumberFormat_Currency;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoNumberFormat_getJson(const VuoNumberFormat value)
{
	char *valueAsString = "decimal";

	if (value == VuoNumberFormat_Percentage)
		valueAsString = "percentage";
	else if (value == VuoNumberFormat_Currency)
		valueAsString = "currency";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a new VuoText string from the formatted number value.
 * @version200New
 */
VuoText VuoNumberFormat_format(VuoReal value,
	VuoNumberFormat format,
	VuoInteger minimumIntegerDigits,
	VuoInteger minimumDecimalPlaces,
	VuoInteger decimalPlaces,
	bool showThousandSeparator)
{
	VuoText text = NULL;

	CFNumberFormatterStyle style = kCFNumberFormatterDecimalStyle;

	if (format == VuoNumberFormat_Percentage)
		style = kCFNumberFormatterPercentStyle;
	else if (format == VuoNumberFormat_Currency)
		style = kCFNumberFormatterCurrencyStyle;

	CFLocaleRef currentLocale = CFLocaleCopyCurrent();
	CFNumberFormatterRef numberFormatter = CFNumberFormatterCreate(NULL, currentLocale, style);

	{
		CFNumberRef cfn = CFNumberCreate(NULL, kCFNumberIntType, &minimumIntegerDigits);
		CFNumberFormatterSetProperty(numberFormatter, kCFNumberFormatterMinIntegerDigits, cfn);
		CFRelease(cfn);
	}

	{
		CFNumberRef cfn = CFNumberCreate(NULL, kCFNumberIntType, &minimumDecimalPlaces);
		CFNumberFormatterSetProperty(numberFormatter, kCFNumberFormatterMinFractionDigits, cfn);
		CFRelease(cfn);
	}

	{
		CFNumberRef maxFractionDigits = CFNumberCreate(NULL, kCFNumberIntType, &decimalPlaces);
		CFNumberFormatterSetProperty(numberFormatter, kCFNumberFormatterMaxFractionDigits, maxFractionDigits);
		CFRelease(maxFractionDigits);
	}

	CFNumberFormatterSetProperty(numberFormatter, kCFNumberFormatterUseGroupingSeparator, showThousandSeparator ? kCFBooleanTrue : kCFBooleanFalse);

	{
		CFStringRef formattedNumberString = CFNumberFormatterCreateStringWithValue(NULL, numberFormatter, kCFNumberDoubleType, &value);
		text = VuoText_makeFromCFString(formattedNumberString);
		CFRelease(formattedNumberString);
	}

	CFRelease(numberFormatter);
	CFRelease(currentLocale);

	return text;
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoNumberFormat VuoNumberFormat_getAllowedValues(void)
{
	VuoList_VuoNumberFormat l = VuoListCreate_VuoNumberFormat();
	VuoListAppendValue_VuoNumberFormat(l, VuoNumberFormat_Decimal);
	VuoListAppendValue_VuoNumberFormat(l, VuoNumberFormat_Percentage);
	VuoListAppendValue_VuoNumberFormat(l, VuoNumberFormat_Currency);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoNumberFormat_getSummary(const VuoNumberFormat value)
{
	char *valueAsString = "Decimal";

	if (value == VuoNumberFormat_Percentage)
		valueAsString = "Percentage";
	else if (value == VuoNumberFormat_Currency)
		valueAsString = "Currency";

	return strdup(valueAsString);
}

/**
 * Returns true if the two values are equal.
 */
bool VuoNumberFormat_areEqual(const VuoNumberFormat valueA, const VuoNumberFormat valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoNumberFormat_isLessThan(const VuoNumberFormat valueA, const VuoNumberFormat valueB)
{
	return valueA < valueB;
}


/**
 * @file
 * VuoInputEditorWithEnumMenu implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoInputEditorWithEnumMenu.hh"

#include <dlfcn.h>

/**
 * Creates an enum input editor for the specified Vuo Type.
 *
 * @param type e.g., `VuoBoolean`
 */
VuoInputEditorWithEnumMenu::VuoInputEditorWithEnumMenu(QString type)
{
	this->type = type;
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorWithEnumMenu::setUpMenuTree(json_object *details)
{
	VuoInputEditorMenuItem *rootMenuItem = new VuoInputEditorMenuItem("root");

	QString allowedValuesFunctionName = this->type + "_getAllowedValues";
	typedef void *(*allowedValuesFunctionType)(void);
	allowedValuesFunctionType allowedValuesFunction = (allowedValuesFunctionType)dlsym(RTLD_DEFAULT, allowedValuesFunctionName.toUtf8().constData());

	QString summaryFunctionName = this->type + "_getSummary";
	typedef char *(*summaryFunctionType)(int64_t);
	summaryFunctionType summaryFunction = (summaryFunctionType)dlsym(RTLD_DEFAULT, summaryFunctionName.toUtf8().constData());

	QString jsonFunctionName = this->type + "_getJson";
	typedef json_object *(*jsonFunctionType)(int64_t);
	jsonFunctionType jsonFunction = (jsonFunctionType)dlsym(RTLD_DEFAULT, jsonFunctionName.toUtf8().constData());

	QString listCountFunctionName = "VuoListGetCount_" + this->type;
	typedef unsigned long (*listCountFunctionType)(void *);
	listCountFunctionType listCountFunction = (listCountFunctionType)dlsym(RTLD_DEFAULT, listCountFunctionName.toUtf8().constData());

	QString listValueFunctionName = "VuoListGetValue_" + this->type;
	typedef int64_t (*listValueFunctionType)(void *, unsigned long);
	listValueFunctionType listValueFunction = (listValueFunctionType)dlsym(RTLD_DEFAULT, listValueFunctionName.toUtf8().constData());

	if (allowedValuesFunction && summaryFunction && jsonFunction && listCountFunction && listValueFunction)
	{
		void *allowedValues = allowedValuesFunction();
		unsigned long count = listCountFunction(allowedValues);
		for (unsigned long i=1; i<=count; ++i)
		{
			int64_t value = listValueFunction(allowedValues, i);
			json_object *valueJson = jsonFunction(value);
			if (!shouldIncludeValue(valueJson))
				continue;

			char *summary = summaryFunction(value);
			rootMenuItem->addItem(new VuoInputEditorMenuItem(summary, valueJson));
			free(summary);
		}
	}

	return rootMenuItem;
}

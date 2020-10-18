/**
 * @file
 * VuoInputEditorManager implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoInputEditorManager.hh"
#include "VuoInputEditorNamedEnum.hh"
#include "VuoInputEditorWithEnumMenu.hh"
#include "VuoFileUtilities.hh"
#include "VuoType.hh"

#include <dlfcn.h>

/**
 * Loads all input editor plugins (built-in and third-party).
 */
VuoInputEditorManager::VuoInputEditorManager(QList<QDir> extraPluginDirectories)
{
	initialization.lock();
	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
	QList<QDir> pluginDirectories;
	pluginDirectories += QCoreApplication::applicationDirPath() + "/../Resources/InputEditors";
	pluginDirectories += QString::fromUtf8(VuoFileUtilities::getSystemModulesPath().c_str());
	pluginDirectories += QString::fromUtf8(VuoFileUtilities::getUserModulesPath().c_str());
	pluginDirectories += extraPluginDirectories;

	foreach (QDir pluginDirectory, pluginDirectories)
	{
		foreach (QString pluginFileName, pluginDirectory.entryList(QDir::Files))
		{
			QPluginLoader loader(pluginDirectory.absoluteFilePath(pluginFileName));
			QObject *plugin = loader.instance();
			if (plugin)
			{
				VuoInputEditorFactory *inputEditorFactory = qobject_cast<VuoInputEditorFactory *>(plugin);
				plugins[loader.metaData().value("MetaData").toObject().value("type").toString()] = inputEditorFactory;
			}
		}
	}
		initialization.unlock();
	});
}

/**
 * Waits until all the plugins have finished initializing.
 */
void VuoInputEditorManager::waitForInitiailization()
{
	while (!initialization.try_lock())
	{
		// Allow plugins to execute code on the main thread.
		QApplication::processEvents();
		usleep(USEC_PER_SEC/10);
	}
}

/**
 * Creates a new input editor for type @c type.
 * An optional port @c details object may be provided in cases where the details
 * object may affect the choice of input editor.
 */
VuoInputEditor * VuoInputEditorManager::newInputEditor(VuoType *type, json_object *details)
{
	if (!type)
		return NULL;

	json_object *menuItemsValue = NULL;
	if (type->getModuleKey() == "VuoInteger" && details && json_object_object_get_ex(details, "menuItems", &menuItemsValue))
		return new VuoInputEditorNamedEnum();

	VuoInputEditorFactory *inputEditorFactory = plugins[type->getModuleKey().c_str()];

	if (!inputEditorFactory)
	{
		// There's no explicitly-defined input editor for this type — but if it's an enum type, we can provide the standard enum input editor.
		string allowedValuesFunctionName = type->getModuleKey() + "_getAllowedValues";
		if (dlsym(RTLD_SELF, allowedValuesFunctionName.c_str()))
			return new VuoInputEditorWithEnumMenu(type->getModuleKey().c_str());

		return NULL;
	}

	return inputEditorFactory->newInputEditor();
}

/**
 * Returns true if `type` may be serialized into a port constant in a composition file.
 * (For better performance, some types' serializations cheat by serializing a memory pointer.)
 *
 * If a type has an input editor, it's assumed to be offline-serializable.
 *
 * @todo https://b33p.net/kosada/node/7368
 */
bool VuoInputEditorManager::doesTypeAllowOfflineSerialization(VuoType *type)
{
	if (!type)
		return false;

	VuoInputEditor *inputEditorLoadedForPortDataType = newInputEditor(type);
	if (inputEditorLoadedForPortDataType)
	{
		inputEditorLoadedForPortDataType->deleteLater();
		return true;
	}

	return false;
}

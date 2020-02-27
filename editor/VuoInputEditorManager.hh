/**
 * @file
 * VuoInputEditorManager interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoInputEditor;
class VuoInputEditorFactory;
class VuoType;

/**
 * This class keeps track of implementations of the VuoInputEditor interface.
 */
class VuoInputEditorManager
{
public:
	VuoInputEditorManager(QList<QDir> extraPluginDirectories = QList<QDir>());
	VuoInputEditor * newInputEditor(VuoType *type, json_object *details = NULL);

	bool doesTypeAllowOfflineSerialization(VuoType *type);

private:
	map<QString, VuoInputEditorFactory *> plugins;
};

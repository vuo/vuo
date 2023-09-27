/**
 * @file
 * VuoModuleManager interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCompilerDelegate.hh"
class VuoType;
class VuoCompiler;
class VuoCompilerIssues;
class VuoCompilerModule;
class VuoCompilerNodeClass;
class VuoCompilerType;
class VuoCodeWindow;
class VuoEditorComposition;
class VuoNodeLibrary;

typedef set<pair<VuoCompilerModule *, VuoCompilerModule *> > ModulesModifiedSet;  ///< Needed to avoid a syntax error with `Q_DECLARE_METATYPE`

/**
 * Handles loading of node classes and types. When modules are loaded on launch,
 * and when they are installed, uninstalled, or modified while the editor is running,
 * this class receives notifications from the compiler and updates the UI and cached data.
 */
class VuoModuleManager : public QObject, VuoCompilerDelegate
{
	Q_OBJECT

public:
	typedef void (^CallbackType)(void);  ///< Invoked when a node class is loaded.

	VuoModuleManager(VuoCompiler *compiler);
	virtual ~VuoModuleManager(void);
	void setComposition(VuoEditorComposition *composition);
	void setCodeWindow(VuoCodeWindow *codeWindow);
	void setNodeLibrary(VuoNodeLibrary *nodeLibrary);
	VuoNodeLibrary * getNodeLibrary(void);
	void updateWithAlreadyLoadedModules(void);

	void doNextTimeNodeClassIsLoaded(const string &nodeClassName, CallbackType callback);
	void doEveryTimeNodeClassIsLoaded(const string &nodeClassName, CallbackType callback);
	void cancelCallbackForNodeClass(const string &nodeClassName);

	map<string, VuoCompilerType *> getLoadedSingletonTypes(bool limitToSpecializationTargets);
	map<string, VuoCompilerType *> getLoadedGenericCompoundTypes(void);
	set<string> getKnownListTypeNames(bool limitToSpecializationTargets);
	set<string> getKnownNodeSets(void);
	string getNodeSetForType(const string &typeName);
	vector<string> getCompatibleTypecastClasses(const string &fromTypeName, VuoType *fromType, const string &toTypeName, VuoType *toType);
	set<string> findInstancesOfNodeClass(const string &sourcePath);

	void loadedModules(const map<string, VuoCompilerModule *> &modulesAdded,
					   const map<string, pair<VuoCompilerModule *, VuoCompilerModule *> > &modulesModified,
					   const map<string, VuoCompilerModule *> &modulesRemoved, VuoCompilerIssues *issues);

signals:
	/**
	 * Used for scheduling UI changes on the main thread.
	 */
	void loadedModulesAndReadyToUpdate(const vector<string> &nodeClassesToRemove, const vector<VuoCompilerNodeClass *> &nodeClassesToAdd, const vector<string> &typesToRemove, const vector<VuoCompilerType *> &typesToAdd, const vector<string> &librariesToRemove, const ModulesModifiedSet &modulesModified, VuoCompilerIssues *issues);

private:
	void updateWithModulesBeingLoaded(const vector<string> &nodeClassesToRemove, const vector<VuoCompilerNodeClass *> &nodeClassesToAdd, const vector<string> &typesToRemove, const vector<VuoCompilerType *> &typesToAdd, const vector<string> &librariesToRemove, const ModulesModifiedSet &modulesModified, VuoCompilerIssues *issues);
	void update(const vector<string> &nodeClassesToRemove, const vector<VuoCompilerNodeClass *> &nodeClassesToAdd, const vector<string> &typesToRemove, const vector<VuoCompilerType *> &typesToAdd, const vector<std::string> &librariesToRemove, const set<pair<VuoCompilerModule *, VuoCompilerModule *> > &modulesModified, VuoCompilerIssues *issues);
	static bool isResponsibleForReportingErrors(VuoModuleManager *currManager, VuoCompilerIssues *errors, const vector<VuoCompilerNodeClass *> &nodeClassesToAdd);
	void showErrorDialog(VuoCompilerIssues *errors);

	void updateLoadedNodeClasses(const vector<string> &nodeClassesToRemove, const vector<VuoCompilerNodeClass *> &nodeClassesToAdd);
	void updateLoadedTypes(const vector<string> &typesToRemove, const vector<VuoCompilerType *> &typesToAdd);
	string getPrimaryAffiliatedNodeSetForType(const string &typeName);

	VuoCompiler *compiler;  ///< The compiler from which this class receives notifications.
	VuoEditorComposition *composition;  ///< The composition to update (if any) when modules are loaded/unloaded.
	VuoCodeWindow *codeWindow;  ///< The code window to update (if any) when its node class fails to load.
	VuoNodeLibrary *nodeLibrary;  ///< The node library to update (if any) when modules are loaded/unloaded.

	map<string, pair<CallbackType, bool> > callbackForNodeClass;  ///< Callbacks to be called when node classes are added or modified, indexed by node class name.

	map<pair<VuoType *, VuoType *>, set<string>> loadedTypeConverterNodeClasses;  ///< Tracks the type converters that have been loaded and not unloaded, organizing them by input and output port type.
	map<string, VuoCompilerType *> loadedSingletonTypes;  ///< Tracks the singleton types that have been loaded and not unloaded.
	map<string, VuoCompilerType *> loadedGenericCompoundTypes;  ///< Tracks the unspecialized generic compound types (e.g. VuoList) that have been loaded and not unloaded.
	set<string> knownNodeSets;  ///< Tracks the node sets from which at least one node class or type has been loaded (and may or may not still be loaded).

	static set<VuoModuleManager *> allModuleManagers;  ///< All module managers that have been constructed and not yet destroyed.
};

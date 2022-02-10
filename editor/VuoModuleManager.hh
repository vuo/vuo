/**
 * @file
 * VuoModuleManager interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCompiler.hh"
class VuoType;
class VuoCompiler;
class VuoCompilerIssues;
class VuoCompilerModule;
class VuoCompilerType;
class VuoCodeWindow;
class VuoEditorComposition;
class VuoNodeLibrary;

/**
 * Handles loading of node classes and types. When modules are loaded on launch,
 * and when they are installed, uninstalled, or modified while the editor is running,
 * this class receives notifications from the compiler and updates the UI and cached data.
 */
class VuoModuleManager : public VuoCompilerDelegate
{
public:
	typedef void (^CallbackType)(void);  ///< Invoked when a node class is loaded.

	VuoModuleManager(VuoCompiler *compiler);
	virtual ~VuoModuleManager(void);
	void deleteWhenReady(void);
	void setComposition(VuoEditorComposition *composition);
	void setCodeWindow(VuoCodeWindow *codeWindow);
	void setNodeLibrary(VuoNodeLibrary *nodeLibrary);
	VuoNodeLibrary * getNodeLibrary(void);
	void updateWithAlreadyLoadedModules(void);
	vector<string> getCompatibleTypecastClasses(VuoType *inType, VuoType *outType);
	map<string, set<VuoCompilerType *> > getLoadedTypesForNodeSet(void);
	void doNextTimeNodeClassIsLoaded(const string &nodeClassName, CallbackType callback);
	void doEveryTimeNodeClassIsLoaded(const string &nodeClassName, CallbackType callback);
	void cancelCallbackForNodeClass(const string &nodeClassName);
	set<string> findInstancesOfNodeClass(const string &sourcePath);

	void loadedModules(const map<string, VuoCompilerModule *> &modulesAdded,
					   const map<string, pair<VuoCompilerModule *, VuoCompilerModule *> > &modulesModified,
					   const map<string, VuoCompilerModule *> &modulesRemoved, VuoCompilerIssues *issues);

private:
	void update(const vector<string> &nodeClassesToRemove, const vector<VuoCompilerNodeClass *> &nodeClassesToAdd, const vector<string> &typesToRemove, const vector<VuoCompilerType *> &typesToAdd, const vector<std::string> &librariesToRemove, const set<pair<VuoCompilerModule *, VuoCompilerModule *> > &modulesModified, VuoCompilerIssues *issues);
	static bool isResponsibleForReportingErrors(VuoModuleManager *currManager, VuoCompilerIssues *errors, const vector<VuoCompilerNodeClass *> &nodeClassesToAdd);
	void showErrorDialog(VuoCompilerIssues *errors);
	void updateLoadedTypecastClasses(const vector<string> &nodeClassesToRemove, const vector<VuoCompilerNodeClass *> &nodeClassesToAdd, const vector<string> &typesToRemove, const vector<VuoCompilerType *> &typesToAdd);
	void updateLoadedTypesByNodeSet(const vector<string> &typesToRemove, const vector<VuoCompilerType *> &typesToAdd);
	string getPrimaryAffiliatedNodeSetForType(const string &typeName);

	VuoCompiler *compiler;  ///< The compiler from which this class receives notifications.
	VuoEditorComposition *composition;  ///< The composition to update (if any) when modules are loaded/unloaded.
	VuoCodeWindow *codeWindow;  ///< The code window to update (if any) when its node class fails to load.
	VuoNodeLibrary *nodeLibrary;  ///< The node library to update (if any) when modules are loaded/unloaded.
	map<pair<VuoType *, VuoType *>, vector<string> > loadedTypecastClasses;  ///< Maps input/output data pairs to lists of suitable typeconverter class names.
	map<string, set<VuoCompilerType *> > loadedTypesForNodeSet;  ///< Maps node set names to sets of associated types.
	map<string, pair<CallbackType, bool> > callbackForNodeClass;  ///< Callbacks to be called when node classes are added or modified, indexed by node class name.
	dispatch_group_t updateGroup;  ///< Keeps track of asynchronous calls to @ref update().
	static set<VuoModuleManager *> allModuleManagers;  ///< All module managers that have been constructed and not yet destroyed.
};

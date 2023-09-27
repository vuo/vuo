/**
 * @file
 * VuoModuleManager implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoModuleManager.hh"

#include "VuoCodeWindow.hh"
#include "VuoComposition.hh"
#include "VuoEditor.hh"
#include "VuoException.hh"
#include "VuoGenericType.hh"
#include "VuoNodeSet.hh"
#include "VuoSubcompositionMessageRouter.hh"
#include "VuoStringUtilities.hh"
#include "VuoCompiler.hh"
#include "VuoCompilerComposition.hh"
#include "VuoCompilerIssue.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerNodeClass.hh"
#include "VuoCompilerPortClass.hh"
#include "VuoCompilerSpecializedNodeClass.hh"
#include "VuoCompilerType.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorUtilities.hh"
#include "VuoErrorDialog.hh"
#include "VuoModule.hh"
#include "VuoNodeClass.hh"
#include "VuoRendererFonts.hh"

Q_DECLARE_METATYPE(vector<string>)
Q_DECLARE_METATYPE(vector<VuoCompilerNodeClass *>)
Q_DECLARE_METATYPE(vector<VuoCompilerType *>)
Q_DECLARE_METATYPE(ModulesModifiedSet)

set<VuoModuleManager *> VuoModuleManager::allModuleManagers;

/**
 * Constructs a VuoModuleManager that is ready to receive notifications when @a compiler loads/unloads modules.
 *
 * This object becomes responsible for deleting @a compiler.
 */
VuoModuleManager::VuoModuleManager(VuoCompiler *compiler)
{
	this->compiler = compiler;
	compiler->setDelegate(this);

	composition = nullptr;
	codeWindow = nullptr;
	nodeLibrary = nullptr;

	allModuleManagers.insert(this);

	qRegisterMetaType< vector<string> >();
	qRegisterMetaType< vector<VuoCompilerNodeClass *> >();
	qRegisterMetaType< vector<VuoCompilerType *> >();
	qRegisterMetaType< ModulesModifiedSet >();
	connect(this, &VuoModuleManager::loadedModulesAndReadyToUpdate, this, &VuoModuleManager::updateWithModulesBeingLoaded, Qt::QueuedConnection);
}

/**
 * Destroys this object after waiting for all pending work to complete.
 */
VuoModuleManager::~VuoModuleManager(void)
{
	composition = nullptr;
	codeWindow = nullptr;
	nodeLibrary = nullptr;

	for (auto callbackIter : callbackForNodeClass)
		Block_release(callbackIter.second.first);
	callbackForNodeClass.clear();

	allModuleManagers.erase(this);

	compiler->setDelegate(nullptr);
	disconnect(this, &VuoModuleManager::loadedModulesAndReadyToUpdate, this, &VuoModuleManager::updateWithModulesBeingLoaded);
	delete compiler;
}

/**
 * Sets the composition to update when modules are loaded/unloaded.
 */
void VuoModuleManager::setComposition(VuoEditorComposition *composition)
{
	this->composition = composition;
}

/**
 * Sets the code window to update when the code window's node class fails to load.
 */
void VuoModuleManager::setCodeWindow(VuoCodeWindow *codeWindow)
{
	this->codeWindow = codeWindow;
}

/**
 * Sets the node library to update when modules are loaded/unloaded.
 */
void VuoModuleManager::setNodeLibrary(VuoNodeLibrary *nodeLibrary)
{
	this->nodeLibrary = nodeLibrary;
	if (nodeLibrary)
	{
		nodeLibrary->setCompiler(compiler);
		nodeLibrary->setModuleManager(this);
	}
}

/**
 * Returns the node library currently being updated when modules are loaded/unloaded.
 *
 * May be null if the node library is detached.
 */
VuoNodeLibrary * VuoModuleManager::getNodeLibrary()
{
	return nodeLibrary;
}

/**
 * Returns all singleton (non-compound) types that have been loaded and not subsequently unloaded.
 *
 * This excludes the generic `VuoList` type, which is instead returned by VuoModuleManager::getLoadedGenericCompoundTypes().
 * It does include the `VuoDictionary_*_*` types.
 *
 * @param limitToSpecializationTargets If true, excludes the few types to which a generic type
 *    cannot be specialized due to implementation details.
 */
map<string, VuoCompilerType *> VuoModuleManager::getLoadedSingletonTypes(bool limitToSpecializationTargets)
{
	map<string, VuoCompilerType *> types;

	auto shouldCopy = [limitToSpecializationTargets] (pair<string, VuoCompilerType *> i)
	{
		return ! limitToSpecializationTargets || VuoRendererPort::isSpecializationImplementedForType(i.first);
	};

	std::copy_if(loadedSingletonTypes.begin(), loadedSingletonTypes.end(),
				 std::inserter(types, types.end()),
				 shouldCopy);

	return types;
}

/**
 * Returns all unspecialized generic compound types that have been unloaded and subsequently unloaded.
 */
map<string, VuoCompilerType *> VuoModuleManager::getLoadedGenericCompoundTypes(void)
{
	return loadedGenericCompoundTypes;
}

/**
 * Returns the names of all specializations of the `VuoList` compound type that could be created from the
 * singleton types that have been loaded. (The returned list types have not necessarily been loaded yet.)
 */
set<string> VuoModuleManager::getKnownListTypeNames(bool limitToSpecializationTargets)
{
	set<string> listTypes;

	map<string, VuoCompilerType *> singletonTypes = getLoadedSingletonTypes(limitToSpecializationTargets);
	for (auto i : singletonTypes)
		listTypes.insert(VuoType::listTypeNamePrefix + i.first);

	return listTypes;
}

/**
 * Returns the node sets from which at least one node class or type has been loaded (though is not necessarily
 * still loaded).
 */
set<string> VuoModuleManager::getKnownNodeSets(void)
{
	return knownNodeSets;
}

/**
 * Returns the name of the node set to which @a typeName belongs for the purpose of display in the editor
 * (not necessarily its actual node set), or an empty string if it's considered a core type not belonging
 * to any node set.
 */
string VuoModuleManager::getNodeSetForType(const string &typeName)
{
	// Treat as a core type (https://b33p.net/kosada/node/9465)
	if (typeName == "VuoLayer")
		return "";

	auto typeIter = loadedSingletonTypes.find(typeName);
	if (typeIter != loadedSingletonTypes.end())
	{
		VuoNodeSet *nodeSet = typeIter->second->getBase()->getNodeSet();
		if (nodeSet)
			return nodeSet->getName();
	}

	return getPrimaryAffiliatedNodeSetForType(typeName);
}

/**
 * Returns the type-converter node classes that can convert from the given from-type to the given to-type.
 *
 * If the from-type and to-type are both concrete types (as opposed to unspecialized generic types), then
 * only @a fromTypeName and @a toTypeName need to be passed. This function will return all type-converters where
 * the input port type is or can be specialized to @a fromTypeName and the output port type is or can be
 * specialized to @a toTypeName.
 *
 * If the from-type is an unspecialized generic type, then @a fromType must additionally be provided. This
 * function will return all type-converters where the input port type is generic and compatible with @a fromType.
 * Same idea if the to-type is an unspecialized generic type.
 */
vector<string> VuoModuleManager::getCompatibleTypecastClasses(const string &fromTypeName, VuoType *fromType, const string &toTypeName, VuoType *toType)
{
	vector<string> typeConverters;

	for (auto const &i : loadedTypeConverterNodeClasses)
		if (i.first.first->getModuleKey() == fromTypeName && i.first.second->getModuleKey() == toTypeName)
			typeConverters.insert(typeConverters.end(), i.second.cbegin(), i.second.cend());

	VuoGenericType *genericInType = dynamic_cast<VuoGenericType *>(fromType);
	VuoGenericType *genericOutType = dynamic_cast<VuoGenericType *>(toType);

	auto genericTypeCanSpecializeTo = [](VuoGenericType *genericType, const string &otherTypeName, VuoGenericType *otherTypeAsGeneric)
	{
		return otherTypeAsGeneric ?
					genericType->isGenericTypeCompatible(otherTypeAsGeneric) :
					(genericType->isSpecializedTypeCompatible(otherTypeName) && VuoRendererPort::isSpecializationImplementedForType(otherTypeName));
	};

	/**
	 * Returns true if converterInOut matches (or can be specialized to) fromType + toType.
	 *
	 * If so, and if the converter requires specialization, outputs the ordered specialized type names in `outSpecializations`.
	 */
	auto typeConverterCanSpecializeToQueriedTypes = [=](pair<VuoType *, VuoType *> converterInOut, vector<string> &outSpecializations)
	{
		VuoType *converterInType = converterInOut.first;
		VuoGenericType *converterGenericInType = dynamic_cast<VuoGenericType *>(converterInType);
		if (converterGenericInType)
		{
			if (! genericTypeCanSpecializeTo(converterGenericInType, fromTypeName, genericInType))
				return false;

			outSpecializations.push_back(fromTypeName);
		}
		else if (converterInType->getModuleKey() != fromTypeName)
			return false;

		VuoType *converterOutType = converterInOut.second;
		VuoGenericType *converterGenericOutType = dynamic_cast<VuoGenericType *>(converterOutType);
		if (converterGenericOutType)
		{
			if (! genericTypeCanSpecializeTo(converterGenericOutType, toTypeName, genericOutType))
				return false;

			outSpecializations.push_back(toTypeName);
		}
		else if (converterOutType->getModuleKey() != toTypeName)
			return false;

		if (converterGenericInType && converterGenericOutType &&
				VuoType::extractInnermostTypeName(converterInType->getModuleKey()) == VuoType::extractInnermostTypeName(converterOutType->getModuleKey()) &&
				VuoType::extractInnermostTypeName(fromTypeName) != VuoType::extractInnermostTypeName(toTypeName))
			return false;

		return true;
	};

	for (auto const &i : loadedTypeConverterNodeClasses)
	{
		vector<string> specializations;
		if (typeConverterCanSpecializeToQueriedTypes(i.first, specializations))
		{
			if (specializations.empty())
				typeConverters.insert(typeConverters.end(), i.second.cbegin(), i.second.cend());
			else
				for (string unspecializedTypeConverter : i.second)
					typeConverters.push_back(VuoCompilerSpecializedNodeClass::createSpecializedNodeClassName(unspecializedTypeConverter, specializations));
		}
	}

	return typeConverters;
}

/**
 * Schedules the block @a callback to be called the next time this module manager is notified that a
 * node class with the given name has been added or modified.
 */
void VuoModuleManager::doNextTimeNodeClassIsLoaded(const string &nodeClassName, CallbackType callback)
{
	cancelCallbackForNodeClass(nodeClassName);

	CallbackType callbackOnHeap = Block_copy(callback);
	callbackForNodeClass[nodeClassName] = make_pair(callbackOnHeap, false);
}

/**
 * Schedules the block @a callback to be called every time this module manager is notified that a
 * node class with the given name has been added or modified.
 */
void VuoModuleManager::doEveryTimeNodeClassIsLoaded(const string &nodeClassName, CallbackType callback)
{
	cancelCallbackForNodeClass(nodeClassName);

	CallbackType callbackOnHeap = Block_copy(callback);
	callbackForNodeClass[nodeClassName] = make_pair(callbackOnHeap, true);
}

/**
 * Cancels the callback previously scheduled by @ref doNextTimeNodeClassIsLoaded or @ref doEveryTimeNodeClassIsLoaded.
 */
void VuoModuleManager::cancelCallbackForNodeClass(const string &nodeClassName)
{
	auto callbackIter = callbackForNodeClass.find(nodeClassName);
	if (callbackIter != callbackForNodeClass.end())
	{
		Block_release(callbackIter->second.first);
		callbackForNodeClass.erase(callbackIter);
	}
}

/**
 * Returns the fully qualified node identifier (i.e., prefixed by composition identifier) of each instance of
 * the given node class that appears at any level in this module manager's composition.
 *
 * @param sourcePath The path of the node class's source code. If a node class with the same class name is
 *     installed at multiple locations accessible to the composition (e.g., user modules and composition-local modules),
 *     this function only returns instances of the node class if the composition actually uses the implementation
 *     at @a sourcePath.
 */
set<string> VuoModuleManager::findInstancesOfNodeClass(const string &sourcePath)
{
	set<string> compositionIdentifiers;

	VuoCompilerNodeClass *nodeClass = compiler->getNodeClass( VuoCompiler::getModuleKeyForPath(sourcePath) );
	if (nodeClass && VuoFileUtilities::arePathsEqual(nodeClass->getSourcePath(), sourcePath))
	{
		list< pair<string, VuoCompilerNodeClass *> > nodesToVisit;

		for (VuoNode *node : composition->getBase()->getNodes())
		{
			if (node->hasCompiler())
			{
				string fullyQualifiedNodeIdentifier = VuoStringUtilities::buildCompositionIdentifier(VuoCompilerComposition::topLevelCompositionIdentifier,
																									node->getCompiler()->getIdentifier());
				nodesToVisit.push_back(make_pair(fullyQualifiedNodeIdentifier, node->getNodeClass()->getCompiler()));
			}
		}

		while (! nodesToVisit.empty())
		{
			pair<string, VuoCompilerNodeClass *> v = nodesToVisit.front();
			string currNodeIdentifier = v.first;
			VuoCompilerNodeClass *currNodeClass = v.second;
			nodesToVisit.pop_front();

			if (currNodeClass == nodeClass)
				compositionIdentifiers.insert(currNodeIdentifier);
			else
			{
				for (pair<string, string> containedNode : currNodeClass->getContainedNodes())
				{
					VuoCompilerNodeClass *containedNodeClass = compiler->getNodeClass(containedNode.second);
					if (containedNodeClass)
					{
						string fullyQualifiedNodeIdentifier = VuoStringUtilities::buildCompositionIdentifier(currNodeIdentifier, containedNode.first);
						nodesToVisit.push_back(make_pair(fullyQualifiedNodeIdentifier, containedNodeClass));
					}
				}
			}
		}
	}

	return compositionIdentifiers;
}

/**
 * Compiler delegate method, called when modules are loaded/unloaded.
 */
void VuoModuleManager::loadedModules(const map<string, VuoCompilerModule *> &modulesAdded,
									 const map<string, pair<VuoCompilerModule *, VuoCompilerModule *> > &modulesModified,
									 const map<string, VuoCompilerModule *> &modulesRemoved, VuoCompilerIssues *issues)
{
	//VLog("%p -- %lu %lu %lu, %lu", this,
		 //modulesAdded.size(), modulesModified.size(), modulesRemoved.size(), issues->getList().size());
	//if (modulesAdded.size() == 1) VLog("    %s", modulesAdded.begin()->first.c_str());
	//if (modulesModified.size() == 1) VLog("    %s", modulesModified.begin()->first.c_str());
	//if (modulesRemoved.size() == 1) VLog("    %s", modulesRemoved.begin()->first.c_str());
	//if (! issues->isEmpty()) VLog("    %s", issues->getLongDescription(false).c_str());

	vector<VuoCompilerModule *> modulesAddedOrModified;
	for (auto m : modulesAdded)
		modulesAddedOrModified.push_back(m.second);
	for (auto m : modulesModified)
		modulesAddedOrModified.push_back(m.second.second);

	vector<VuoCompilerModule *> modulesRemovedOrModified;
	for (auto m : modulesRemoved)
		modulesRemovedOrModified.push_back(m.second);
	for (auto m : modulesModified)
		modulesRemovedOrModified.push_back(m.second.first);

	vector<VuoCompilerNodeClass *> nodeClassesAddedOrModified;
	vector<VuoCompilerType *> typesAddedOrModified;
	foreach (VuoCompilerModule *module, modulesAddedOrModified)
	{
		VuoCompilerNodeClass *nodeClass = dynamic_cast<VuoCompilerNodeClass *>(module);
		if (nodeClass)
			nodeClassesAddedOrModified.push_back(nodeClass);
		else
		{
			VuoCompilerType *type = dynamic_cast<VuoCompilerType *>(module);
			if (type)
				typesAddedOrModified.push_back(type);
		}
	}

	vector<string> nodeClassesRemovedOrModified;
	vector<string> typesRemovedOrModified;
	vector<string> librariesRemovedOrModified;
	foreach (VuoCompilerModule *module, modulesRemovedOrModified)
	{
		string moduleKey = module->getPseudoBase()->getModuleKey();
		VuoCompilerNodeClass *nodeClass = dynamic_cast<VuoCompilerNodeClass *>(module);
		if (nodeClass)
			nodeClassesRemovedOrModified.push_back(moduleKey);
		else
		{
			VuoCompilerType *type = dynamic_cast<VuoCompilerType *>(module);
			if (type)
				typesRemovedOrModified.push_back(moduleKey);
			else
				librariesRemovedOrModified.push_back(moduleKey);
		}
	}

	set< pair<VuoCompilerModule *, VuoCompilerModule *> > modulesModifiedSet;
	for (auto m : modulesModified)
		modulesModifiedSet.insert(m.second);

	emit loadedModulesAndReadyToUpdate(nodeClassesRemovedOrModified, nodeClassesAddedOrModified, typesRemovedOrModified, typesAddedOrModified,
									   librariesRemovedOrModified, modulesModifiedSet, issues);
}

/**
 * This function exists so that the call to `update()` can be scheduled on the main thread and canceled if the VuoModuleManager
 * is destroyed, by virtue of the call to this function being made through a signal-slot queued connection.
 */
void VuoModuleManager::updateWithModulesBeingLoaded(const vector<string> &nodeClassesToRemove, const vector<VuoCompilerNodeClass *> &nodeClassesToAdd,
													const vector<string> &typesToRemove, const vector<VuoCompilerType *> &typesToAdd,
													const vector<string> &librariesToRemove,
													const ModulesModifiedSet &modulesModified,
													VuoCompilerIssues *issues)
{
	update(nodeClassesToRemove, nodeClassesToAdd, typesToRemove, typesToAdd, librariesToRemove, modulesModified, issues);
	loadedModulesCompleted();
}

/**
 * Populates this class and the UI with existing modules, loaded before this VuoModuleManager was created.
 */
void VuoModuleManager::updateWithAlreadyLoadedModules(void)
{
	map<string, VuoCompilerNodeClass *> nodeClassesMap = compiler->getNodeClasses();
	map<string, VuoCompilerType *> typesMap = compiler->getTypes();

	vector<VuoCompilerNodeClass *> nodeClasses;
	for (map<string, VuoCompilerNodeClass *>::iterator i = nodeClassesMap.begin(); i != nodeClassesMap.end(); ++i)
		nodeClasses.push_back(i->second);

	vector<VuoCompilerType *> types;
	for (map<string, VuoCompilerType *>::iterator i = typesMap.begin(); i != typesMap.end(); ++i)
		types.push_back(i->second);

	VuoCompilerIssues issues;
	update(vector<string>(), nodeClasses, vector<string>(), types, vector<string>(), set< pair<VuoCompilerModule *, VuoCompilerModule *> >(), &issues);
}

/**
 * Updates the cached data structures stored by this class.
 *
 * Updates the UI:
 *    - the node library
 *    - the composition
 *
 * Displays a dialog for any errors in @a issues. Warnings are ignored.
 */
void VuoModuleManager::update(const vector<string> &nodeClassesToRemove, const vector<VuoCompilerNodeClass *> &nodeClassesToAdd,
							  const vector<string> &typesToRemove, const vector<VuoCompilerType *> &typesToAdd,
							  const vector<string> &librariesToRemove,
							  const set< pair<VuoCompilerModule *, VuoCompilerModule *> > &modulesModified,
							  VuoCompilerIssues *issues)
{
	// Update cached info.

	updateLoadedNodeClasses(nodeClassesToRemove, nodeClassesToAdd);
	updateLoadedTypes(typesToRemove, typesToAdd);

	if (composition)
	{
		// Check if the composition depends on any of the invalidated modules.

		map<string, VuoCompilerNodeClass *> dependenciesInvalidated;
		set<string> dependenciesLost;
		map<string, string> originalGenericNodeClassName;
		if (! (nodeClassesToRemove.empty() && typesToRemove.empty() && librariesToRemove.empty()) )
		{
			// Invalidate modules that are in nodeClassesToRemove, typesToRemove, or librariesToRemove.

			set<string> dependencies = compiler->getDependenciesForComposition(composition->getBase()->getCompiler());

			vector<string> sortedDependencies(dependencies.begin(), dependencies.end());
			std::sort(sortedDependencies.begin(), sortedDependencies.end());

			vector<string> sortedNodeClassesToRemove(nodeClassesToRemove.begin(), nodeClassesToRemove.end());
			std::sort(sortedNodeClassesToRemove.begin(), sortedNodeClassesToRemove.end());

			vector<string> sortedTypesToRemove(typesToRemove.begin(), typesToRemove.end());
			std::sort(sortedTypesToRemove.begin(), sortedTypesToRemove.end());

			vector<string> sortedLibrariesToRemove(librariesToRemove.begin(), librariesToRemove.end());
			std::sort(sortedLibrariesToRemove.begin(), sortedLibrariesToRemove.end());

			set<string> dependenciesInvalidatedSet;
			std::set_intersection(sortedNodeClassesToRemove.begin(), sortedNodeClassesToRemove.end(),
								  sortedDependencies.begin(), sortedDependencies.end(),
								  std::inserter(dependenciesInvalidatedSet, dependenciesInvalidatedSet.end()));
			std::set_intersection(sortedTypesToRemove.begin(), sortedTypesToRemove.end(),
								  sortedDependencies.begin(), sortedDependencies.end(),
								  std::inserter(dependenciesInvalidatedSet, dependenciesInvalidatedSet.end()));
			std::set_intersection(sortedLibrariesToRemove.begin(), sortedLibrariesToRemove.end(),
								  sortedDependencies.begin(), sortedDependencies.end(),
								  std::inserter(dependenciesInvalidatedSet, dependenciesInvalidatedSet.end()));

			// Invalidate node classes that depend on types in typesToRemove.

			for (VuoNode *node : composition->getBase()->getNodes())
			{
				if (node->getNodeClass()->hasCompiler())
				{
					set<string> nodeClassDependencies = node->getNodeClass()->getCompiler()->getDependencies();
					set<string> nodeClassDependenciesInvalidated;
					std::set_intersection(nodeClassDependencies.begin(), nodeClassDependencies.end(),
										  typesToRemove.begin(), typesToRemove.end(),
										  std::inserter(nodeClassDependenciesInvalidated, nodeClassDependenciesInvalidated.end()));

					if (! nodeClassDependenciesInvalidated.empty())
						dependenciesInvalidatedSet.insert(node->getNodeClass()->getClassName());
				}
			}

			// Invalidate node classes that are specializations of generic node classes in nodeClassesToRemove.

			for (VuoNode *node : composition->getBase()->getNodes())
			{
				if (node->getNodeClass()->hasCompiler())
				{
					VuoCompilerSpecializedNodeClass *specializedNodeClass = dynamic_cast<VuoCompilerSpecializedNodeClass *>(node->getNodeClass()->getCompiler());
					if (specializedNodeClass)
					{
						string genericNodeClassName = specializedNodeClass->getOriginalGenericNodeClassName();
						if (find(nodeClassesToRemove.begin(), nodeClassesToRemove.end(), genericNodeClassName) != nodeClassesToRemove.end())
						{
							dependenciesInvalidatedSet.insert(genericNodeClassName);
							originalGenericNodeClassName[ specializedNodeClass->getBase()->getClassName() ] = genericNodeClassName;
						}
					}
				}
			}

			// Divvy up dependenciesInvalidatedSet into dependenciesInvalidated and dependenciesLost.

			for (set<string>::iterator i = dependenciesInvalidatedSet.begin(); i != dependenciesInvalidatedSet.end(); ++i)
			{
				string dependency = *i;

				bool foundReplacement = false;
				for (set< pair<VuoCompilerModule *, VuoCompilerModule *> >::const_iterator j = modulesModified.begin(); j != modulesModified.end(); ++j)
				{
					if (j->first->getPseudoBase()->getModuleKey() == dependency)
					{
						VuoCompilerNodeClass *replacementNodeClass = dynamic_cast<VuoCompilerNodeClass *>(j->second);
						if (replacementNodeClass)
							dependenciesInvalidated[dependency] = replacementNodeClass;

						foundReplacement = true;
						break;
					}
				}

				if (! foundReplacement)
					dependenciesLost.insert(dependency);
			}
		}

		// Check if the composition depends on any missing node classes that have been added.

		map<string, VuoCompilerNodeClass *> dependenciesAdded;
		if (! nodeClassesToAdd.empty())
		{
			foreach (VuoNode *node, composition->getBase()->getNodes())
			{
				if (! node->getNodeClass()->hasCompiler())
				{
					string nodeClassName = node->getNodeClass()->getClassName();
					for (VuoCompilerNodeClass *addedNodeClass : nodeClassesToAdd)
					{
						string addedNodeClassName = addedNodeClass->getBase()->getClassName();
						if (addedNodeClassName == nodeClassName)
						{
							dependenciesAdded[nodeClassName] = addedNodeClass;
						}
						else if (VuoCompilerSpecializedNodeClass::isSpecializationOfNodeClass(nodeClassName, addedNodeClass))
						{
							dependenciesAdded[addedNodeClassName] = addedNodeClass;
							originalGenericNodeClassName[nodeClassName] = addedNodeClassName;
							break;
						}
					}
				}
			}
		}

		if (! (dependenciesInvalidated.empty() && dependenciesLost.empty() && dependenciesAdded.empty()) )
		{
			// Replace each node whose node class has been invalidated (removed or modified)
			// or whose previously missing node class has been added.

			composition->modifyComponents(^{

			foreach (VuoNode *node, composition->getBase()->getNodes())
			{
				string nodeClassName = node->getNodeClass()->getClassName();

				auto nameIter = originalGenericNodeClassName.find(nodeClassName);
				bool isSpecializedNodeClass = (nameIter != originalGenericNodeClassName.end());
				if (isSpecializedNodeClass)
					nodeClassName = nameIter->second;

				VuoCompilerNodeClass *replacementNodeClass = NULL;
				map<string, VuoCompilerNodeClass *>::const_iterator invIter = dependenciesInvalidated.find(nodeClassName);
				if (invIter != dependenciesInvalidated.end())
				{
					replacementNodeClass = invIter->second;
				}
				else
				{
					map<string, VuoCompilerNodeClass *>::const_iterator addIter = dependenciesAdded.find(nodeClassName);
					if (addIter != dependenciesAdded.end())
					{
						replacementNodeClass = addIter->second;
					}
				}

				VuoNode *replacementNode = nullptr;
				if (replacementNodeClass)
				{
					if (isSpecializedNodeClass)
						replacementNodeClass = compiler->getNodeClass( node->getNodeClass()->getClassName() );

					if (replacementNodeClass)
						replacementNode = composition->createBaseNode(replacementNodeClass, node);
				}
				else if (dependenciesLost.find(nodeClassName) != dependenciesLost.end())
				{
					replacementNode = composition->createNodeWithMissingImplementation(node->getNodeClass(), node);

					if (node->getNodeClass()->hasCompiler() && node->getNodeClass()->getCompiler()->isSubcomposition()) {
						static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->unlinkNodeInSupercompositionFromSubcomposition(composition, node->getCompiler()->getIdentifier());
					}
				}

				if (replacementNode)
				{
					composition->updateNodeImplementationInPlace(node->getRenderer(), replacementNode);
				}
			}

			});

			// If the composition is running, do a live-coding reload.

			VuoCompilerCompositionDiff *diffInfo = new VuoCompilerCompositionDiff();

			for (set< pair<VuoCompilerModule *, VuoCompilerModule *> >::const_iterator i = modulesModified.begin(); i != modulesModified.end(); ++i)
			{
				VuoCompilerNodeClass *oldNodeClass = dynamic_cast<VuoCompilerNodeClass *>(i->first);
				VuoCompilerNodeClass *newNodeClass = dynamic_cast<VuoCompilerNodeClass *>(i->second);

				if (oldNodeClass && newNodeClass)
					diffInfo->addNodeClassReplacement(oldNodeClass, newNodeClass);
				else
					diffInfo->addModuleReplacement(i->first->getPseudoBase()->getModuleKey());
			}

			string snapshotWithNewModules = composition->takeSnapshot();
			composition->updateRunningComposition(snapshotWithNewModules, snapshotWithNewModules, diffInfo, dependenciesLost);
		}
	}

	if (nodeLibrary)
	{
		// Update the node library.

		// Do this after updating the composition so focus won't be taken away from the node library,
		// turning highlighted node library items from blue to gray.

		nodeLibrary->updateNodeClassList(nodeClassesToRemove, nodeClassesToAdd);
	}

	// Call any callbacks that have been scheduled for the node classes added.

	for (vector<VuoCompilerNodeClass *>::const_iterator i = nodeClassesToAdd.begin(); i != nodeClassesToAdd.end(); ++i)
	{
		string nodeClassName = (*i)->getBase()->getClassName();
		auto callbackIter = callbackForNodeClass.find(nodeClassName);
		if (callbackIter != callbackForNodeClass.end())
		{
			CallbackType callback = callbackIter->second.first;
			callback();

			if (! callbackIter->second.second)
				cancelCallbackForNodeClass(nodeClassName);
		}
	}

	// Display any errors that this module manager is responsible for displaying.

	VuoCompilerIssues *errors = issues->getErrors();
	if (isResponsibleForReportingErrors(this, errors, nodeClassesToAdd))
	{
		if (! errors->isEmpty())
		{
			if (codeWindow)
				codeWindow->setIssues(errors);
			else
				showErrorDialog(errors);
		}
		else
		{
			if (codeWindow)
			{
				VuoCompilerIssues noIssues;
				codeWindow->setIssues(&noIssues);
			}
		}
	}
}

/**
 * Returns true if @a currManager is the one responsible for displaying @a errors,
 * false if some other VuoModuleManager instance is responsible.
 */
bool VuoModuleManager::isResponsibleForReportingErrors(VuoModuleManager *currManager, VuoCompilerIssues *errors,
													   const vector<VuoCompilerNodeClass *> &nodeClassesToAdd)
{
	// Order of precedence for handling errors:
	// 1. If the errors are for a node class whose code editor is open, the code editor gets them.
	// 2. If the errors are for a composition-local module, the composition window(s) at that scope get them.
	// 3. Otherwise, the app-wide module manager gets them.

	VuoModuleManager *managerWithCodeEditor = nullptr;
	VuoModuleManager *managerWithCompositionLocal = nullptr;
	VuoModuleManager *managerAppWide = nullptr;

	// Assumption: All errors pertain to the same composition / code editor.
	string issuePath;
	string nodeClassName;
	if (! errors->isEmpty())
	{
		VuoCompilerIssue issue = errors->getList().front();
		issuePath = issue.getFilePath();
		nodeClassName = VuoCompiler::getModuleKeyForPath(issuePath);
	}
	else if (! nodeClassesToAdd.empty())
	{
		issuePath = "";
		nodeClassName = nodeClassesToAdd.front()->getBase()->getClassName();
	}

	for (VuoModuleManager *m : allModuleManagers)
	{
		if (m->codeWindow)
		{
			if (! nodeClassName.empty())
			{
				if (m->codeWindow->getNodeClassName() == nodeClassName)
					managerWithCodeEditor = m;
			}
		}
		else if (m->composition)
		{
			if (! issuePath.empty())
			{
				string issueDir, file, ext;
				VuoFileUtilities::splitPath(issuePath, issueDir, file, ext);
				if (VuoFileUtilities::arePathsEqual(m->compiler->getCompositionLocalModulesPath(), issueDir))
					managerWithCompositionLocal = m;
			}
		}
		else
		{
			managerAppWide = m;
		}
	}

	VuoModuleManager *responsibleManager = (managerWithCodeEditor ? managerWithCodeEditor :
																	(managerWithCompositionLocal ? managerWithCompositionLocal :
																								   managerAppWide));
	return responsibleManager == currManager;
}

/**
 * Displays @a errors in a dialog.
 */
void VuoModuleManager::showErrorDialog(VuoCompilerIssues *errors)
{
	QString errorSummary = VuoEditor::tr("There was a problem loading one or more nodes into your library.");
	QString errorDisclosureDetails = QString::fromStdString(errors->getLongDescription(false));

	QStringList brokenModulePaths;
	foreach (VuoCompilerIssue issue, errors->getList())
		brokenModulePaths.append(QString::fromStdString(issue.getFilePath()));
	brokenModulePaths.removeDuplicates();

	QMessageBox messageBox;

	// On OS X, this combination of flags displays the minimize, maximimize, and close buttons, but all in a disabled state.
	messageBox.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowMaximizeButtonHint);

	VuoRendererFonts *fonts = VuoRendererFonts::getSharedFonts();
	messageBox.setFont(fonts->dialogHeadingFont());
	messageBox.setTextFormat(Qt::RichText);
	messageBox.setStandardButtons(QMessageBox::Discard | QMessageBox::Cancel);
	messageBox.setButtonText(QMessageBox::Discard, VuoEditor::tr("Move Broken Nodes to Trash"));
	messageBox.setButtonText(QMessageBox::Cancel, VuoEditor::tr("Not Now"));
	messageBox.setDefaultButton(QMessageBox::Discard);
	messageBox.setText(errorSummary);
	messageBox.setStyleSheet("#qt_msgbox_informativelabel, QMessageBoxDetailsText {" + fonts->getCSS(fonts->dialogBodyFont()) + "}");
	messageBox.setDetailedText(errorDisclosureDetails);

	// Give the "Not Now" button keyboard focus (without "Default" status) so that it can be activated by spacebar.
	static_cast<QPushButton *>(messageBox.button(QMessageBox::Cancel))->setAutoDefault(false);
	messageBox.button(QMessageBox::Cancel)->setFocus();

	messageBox.setIconPixmap(VuoEditorUtilities::vuoLogoForDialogs());

	if (messageBox.exec() == QMessageBox::Discard)
	{
		try
		{
			// Move broken modules to the trash.
			foreach (QString path, brokenModulePaths)
				VuoFileUtilities::moveFileToTrash(path.toUtf8().constData());
		}
		catch (const VuoException &e)
		{
			VuoErrorDialog::show(NULL, VuoEditor::tr("Couldn't move a broken node to the Trash"), e.what());
		}
	}
}

/**
 * Updates internal lists of node classes and node sets.
 */
void VuoModuleManager::updateLoadedNodeClasses(const vector<string> &nodeClassesToRemove,
											   const vector<VuoCompilerNodeClass *> &nodeClassesToAdd)
{
	for (VuoCompilerNodeClass *nodeClassToAdd : nodeClassesToAdd)
	{
		VuoNodeSet *nodeSet = nodeClassToAdd->getBase()->getNodeSet();
		if (nodeSet)
			knownNodeSets.insert(nodeSet->getName());
	}

	for (auto i = loadedTypeConverterNodeClasses.begin(); i != loadedTypeConverterNodeClasses.end(); ++i)
	{
		set<string> stillLoaded;
		std::set_difference(i->second.begin(), i->second.end(),
							nodeClassesToRemove.begin(), nodeClassesToRemove.end(),
							std::inserter(stillLoaded, stillLoaded.end()));
		i->second = stillLoaded;
	}

	for (VuoCompilerNodeClass *compilerNodeClass : nodeClassesToAdd)
	{
		VuoNodeClass *nodeClass = compilerNodeClass->getBase();

		// For type-converter node classes that have generic port types, only add the generic node class
		// (e.g. vuo.data.summarize), not specializations of the node class (e.g. vuo.data.summarize.VuoLayer).
		// Each generic node class will have unique VuoGenericTypes for its input port and output port, so it
		// will have a unique key in loadedTypeConverterNodeClasses.
		if (nodeClass->isTypecastNodeClass() &&
				! dynamic_cast<VuoCompilerSpecializedNodeClass *>(nodeClass->getCompiler()) &&
				! nodeClass->getDeprecated())
		{
			VuoPortClass *inPortClass = nodeClass->getInputPortClasses()[VuoNodeClass::unreservedInputPortStartIndex];
			VuoPortClass *outPortClass = nodeClass->getOutputPortClasses()[VuoNodeClass::unreservedOutputPortStartIndex];

			if (inPortClass->hasCompiler() && outPortClass->hasCompiler())
			{
				VuoType *inType = static_cast<VuoCompilerPortClass *>(inPortClass->getCompiler())->getDataVuoType();
				VuoType *outType = static_cast<VuoCompilerPortClass *>(outPortClass->getCompiler())->getDataVuoType();

				if (inType != outType)
					loadedTypeConverterNodeClasses[{inType, outType}].insert(nodeClass->getClassName());
			}
		}
	}
}

/**
 * Updates internal lists of types and node sets.
 */
void VuoModuleManager::updateLoadedTypes(const vector<string> &typesToRemove, const vector<VuoCompilerType *> &typesToAdd)
{
	for (auto i = loadedSingletonTypes.begin(); i != loadedSingletonTypes.end(); )
	{
		if (std::find(typesToRemove.begin(), typesToRemove.end(), i->first) != typesToRemove.end())
			i = loadedSingletonTypes.erase(i);
		else
			++i;
	}

	for (auto i = loadedGenericCompoundTypes.begin(); i != loadedGenericCompoundTypes.end(); )
	{
		if (std::find(typesToRemove.begin(), typesToRemove.end(), i->first) != typesToRemove.end())
			i = loadedGenericCompoundTypes.erase(i);
		else
			++i;
	}

	for (VuoCompilerType *typeToAdd : typesToAdd)
	{
		string typeName = typeToAdd->getBase()->getModuleKey();

		if (! VuoCompilerType::isListType(typeToAdd))
		{
			if (typeName == "VuoList")
				loadedGenericCompoundTypes[typeName] = typeToAdd;
			else
				loadedSingletonTypes[typeName] = typeToAdd;

			VuoNodeSet *nodeSet = typeToAdd->getBase()->getNodeSet();
			if (nodeSet)
				knownNodeSets.insert(nodeSet->getName());
		}
	}
}

/**
 * Returns the node set earmarked as being the primary associated node set for the
 * provided type, or the empty string if no such association has been made.
 */
string VuoModuleManager::getPrimaryAffiliatedNodeSetForType(const string &typeName)
{
	map<string, string> nodeSetForType;
	nodeSetForType["VuoAnchor"] = "vuo.layer";
	nodeSetForType["VuoAudioSamples"] = "vuo.audio";
	nodeSetForType["VuoBlendMode"] = "vuo.image";
	nodeSetForType["VuoCoordinateUnit"] = "vuo.window";
	nodeSetForType["VuoCubemap"] = "vuo.image";
	nodeSetForType["VuoCursor"] = "vuo.window";
	nodeSetForType["VuoCurve"] = "vuo.motion";
	nodeSetForType["VuoCurveEasing"] = "vuo.motion";
	nodeSetForType["VuoDiode"] = "vuo.math";
	nodeSetForType["VuoFont"] = "vuo.font";
	nodeSetForType["VuoHorizontalAlignment"] = "vuo.layer";
	nodeSetForType["VuoImageColorDepth"] = "vuo.image";
	nodeSetForType["VuoImageWrapMode"] = "vuo.image";
	nodeSetForType["VuoIntegerRange"] = "vuo.math";
	nodeSetForType["VuoInteraction"] = "vuo.ui";
	nodeSetForType["VuoInteractionType"] = "vuo.ui";
	nodeSetForType["VuoListPosition"] = "vuo.list";
	nodeSetForType["VuoLoopType"] = "vuo.motion";
	nodeSetForType["VuoMathExpressionList"] = "vuo.math";
	nodeSetForType["VuoMesh"] = "vuo.mesh";
	nodeSetForType["VuoModifierKey"] = "vuo.keyboard";
	nodeSetForType["VuoOrientation"] = "vuo.ui";
	nodeSetForType["VuoRange"] = "vuo.math";
	nodeSetForType["VuoRectangle"] = "vuo.image";
	nodeSetForType["VuoRenderedLayers"] = "vuo.ui";
	nodeSetForType["VuoScreen"] = "vuo.screen";
	nodeSetForType["VuoShader"] = "vuo.shader";
	nodeSetForType["VuoSortOrder"] = "vuo.list";
	nodeSetForType["VuoTextCase"] = "vuo.text";
	nodeSetForType["VuoTextComparison"] = "vuo.text";
	nodeSetForType["VuoTextSort"] = "vuo.text";
	nodeSetForType["VuoTransform"] = "vuo.transform";
	nodeSetForType["VuoTransform2d"] = "vuo.transform";
	nodeSetForType["VuoTree"] = "vuo.tree";
	nodeSetForType["VuoUrl"] = "vuo.url";
	nodeSetForType["VuoUuid"] = "vuo.ui"; // @todo hide?
	nodeSetForType["VuoVerticalAlignment"] = "vuo.layer";
	nodeSetForType["VuoWave"] = "vuo.motion";
	nodeSetForType["VuoWindowDescription"] = "vuo.window";
	nodeSetForType["VuoWindowProperty"] = "vuo.window";
	nodeSetForType["VuoWindowReference"] = "vuo.window";
	nodeSetForType["VuoWrapMode"] = "vuo.math";

	map<string, string>::iterator i = nodeSetForType.find(typeName);
	if (i != nodeSetForType.end())
		return i->second;
	else
		return "";
}

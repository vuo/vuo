/**
 * @file
 * VuoSubcompositionMessageRouter implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoFileUtilities.hh"
#include "VuoStringUtilities.hh"
#include "VuoCompiler.hh"
#include "VuoCompilerComposition.hh"
#include "VuoCompilerNode.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorWindow.hh"

#include "VuoSubcompositionMessageRouter.hh"

/**
 * Needed for `std::map::operator[]`.
 */
VuoSubcompositionMessageRouter::CompositionInfo::CompositionInfo(void)
{
	window = NULL;
	topLevelComposition = NULL;
	supercomposition = NULL;
	nodeIdentifierInSupercomposition = "";
	compositionIdentifier = "";
}

/**
 * Creates a new composition info that is not yet linked to a supercomposition.
 */
VuoSubcompositionMessageRouter::CompositionInfo::CompositionInfo(VuoEditorWindow *window)
{
	this->window = window;
	reset();
}

/**
 * Resets the composition info to its original state, before it was linked to any supercomposition.
 */
void VuoSubcompositionMessageRouter::CompositionInfo::reset(void)
{
	topLevelComposition = window->getComposition();
	supercomposition = NULL;
	nodeIdentifierInSupercomposition = "";
	compositionIdentifier = VuoCompilerComposition::topLevelCompositionIdentifier;
}

/**
 * Creates a new message router with no compositions registered yet.
 */
VuoSubcompositionMessageRouter::VuoSubcompositionMessageRouter(void)
{
}

/**
 * Destructor. Waits for any member functions in progress on other threads to complete.
 */
VuoSubcompositionMessageRouter::~VuoSubcompositionMessageRouter(void)
{
	compositionInfosMutex.lock();
	compositionInfosMutex.unlock();
}

/**
 * Registers the composition in the message router, so that it can start sending and receiving messages.
 *
 * The composition is registered as a top-level composition, not yet associated with any supercomposition
 * or subcompositions.
 */
void VuoSubcompositionMessageRouter::addComposition(VuoEditorWindow *window)
{
	compositionInfosMutex.lock();

	compositionInfos[window->getComposition()] = CompositionInfo(window);

	compositionInfosMutex.unlock();
}

/**
 * Unregisters the composition, so that it can no longer send or receive messages.
 */
void VuoSubcompositionMessageRouter::removeComposition(VuoEditorWindow *window)
{
	compositionInfosMutex.lock();

	map<VuoEditorComposition *, CompositionInfo> *cis[] = { &compositionInfos, &unlinkedCompositionInfos };
	for (int i = 0; i < 2; ++i)
	{
		auto curr = (*cis[i]).find(window->getComposition());
		if (curr != (*cis[i]).end())
			(*cis[i]).erase(curr);

		for (auto &c : *cis[i])
			if (c.second.topLevelComposition == window->getComposition())
				c.second.reset();
	}

	compositionInfosMutex.unlock();
}

/**
 * Informs the message router that @a subcompositionInstance corresponds to @a nodeInSupercomposition in
 * @a supercomposition, so that messages can be routed up and down the composition hierarchy.
 */
void VuoSubcompositionMessageRouter::linkSubcompositionToNodeInSupercomposition(VuoEditorComposition *subcompositionInstance,
																				VuoEditorComposition *supercomposition,
																				VuoRendererNode *nodeInSupercomposition)
{
	compositionInfosMutex.lock();

	auto sub = compositionInfos.find(subcompositionInstance);
	if (sub != compositionInfos.end())
	{
		auto super = compositionInfos.find(supercomposition);
		if (super != compositionInfos.end())
		{
			sub->second.topLevelComposition = super->second.topLevelComposition;
			sub->second.supercomposition = supercomposition;
			sub->second.nodeIdentifierInSupercomposition = nodeInSupercomposition->getBase()->getCompiler()->getIdentifier();
			sub->second.compositionIdentifier = VuoStringUtilities::buildCompositionIdentifier(super->second.compositionIdentifier,
																							  nodeInSupercomposition->getBase()->getCompiler()->getIdentifier());
		}
	}

	compositionInfosMutex.unlock();
}

/**
 * Dissociates this subcomposition from the node in the supercomposition that it was previously linked to (if any),
 * changing the subcomposition back to a top-lvel composition as far as message routing is concerned.
 */
void VuoSubcompositionMessageRouter::unlinkSubcompositionFromNodeInSupercomposition(VuoEditorComposition *subcompositionInstance)
{
	compositionInfosMutex.lock();

	auto sub = compositionInfos.find(subcompositionInstance);
	if (sub != compositionInfos.end())
		compositionInfos[sub->first].reset();

	sub = unlinkedCompositionInfos.find(subcompositionInstance);
	if (sub != unlinkedCompositionInfos.end())
		unlinkedCompositionInfos.erase(sub);

	compositionInfosMutex.unlock();
}

/**
 * Dissociates this node in the supercomposition from the subcomposition that it was previously linked to (if any),
 * changing the subcomposition back to a top-lvel composition as far as message routing is concerned.
 */
void VuoSubcompositionMessageRouter::unlinkNodeInSupercompositionFromSubcomposition(VuoEditorComposition *supercomposition,
																					const string &nodeIdentifier)
{
	compositionInfosMutex.lock();

	for (auto &c : compositionInfos)
	{
		if (c.second.supercomposition == supercomposition && c.second.nodeIdentifierInSupercomposition == nodeIdentifier)
		{
			unlinkedCompositionInfos[c.first] = c.second;
			c.second.reset();
		}
	}

	compositionInfosMutex.unlock();
}

/**
 * Undoes the most recent call to @ref unlinkNodeInSupercompositionFromSubcomposition for this supercomposition and node,
 * re-associating the subcomposition that was previously linked to them (if any), unless that subcomposition has already
 * been removed.
 */
void VuoSubcompositionMessageRouter::relinkNodeInSupercompositionToSubcomposition(VuoEditorComposition *supercomposition,
																				  const string &nodeIdentifier)
{
	compositionInfosMutex.lock();

	for (map<VuoEditorComposition *, CompositionInfo>::iterator i = unlinkedCompositionInfos.begin(); i != unlinkedCompositionInfos.end(); ++i)
	{
		if (i->second.supercomposition == supercomposition && i->second.nodeIdentifierInSupercomposition == nodeIdentifier)
		{
			compositionInfos[i->first] = i->second;
			unlinkedCompositionInfos.erase(i);
			break;
		}
	}

	compositionInfosMutex.unlock();
}

/**
 * Returns the composition identifier of @a composition if it has been registered with @ref addComposition
 * and, if a subcomposition, recorded as part of a composition hierarchy by @ref linkSubcompositionToNodeInSupercomposition;
 * otherwise, an empty string.
 */
string VuoSubcompositionMessageRouter::getCompositionIdentifier(VuoEditorComposition *composition)
{
	compositionInfosMutex.lock();

	string compositionIdentifier;
	auto curr = compositionInfos.find(composition);
	if (curr != compositionInfos.end())
		compositionIdentifier = curr->second.compositionIdentifier;

	compositionInfosMutex.unlock();

	return compositionIdentifier;
}

/**
 * Calls @a doForComposition for the top-level composition associated with @a currComposition, which is either
 * @a currComposition itself or in the composition hierarchy established by @ref linkSubcompositionToNodeInSupercomposition.
 *
 * The top-level composition and @a currComposition's composition identifier are passed to @a doForComposition.
 */
void VuoSubcompositionMessageRouter::applyToLinkedTopLevelComposition(VuoEditorComposition *currComposition,
																	  CompositionCallbackWithIdentifier doForComposition)
{
	compositionInfosMutex.lock_shared();

	auto curr = compositionInfos.find(currComposition);
	if (curr != compositionInfos.end())
		doForComposition(curr->second.topLevelComposition, curr->second.compositionIdentifier);

	compositionInfosMutex.unlock_shared();
}

/**
 * Calls @a doForComposition for the composition associated with @a currComposition, in the hierarchy established by
 * @ref linkSubcompositionToNodeInSupercomposition, whose composition identifier is @a compositionIdentifier (if any).
 *
 * The composition whose identifier is @a compositionIdentifier is passed to @a doForComposition.
 */
void VuoSubcompositionMessageRouter::applyToLinkedCompositionWithIdentifier(VuoEditorComposition *currComposition,
																			string compositionIdentifier,
																			CompositionCallback doForComposition)
{
	compositionInfosMutex.lock_shared();

	auto curr = compositionInfos.find(currComposition);
	if (curr != compositionInfos.end())
		for (auto c : compositionInfos)
			if (c.second.topLevelComposition == curr->second.topLevelComposition && c.second.compositionIdentifier == compositionIdentifier)
				doForComposition(c.first);

	compositionInfosMutex.unlock_shared();
}

/**
 * Calls @a doForComposition for @a currComposition and all compositions associated with it in the hierarchy
 * established by @ref linkSubcompositionToNodeInSupercomposition.
 *
 * Each composition in the hierarchy and its composition identifier are passed to @a doForComposition.
 */
void VuoSubcompositionMessageRouter::applyToAllLinkedCompositions(VuoEditorComposition *currComposition,
																  CompositionCallbackWithIdentifier doForComposition)
{
	compositionInfosMutex.lock_shared();

	auto curr = compositionInfos.find(currComposition);
	if (curr != compositionInfos.end())
		for (auto c : compositionInfos)
			if (c.second.topLevelComposition == curr->second.topLevelComposition)
				doForComposition(c.first, c.second.compositionIdentifier);

	compositionInfosMutex.unlock_shared();
}

/**
 * Calls @a doForComposition for all top-level compositions that have been registered with @ref addComposition
 * except for @a currComposition.
 *
 * Each top-level composition is passed to @a doForComposition.
 */
void VuoSubcompositionMessageRouter::applyToAllOtherTopLevelCompositions(VuoEditorComposition *currComposition,
																		 CompositionCallback doForComposition)
{
	compositionInfosMutex.lock_shared();

	for (auto c : compositionInfos)
		if (c.first != currComposition && c.first == c.second.topLevelComposition)
			doForComposition(c.first);

	compositionInfosMutex.unlock_shared();
}

/**
 * Calls @a doForComposition for @a currComposition if it is installed as a subcomposition.
 *
 * @a currComposition and its file path are passed to @a doForComposition.
 */
void VuoSubcompositionMessageRouter::applyIfInstalledAsSubcomposition(VuoEditorComposition *currComposition,
																	  CompositionCallbackWithPath doForComposition)
{
	compositionInfosMutex.lock_shared();

	auto curr = compositionInfos.find(currComposition);
	if (curr != compositionInfos.end())
	{
		string compositionPath = curr->second.window->windowFilePath().toStdString();
		if (VuoFileUtilities::isInstalledAsModule(compositionPath))
			doForComposition(currComposition, compositionPath);
	}

	compositionInfosMutex.unlock_shared();
}

/**
 * Calls @a doForComposition for all compositions that have been registered with @ref addComposition and are
 * installed as subcompositions, except for @a currComposition.
 *
 * Each composition and its file path are passed to @a doForComposition.
 */
void VuoSubcompositionMessageRouter::applyToAllOtherCompositionsInstalledAsSubcompositions(VuoEditorComposition *currComposition,
																						   CompositionCallbackWithPath doForComposition)
{
	compositionInfosMutex.lock_shared();

	for (auto c : compositionInfos)
		if (c.first != currComposition)
			applyIfInstalledAsSubcomposition(c.first, doForComposition);

	compositionInfosMutex.unlock_shared();
}

/**
 * @file
 * VuoSubcompositionMessageRouter interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoEditorComposition;
class VuoEditorWindow;
class VuoRendererNode;
#include "shared_mutex.hh"

/**
 * Facilitates communication between open subcompositions and open compositions that contain instances of them.
 */
class VuoSubcompositionMessageRouter
{
public:
	VuoSubcompositionMessageRouter(void);
	~VuoSubcompositionMessageRouter(void);

	void addComposition(VuoEditorWindow *window);
	void removeComposition(VuoEditorWindow *window);
	void linkSubcompositionToNodeInSupercomposition(VuoEditorComposition *subcompositionInstance, VuoEditorComposition *supercomposition, VuoRendererNode *nodeInSupercomposition);
	void unlinkSubcompositionFromNodeInSupercomposition(VuoEditorComposition *subcompositionInstance);
	void unlinkNodeInSupercompositionFromSubcomposition(VuoEditorComposition *supercomposition, const string &nodeIdentifier);
	void relinkNodeInSupercompositionToSubcomposition(VuoEditorComposition *supercomposition, const string &nodeIdentifier);
	string getCompositionIdentifier(VuoEditorComposition *composition);

	/// @{ Callbacks.
	typedef void (^CompositionCallback)(VuoEditorComposition *);
	typedef void (^CompositionCallbackWithIdentifier)(VuoEditorComposition *, string);
	typedef void (^CompositionCallbackWithPath)(VuoEditorComposition *, string);
	/// @}
	void applyToLinkedTopLevelComposition(VuoEditorComposition *currComposition, CompositionCallbackWithIdentifier doForComposition);
	void applyToLinkedCompositionWithIdentifier(VuoEditorComposition *currComposition, string compositionIdentifier, CompositionCallback doForComposition);
	void applyToAllLinkedCompositions(VuoEditorComposition *currComposition, CompositionCallbackWithIdentifier doForComposition);
	void applyToAllOtherTopLevelCompositions(VuoEditorComposition *currComposition, CompositionCallback doForComposition);
	void applyIfInstalledAsSubcomposition(VuoEditorComposition *currComposition, CompositionCallbackWithPath doForComposition);
	void applyToAllOtherCompositionsInstalledAsSubcompositions(VuoEditorComposition *currComposition, CompositionCallbackWithPath doForComposition);

private:
	class CompositionInfo
	{
	public:
		CompositionInfo(void);
		CompositionInfo(VuoEditorWindow *window);
		void reset(void);

		VuoEditorWindow *window;  ///< The window that contains the composition.
		VuoEditorComposition *topLevelComposition;  ///< Either the composition itself or the top-level composition that (directly or indirectly) contains an instance of it as a subcomposition.
		VuoEditorComposition *supercomposition;  ///< The composition that (directly) contains an instance of this composition as a subcomposition.
		string nodeIdentifierInSupercomposition;  ///< The node identifier of the instance of this composition in its supercomposition.
		string compositionIdentifier;  ///< Fully qualified composition identifier in the runtime.
	};

	map<VuoEditorComposition *, CompositionInfo> compositionInfos;
	ting::shared_mutex compositionInfosMutex;

	map<VuoEditorComposition *, CompositionInfo> unlinkedCompositionInfos;
};

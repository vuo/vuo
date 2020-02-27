/**
 * @file
 * VuoCommandCommon interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoCompiler;
class VuoCompilerCompositionDiff;
class VuoCompositionMetadata;
class VuoEditorComposition;
class VuoEditorWindow;
class VuoNode;
class VuoPort;
class VuoPublishedPort;
class VuoRendererCable;
class VuoRendererNode;

/**
 * Helper functions for undoable actions.
 */
class VuoCommandCommon : public QUndoCommand
{
public:
	static const int moveCommandID = 1;  ///< ID for VuoCommandMove
	static const int addCommandID = 2;  ///< ID for VuoCommandAdd
	static const int removeCommandID = 3;  ///< ID for VuoCommandRemove
	static const int connectCommandID = 4;  ///< ID for VuoCommandConnect
	static const int setPortConstantCommandID = 5;  ///< ID for VuoCommandSetPortConstant
	static const int setNodeTitleCommandID = 6;  ///< ID for VuoCommandSetNodeTitle
	static const int setPublishedPortNameCommandID = 7;  ///< ID for VuoCommandSetPublishedPortName
	static const int publishPortCommandID = 8;  ///< ID for VuoCommandPublishPort
	static const int unpublishPortCommandID = 9;  ///< ID for VuoCommandUnpublishPort
	static const int setItemTintCommandID = 10;  ///< ID for VuoCommandSetItemTint
	static const int replaceNodeCommandID = 11;  ///< ID for VuoCommandReplaceNode
	static const int setTriggerThrottlingCommandID = 12;  ///< ID for VuoCommandSetTriggerThrottling
	static const int setPublishedPortDetailsCommandID = 13;  ///< ID for VuoCommandSetPublishedPortDetails
	static const int setCableHiddenCommandID = 14;  ///< ID for VuoCommandSetCableHidden
	static const int addPublishedPortCommandID = 15;  ///< ID for VuoCommandAddPublishedPort
	static const int removeProtocolPortCommandID = 16;  ///< ID for VuoCommandRemoveProtocolPort
	static const int reorderPublishedPortsCommandID = 17;  ///< ID for VuoCommandReorderPublishedPorts
	static const int setMetadataCommandID = 18;  ///< ID for VuoCommandSetMetadata
	static const int setCommentTextCommandID = 19;  ///< ID for VuoCommandSetCommentText
	static const int resizeCommentCommandID = 20;  ///< ID for VuoCommandResizeComment
	static const int changeNodeCommandID = 21;  ///< ID for VuoCommandChangeNode

	static void addCable(VuoRendererCable *rc, VuoPort *fromPortAfterAdding, VuoPort *toPortAfterAdding, VuoEditorComposition *composition);
	static void removeCable(VuoRendererCable *rc, VuoEditorComposition *composition);
	static void updateCable(VuoRendererCable *rc, VuoPort *updatedFromPort, VuoPort *updatedToPort, VuoEditorComposition *composition, bool preserveDanglingCables=false);

	static VuoPublishedPort * publishInternalPort(VuoPort *internalPort, bool forceEventOnlyPublication, string publishedPortName, VuoEditorComposition *composition, bool attemptMerge);
	static VuoPublishedPort * publishInternalExternalPortCombination(VuoPort *internalPort, VuoPublishedPort *externalPort, bool forceEventOnlyPublication, VuoEditorComposition *composition);
	static void unpublishInternalExternalPortCombination(VuoPort *internalPort, VuoPublishedPort *externalPort, VuoEditorComposition *composition, bool unpublishIsolatedExternalPorts);
	static VuoCompilerCompositionDiff * addNodeReplacementToDiff(VuoCompilerCompositionDiff *diffInfo, VuoRendererNode *oldNode, VuoRendererNode *newNode, map<VuoPort *, VuoPort *> updatedPortForOriginalPort, VuoEditorComposition *composition);

	VuoCommandCommon(VuoEditorWindow *window);
	~VuoCommandCommon();
	void setDescription(const char *formatString, ...) __attribute__((format(printf, 2, 3)));

protected:
	VuoEditorWindow *window;  ///< The window this command occurred in.
	char *description;  ///< See @ref setDescription.

private:
	static bool compositionContainsNode(VuoEditorComposition *composition, VuoNode *node);
};

/**
 * Logs the description (if any).
 *
 * This is a macro so the calling method's filename and line appear in the log message.
 */
#define VuoCommandCommon_redo \
	if (description) \
		VUserLog("%s:      %s", window->getWindowTitleWithoutPlaceholder().toUtf8().data(), description)

/**
 * Logs the description (if any).
 *
 * This is a macro so the calling method's filename and line appear in the log message.
 */
#define VuoCommandCommon_undo \
	if (description) \
		VUserLog("%s: Undo %s", window->getWindowTitleWithoutPlaceholder().toUtf8().data(), description)

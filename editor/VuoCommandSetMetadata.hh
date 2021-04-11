/**
 * @file
 * VuoCommandSetMetadata interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCommandCommon.hh"

class VuoEditorWindow;
class VuoCompiler;
class VuoCompositionMetadata;

/**
 * An undoable action for setting a composition's metadata.
 */
class VuoCommandSetMetadata : public VuoCommandCommon
{
public:
	VuoCommandSetMetadata(VuoCompositionMetadata *metadata, VuoEditorWindow *window);

	int id() const;
	void undo();
	void redo();

private:
	void updateDefaultCompositionName(VuoCompositionMetadata *metadata);

	static const int commandID;
	VuoEditorWindow *window;

	VuoCompositionMetadata *revertedMetadata;
	VuoCompositionMetadata *updatedMetadata;
};

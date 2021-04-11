/**
 * @file
 * VuoCommandAddPublishedPort implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCommandAddPublishedPort.hh"
#include "VuoCommandCommon.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoCompilerPublishedPortClass.hh"
#include "VuoCompilerInputData.hh"
#include "VuoCompilerInputDataClass.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorWindow.hh"

/**
 * Creates a command for adding an externally visible published port without any
 * composition-internal connections.
 */
VuoCommandAddPublishedPort::VuoCommandAddPublishedPort(VuoRendererPublishedPort *publishedPort, VuoEditorWindow *window)
	: VuoCommandCommon(window)
{
	setText(QApplication::translate("VuoEditorWindow", "Add Published Port"));
	this->window = window;
	this->revertedSnapshot = window->getComposition()->takeSnapshot();

	// Start of command content.
	{
		bool isPublishedInput = !publishedPort->getInput();
		window->getComposition()->addPublishedPort(dynamic_cast<VuoPublishedPort *>(publishedPort->getBase()), isPublishedInput);

		string typeName = "event";
		VuoType *vuoType = dynamic_cast<VuoCompilerPublishedPortClass *>(publishedPort->getBase()->getClass()->getCompiler())->getDataVuoType();
		if (vuoType)
			typeName = vuoType->getModuleKey();
		setDescription("Add published %s port %s %s",
					   isPublishedInput ? "input" : "output",
					   typeName.c_str(),
					   publishedPort->getBase()->getClass()->getName().c_str());
	}
	// End of command content.

	this->updatedSnapshot = window->getComposition()->takeSnapshot();
}

/**
 * Returns the ID of this command.
 */
int VuoCommandAddPublishedPort::id() const
{
	return VuoCommandCommon::addPublishedPortCommandID;
}

/**
 * Undoes addition of the published port.
 */
void VuoCommandAddPublishedPort::undo()
{
	VuoCommandCommon_undo;

	window->resetCompositionWithSnapshot(revertedSnapshot);
	window->coalesceSnapshots(updatedSnapshot, revertedSnapshot);
}

/**
 * Adds the published port.
 */
void VuoCommandAddPublishedPort::redo()
{
	VuoCommandCommon_redo;

	window->resetCompositionWithSnapshot(updatedSnapshot);
	window->coalesceSnapshots(revertedSnapshot, updatedSnapshot);
}

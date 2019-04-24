/**
 * @file
 * VuoInputEditorBlackmagicOutputDevice implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorBlackmagicOutputDevice.hh"

#include "VuoComboBox.hh"

extern "C"
{
#include "VuoBlackmagic.h"
#include "VuoList_VuoBlackmagicConnection.h"
#include "VuoList_VuoBlackmagicVideoMode.h"
}

/**
 * Constructs a VuoInputEditorBlackmagicOutputDevice object.
 */
VuoInputEditor * VuoInputEditorBlackmagicOutputDeviceFactory::newInputEditor()
{
	return new VuoInputEditorBlackmagicOutputDevice();
}

/**
 * Sets up a dialog containing drop-down menus for device, connection, and video mode.
 */
void VuoInputEditorBlackmagicOutputDevice::setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details)
{
	VuoBlackmagicOutputDevice originalDevice = VuoBlackmagicOutputDevice_makeFromJson(originalValue);

	{
		deviceComboBox = new VuoComboBox();

		bool foundOriginalDevice = false;
		deviceComboBox->addItem("First available device", QVariant::fromValue((void *)NULL));
		if (VuoText_isEmpty(originalDevice.name))
		{
			deviceComboBox->setCurrentIndex(0);
			foundOriginalDevice = true;
		}

		deviceComboBox->insertSeparator(1);

		deviceComboBox->addItem("Specific device");
		deviceComboBox->setItemEnabled(deviceComboBox->count() - 1, false);

		VuoList_VuoBlackmagicOutputDevice devices = VuoBlackmagic_getOutputDevices(VuoBlackmagicConnection_Composite, VuoBlackmagicVideoMode_NTSC);
		unsigned long deviceCount = VuoListGetCount_VuoBlackmagicOutputDevice(devices);
		if (deviceCount)
			for (unsigned long i = 0; i < deviceCount; ++i)
			{
				VuoBlackmagicOutputDevice device = VuoListGetValue_VuoBlackmagicOutputDevice(devices, i+1);
				deviceComboBox->addItem(VuoText_format("      %s", device.name), QVariant::fromValue((void *)VuoBlackmagicOutputDevice_getJson(device)));

				if (!foundOriginalDevice
				 && strcmp(device.name, originalDevice.name) == 0)
				{
					deviceComboBox->setCurrentIndex(deviceComboBox->count() - 1);
					foundOriginalDevice = true;
				}
			}

		// If none of the above devices matched, add a disabled item for the original device, allowing it to remain selected.
		if (!foundOriginalDevice)
		{
			deviceComboBox->addItem(VuoText_format("      %s", originalDevice.name), QVariant::fromValue((void *)originalValue));
			int index = deviceComboBox->count() - 1;
			deviceComboBox->setCurrentIndex(index);
			deviceComboBox->setItemEnabled(index, false);

		}
		else if (!deviceCount)
		{
			deviceComboBox->addItem("      (no devices found)");
			deviceComboBox->setItemEnabled(deviceComboBox->count() - 1, false);
		}
	}

	{
		connectionComboBox = new VuoComboBox();

		VuoList_VuoBlackmagicConnection connections = VuoBlackmagicConnection_getAllowedValues();
		unsigned long connectionCount = VuoListGetCount_VuoBlackmagicConnection(connections);
		for (unsigned long i = 0; i < connectionCount; ++i)
		{
			VuoBlackmagicConnection connection = VuoListGetValue_VuoBlackmagicConnection(connections, i+1);
			connectionComboBox->addItem(VuoBlackmagicConnection_getSummary(connection), QVariant::fromValue((int)connection));
		}

		connectionComboBox->setCurrentText(VuoBlackmagicConnection_getSummary(originalDevice.connection));
	}

	{
		videoModeComboBox = new VuoComboBox();

		VuoList_VuoBlackmagicVideoMode modes = VuoBlackmagicVideoMode_getAllowedValues();
		unsigned long modeCount = VuoListGetCount_VuoBlackmagicVideoMode(modes);
		for (unsigned long i = 0; i < modeCount; ++i)
		{
			VuoBlackmagicVideoMode mode = VuoListGetValue_VuoBlackmagicVideoMode(modes, i+1);
			videoModeComboBox->addItem(VuoBlackmagicVideoMode_getSummary(mode), QVariant::fromValue((int)mode));
		}

		videoModeComboBox->setCurrentText(VuoBlackmagicVideoMode_getSummary(originalDevice.videoMode));
	}

	updateConnectionAndModeStatus();

	QFormLayout *layout = new QFormLayout(&dialog);
	layout->setContentsMargins(4, 4, 16, 4);
	layout->setSpacing(4);
	layout->addRow(tr("Device"), deviceComboBox);
	layout->addRow(tr("Connection"), connectionComboBox);
	layout->addRow(tr("Video Mode"), videoModeComboBox);
	layout->setGeometry(QRect(0, 0, 280, 100));
	dialog.setLayout(layout);

	typedef void (QComboBox::*QComboIntSignal)(int);
	connect(deviceComboBox,    static_cast<QComboIntSignal>(&QComboBox::currentIndexChanged), this, &VuoInputEditorBlackmagicOutputDevice::emitValueChanged);
	connect(connectionComboBox,    static_cast<QComboIntSignal>(&QComboBox::currentIndexChanged), this, &VuoInputEditorBlackmagicOutputDevice::emitValueChanged);
	connect(videoModeComboBox, static_cast<QComboIntSignal>(&QComboBox::currentIndexChanged), this, &VuoInputEditorBlackmagicOutputDevice::emitValueChanged);
}

/**
 * Returns the current values held in the child widgets.
 */
json_object *VuoInputEditorBlackmagicOutputDevice::getAcceptedValue(void)
{
	VuoBlackmagicOutputDevice device = VuoBlackmagicOutputDevice_makeFromJson((json_object *)deviceComboBox->currentData().value<void *>());
	device.connection = (VuoBlackmagicConnection)connectionComboBox->currentData().value<int>();
	device.videoMode = (VuoBlackmagicVideoMode)videoModeComboBox->currentData().value<int>();
	return VuoBlackmagicOutputDevice_getJson(device);
}

/**
 * Updates the enabled/disabled status of the connection and mode combo box items,
 * based on the capabilities of the currently-selected device.
 */
void VuoInputEditorBlackmagicOutputDevice::updateConnectionAndModeStatus()
{
	VuoBlackmagicOutputDevice device = VuoBlackmagicOutputDevice_makeFromJson((json_object *)deviceComboBox->currentData().value<void *>());

	// If "First" is selected, enable all connections and modes.
	if (VuoText_isEmpty(device.name))
	{
		int items = connectionComboBox->count();
		for (int i = 0; i < items; ++i)
			connectionComboBox->setItemEnabled(i, true);

		items = videoModeComboBox->count();
		for (int i = 0; i < items; ++i)
			videoModeComboBox->setItemEnabled(i, true);
	}

	// If a specific device is selected, only enable connections and modes compatible with it.
	else
	{
		VuoList_VuoBlackmagicConnection connectionList = VuoBlackmagic_getSupportedOutputConnections(device);
		long connectionCount = VuoListGetCount_VuoBlackmagicConnection(connectionList);
		VuoBlackmagicConnection *connections = VuoListGetData_VuoBlackmagicConnection(connectionList);
		int items = connectionComboBox->count();
		for (int i = 0; i < items; ++i)
		{
			bool found = false;
			for (int j = 0; j < connectionCount; ++j)
				if (connections[j] == connectionComboBox->itemData(i))
				{
					found = true;
					break;
				}

			connectionComboBox->setItemEnabled(i, found);
		}

		VuoList_VuoBlackmagicVideoMode modeList = VuoBlackmagic_getSupportedOutputVideoModes(device);
		long modeCount = VuoListGetCount_VuoBlackmagicVideoMode(modeList);
		VuoBlackmagicVideoMode *modes = VuoListGetData_VuoBlackmagicVideoMode(modeList);
		items = videoModeComboBox->count();
		for (int i = 0; i < items; ++i)
		{
			bool found = false;
			for (int j = 0; j < modeCount; ++j)
				if (modes[j] == videoModeComboBox->itemData(i))
				{
					found = true;
					break;
				}

			videoModeComboBox->setItemEnabled(i, found);
		}
	}
}

void VuoInputEditorBlackmagicOutputDevice::emitValueChanged(int index)
{
	updateConnectionAndModeStatus();
	emit valueChanged(getAcceptedValue());
}

extern "C"
{

//@{
/**
 * Dummy definitions of functions not actually used by this input editor but needed for it to build.
 */
void vuoAddCompositionStateToThreadLocalStorage(const struct VuoCompositionState *compositionState) {}
void vuoRemoveCompositionStateFromThreadLocalStorage(void) {}
const void * vuoCopyCompositionStateFromThreadLocalStorage(void) { return NULL; }
uint64_t vuoGetCompositionUniqueIdentifier(const struct VuoCompositionState *compositionState) { return 0; }
//@}

}

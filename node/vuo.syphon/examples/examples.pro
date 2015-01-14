TEMPLATE = aux

EXAMPLE_SOURCES += \
	ReceiveImages.vuo \
	ReceiveImagesOnlyFromVuo.vuo \
	ReceiveImagesPreferablyFromVuo.vuo \
	SendImages.vuo
OTHER_FILES += $$EXAMPLE_SOURCES

include(../../../example.pri)

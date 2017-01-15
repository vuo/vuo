/**
 * @file
 * VuoInputEditorColor implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorColor.hh"

/**
 * Constructs a VuoInputEditorColor object.
 */
VuoInputEditor * VuoInputEditorColorFactory::newInputEditor()
{
	return new VuoInputEditorColor();
}

/**
 * Returns a VuoColor JSON object representing the specified @ref qtColor.
 */
VuoColor VuoInputEditorColor::vuoColorFromQColor(const QColor &qtColor)
{
	return VuoColor_makeWithRGBA(qtColor.redF(), qtColor.greenF(), qtColor.blueF(), qtColor.alphaF());
}

/**
 * Displays a color-picker dialog.
 */
json_object *VuoInputEditorColor::show(QPoint portLeftCenter, json_object *originalValue, json_object *details, map<QString, json_object *> portNamesAndValues)
{
	QColorDialog dialog;

	VuoColor colorVuo = VuoColor_makeFromJson(originalValue);
	QColor colorQt;
	colorQt.setRedF(colorVuo.r);
	colorQt.setGreenF(colorVuo.g);
	colorQt.setBlueF(colorVuo.b);
	colorQt.setAlphaF(colorVuo.a);
	dialog.setCurrentColor(colorQt);
	dialog.setOption(QColorDialog::ShowAlphaChannel);

	// Position the right center of the dialog at the left center of the port.
	// Estimate the dialog size, since QColorDialog doesn't report its actual size.
	QPoint dialogTopLeft = portLeftCenter - QPoint(250, 400/2) /*QPoint(dialog.childrenRect().width(), dialog.childrenRect().height()/2.)*/;
	dialog.move(dialogTopLeft);

	connect(&dialog, SIGNAL(currentColorChanged(const QColor &)), this, SLOT(currentColorChanged(const QColor &)));

	dialog.exec();

	if (dialog.result() == QDialog::Accepted)
		return VuoColor_getJson(vuoColorFromQColor(dialog.selectedColor()));

	return originalValue;
}

/**
 * Fired when the user changes the color in the color dialog.
 */
void VuoInputEditorColor::currentColorChanged(const QColor &color)
{
	json_object *valueAsJson = VuoColor_getJson(vuoColorFromQColor(color));
	emit valueChanged(valueAsJson);
	json_object_put(valueAsJson);
}

/**
 * @file
 * VuoInputEditorAnchor implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */
#include "VuoInputEditorAnchor.hh"
#include <stdio.h>
#include "VuoInputEditorIcon.hh"

extern "C"
{
#include "VuoHorizontalAlignment.h"
#include "VuoVerticalAlignment.h"
}

 /**
 * Renders an icon with the specified line segments.  @c length must be evenly divisable by 4 (for a minimum 2 points).
 */
QIcon* VuoInputEditorAnchor::renderIconWithLineSegments(const float* points, const unsigned int length)
{
	// needs a minimum of 2 points
	if (length % 4 != 0)
		return NULL;

	return VuoInputEditorIcon::renderIcon(
		^(QPainter &p)
		{
			QPainterPath path;

			for(int i = 0; i < length; i += 4)
			{
				path.moveTo(points[i+0], points[i+1]);
				path.lineTo(points[i+2], points[i+3]);
			}

			p.strokePath(path, QPen(Qt::black, 1));
		});
}

QIcon* VuoInputEditorAnchor::iconForAnchorIndex(const int anchor)
{
	const float left_top[8] = { 1.f, 1.f, 1.f, 15.f, 1.f, 1.f, 15.f, 1.f };
	const float center_top[4] = { 1.f, 1.f, 15.f, 1.f };
	const float right_top[8] = { 1.f, 1.f, 15.f, 1.f, 15.f, 1.f, 15.f, 15.f};

	const float left_center[4] = { 1.f, 1.f, 1.f, 15.f };
	const float center_center[8] = { 8.f, 1.f, 8.f, 15.f, 1.f, 8.f, 15.f, 8.f };
	const float right_center[4] = { 15.f, 1.f, 15.f, 15.f };

	const float left_bottom[8] = { 1.f, 15.f, 1.f, 1.f, 1.f, 15.f, 15.f, 15.f };
	const float center_bottom[4] = { 1.f, 15.f, 15.f, 15.f };
	const float right_bottom[8] = { 1.f, 15.f, 15.f, 15.f, 15.f, 15.f, 15.f, 1.f };

	switch(anchor)
	{
		// VuoHorizontalAlignment_Left,		VuoVerticalAlignment_Top
		case 0:
			return renderIconWithLineSegments( left_top, 8 );

		// VuoHorizontalAlignment_Center,	VuoVerticalAlignment_Top
		case 1:
			return renderIconWithLineSegments( center_top, 4 );

		// VuoHorizontalAlignment_Right,	VuoVerticalAlignment_Top
		case 2:
			return renderIconWithLineSegments( right_top, 8 );

		// VuoHorizontalAlignment_Left,		VuoVerticalAlignment_Center
		case 3:
			return renderIconWithLineSegments( left_center, 4 );

		// VuoHorizontalAlignment_Center,	VuoVerticalAlignment_Center
		case 4:
			return renderIconWithLineSegments( center_center, 8 );

		// VuoHorizontalAlignment_Right,	VuoVerticalAlignment_Center
		case 5:
			return renderIconWithLineSegments( right_center, 4 );

		// VuoHorizontalAlignment_Left,		VuoVerticalAlignment_Bottom
		case 6:
			return renderIconWithLineSegments( left_bottom, 8 );
		// VuoHorizontalAlignment_Center,	VuoVerticalAlignment_Bottom
		case 7:
			return renderIconWithLineSegments( center_bottom, 8 );

		// VuoHorizontalAlignment_Right,	VuoVerticalAlignment_Bottom
		case 8:
			return renderIconWithLineSegments( right_bottom, 8 );

		default:
			return renderIconWithLineSegments( left_top, 8 );
	}
}

/**
 * Constructs a VuoInputEditorAnchor object.
 */
VuoInputEditor * VuoInputEditorAnchorFactory::newInputEditor()
{
	return new VuoInputEditorAnchor();
}

int VuoInputEditorAnchor::VuoAnchorToIndex(const VuoAnchor anchor)
{
	int r = anchor.verticalAlignment == VuoVerticalAlignment_Top ? 0 : (anchor.verticalAlignment == VuoVerticalAlignment_Center ? 1 : 2);
	int c = anchor.horizontalAlignment == VuoHorizontalAlignment_Left ? 0 : (anchor.horizontalAlignment == VuoHorizontalAlignment_Center ? 1 : 2);
	return r * 3 + c;
}

/**
 * Prepare an Anchor dialog.
 */
void VuoInputEditorAnchor::setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details)
{
	VuoAnchor curAnchor = originalValue == NULL ? VuoAnchor_make(VuoHorizontalAlignment_Center, VuoVerticalAlignment_Center) : VuoAnchor_makeFromJson(originalValue);
	currentValue = VuoAnchor_getJson(curAnchor);
	int curIndex = VuoAnchorToIndex(curAnchor);

	// layout dialog
	QGridLayout* layout = new QGridLayout;
	layout->setContentsMargins(4, 4, 12, 4);
	layout->setSpacing(0);

	QSettings* settings = new QSettings();
	bool isDark = settings->value("darkInterface").toBool();

	// left top	 		center top	 		right top
	// left center	 	center center	 	right center
	// left bottom	 	center bottom		right center

	signalMapper = new QSignalMapper(this);

	for(int r = 0; r < 3; r++)
	{
		for(int c = 0; c < 3; c++)
		{
			int index = r * 3 + c;

			QIcon* icon = iconForAnchorIndex(index);
			matrix[index] = new QPushButton(*icon, "");
			delete icon;
			matrix[index]->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
			matrix[index]->setCheckable(true);
			matrix[index]->setChecked(false);
			matrix[index]->setAutoExclusive(true);
			matrix[index]->setFocusPolicy(Qt::NoFocus);
			matrix[index]->setDown(false);
			matrix[index]->setFlat(true);
			matrix[index]->setContentsMargins(0,0,0,0);
			matrix[index]->setMaximumWidth(24);
			matrix[index]->setMaximumHeight(24);
			matrix[index]->setMinimumWidth(24);
			matrix[index]->setMinimumHeight(24);
			if(isDark)
				matrix[index]->setStyleSheet( "QPushButton:checked { background:#8B8C8C; border:none; outline:none; }" );
			else
				matrix[index]->setStyleSheet( "QPushButton:checked { background:#909090; border:none; outline:none; }" );
			connect(matrix[index], SIGNAL(released()), signalMapper, SLOT(map()));
			signalMapper->setMapping(matrix[index], index) ;
			layout->addWidget(matrix[index], r, c);
		}
	}

	matrix[curIndex]->setChecked(true);

	connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(onSetAnchor(int)));

	dialog.setFocusPolicy(Qt::NoFocus);
	dialog.setLayout(layout);
}

void VuoInputEditorAnchor::onSetAnchor(int anchor)
{
	matrix[anchor]->setChecked(true);
	currentValue = VuoAnchor_getJson(ANCHOR_MAP[anchor]);
	char* sum = VuoAnchor_getSummary(ANCHOR_MAP[anchor]);
	free(sum);
	emit valueChanged(currentValue);
}

/**
 * Returns the value currently set in the dialog's widgets.
 */
json_object* VuoInputEditorAnchor::getAcceptedValue()
{
	return currentValue;
}

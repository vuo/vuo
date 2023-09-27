/**
 * @file
 * RunImageFilter-Qt main implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <Vuo/VuoMacOSSDKWorkaround.h>
#include <QtWidgets/QApplication>

#include "GLWidget.hh"

int main( int argc, char* argv[] )
{
	QApplication application( argc, argv );

	GLWidget widget;
	widget.setWindowTitle("RunImageFilter");
	widget.setFixedSize(640, 480);
	widget.show();
	return application.exec();
}

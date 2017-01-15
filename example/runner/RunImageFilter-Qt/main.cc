/**
 * @file
 * RunImageFilter-Qt main implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <QtWidgets/QApplication>
#include <QtOpenGL/QGLFormat>

#include "GLWidget.hh"

int main( int argc, char* argv[] )
{
	QApplication application( argc, argv );

	GLWidget widget;
	widget.setFixedSize(640, 480);
	widget.show();
	return application.exec();
}

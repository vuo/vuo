/**
 * @file
 * GLWidget interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include <Vuo/VuoMacOSSDKWorkaround.h>
#include <QtWidgets/QOpenGLWidget>

#include <Vuo/Vuo.h>

class GLWidget : public QOpenGLWidget
{
	Q_OBJECT
public:
	GLWidget(QWidget *parent = 0);
	~GLWidget();

protected:
	virtual void initializeGL();
	virtual void resizeGL(int w, int h);
	virtual void paintGL();

	virtual void keyPressEvent(QKeyEvent *e);

private:
	VuoRunner *runner;
	GLuint vertexArray;
	GLuint quadPositionBuffer;
	GLuint quadElementBuffer;
	GLuint inputTexture;
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint program;
	VuoRunner::Port *inputImagePort;
	VuoRunner::Port *timePort;
	bool isFirstEvent;
	int elapsedMilliseconds;
	GLint positionAttribute;
	GLint textureUniform;
	CGLContextObj cgl_ctx;
};

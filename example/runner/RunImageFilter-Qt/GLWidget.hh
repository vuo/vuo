/**
 * @file
 * GLWidget interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QtOpenGL/QGLBuffer>
#include <QtOpenGL/QGLShaderProgram>
#include <QtOpenGL/QGLWidget>

#include <Vuo/Vuo.h>

class GLWidget : public QGLWidget
{
	Q_OBJECT
public:
	GLWidget(QWidget *parent = 0);

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
	GLint positionAttribute;
	GLint textureUniform;
};

#endif // GLWIDGET_H

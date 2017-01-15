/**
 * @file
 * GLWidget implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "GLWidget.hh"

#include <QtCore/QtCore>
#include <QtGui/QtGui>

#import <OpenGL/gl.h>
//#import <OpenGL/gl3.h>
/// @todo After we drop 10.6 support, switch back to gl3 and remove the below 4 lines.  See also r15430 for shader changes.
#include <OpenGL/glext.h>
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE

#include <OpenGL/CGLCurrent.h>

#include <Vuo/Vuo.h>


#define GLSL_STRING(version,source) "#version " #version "\n" #source

static const char * vertexShaderSource = GLSL_STRING(120,
	attribute vec2 position;
	varying vec2 texcoord;

	void main()
	{
		gl_Position = vec4(position, 0.0, 1.0);
		texcoord = position * vec2(0.5) + vec2(0.5);
	}
);

static const char * fragmentShaderSource = GLSL_STRING(120,
	uniform sampler2D texture;
	varying vec2 texcoord;

	void main()
	{
		gl_FragColor = texture2D(texture, texcoord);
	}
);

static GLuint compileShader(GLenum type, const char * source)
{
	GLint length = strlen(source);
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, (const GLchar**)&source, &length);
	glCompileShader(shader);
	return shader;
}

static const GLfloat quadPositions[] = { -1, -1, 1, -1, -1, 1, 1, 1 };
static const GLushort quadElements[] = { 0, 1, 2, 3 };


GLWidget::GLWidget(QWidget* parent)
	: QGLWidget(parent)
{
	runner = NULL;
	inputImagePort = NULL;
}

void GLWidget::initializeGL()
{
	// Share the GL Context with the Vuo Composition
	VuoGlContext_setGlobalRootContext((void *)CGLGetCurrentContext());

	// Compile, link, and run the composition
	runner = VuoCompiler::newCurrentProcessRunnerFromCompositionFile((QCoreApplication::applicationDirPath() + "/../Resources/RippleImage.vuo").toUtf8().constData());
	runner->start();

	// Upload a quad, for rendering the texture later on
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);

	glGenBuffers(1, &quadPositionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, quadPositionBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadPositions), quadPositions, GL_STATIC_DRAW);

	glGenBuffers(1, &quadElementBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadElementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadElements), quadElements, GL_STATIC_DRAW);

	// Load an image from disk into a GL Texture
	QPixmap image(QCoreApplication::applicationDirPath() + "/../Resources/OttoOperatesTheRoller.jpg");
	GLenum internalformat = GL_RGBA;
	inputTexture = bindTexture(image, GL_TEXTURE_2D, internalformat);
	// Update the texture parameters to match VuoImage_make()'s requirements
	glBindTexture(GL_TEXTURE_2D, inputTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Prepare a shader for displaying the GL Texture onscreen
	vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
	fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

	program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	positionAttribute = glGetAttribLocation(program, "position");
	textureUniform = glGetUniformLocation(program, "texture");

	// Pass the GL Texture to the Vuo Composition
	inputImagePort = runner->getPublishedInputPortWithName("inputImage");
	VuoImage t = VuoImage_make(inputTexture, internalformat, image.width(), image.height());
	VuoRetain(t);
	json_object *o = VuoImage_getJson(t);
	runner->setPublishedInputPortValue(inputImagePort, o);
	json_object_put(o);

	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(updateGL()));
	timer->start((int)(1000./60.));  // approximately 60 fps
}

void GLWidget::resizeGL(int w, int h)
{
	// Set the viewport to window dimensions
	glViewport( 0, 0, w, qMax( h, 1 ) );
}

void GLWidget::paintGL()
{
	// Execute the Vuo Composition
	runner->firePublishedInputPortEvent(inputImagePort);
	runner->waitForAnyPublishedOutputPortEvent();

	// Retrieve the output GL Texture from the Vuo Composition
	VuoRunner::Port *outputImagePort = runner->getPublishedOutputPortWithName("outputImage");
	json_object *o = runner->getPublishedOutputPortValue(outputImagePort);
	VuoImage outputImage = VuoImage_makeFromJson(o);
	json_object_put(o);
	VuoRetain(outputImage);

	// Display the GL Texture onscreen
	glUseProgram(program);
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, outputImage->glTextureName);
		glUniform1i(textureUniform, 0);

		glBindBuffer(GL_ARRAY_BUFFER, quadPositionBuffer);
		glVertexAttribPointer(positionAttribute, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*2, (void*)0);
		glEnableVertexAttribArray(positionAttribute);
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadElementBuffer);
			glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (void*)0);
		}
		glDisableVertexAttribArray(positionAttribute);
	}
	glUseProgram(0);

	VuoRelease(outputImage);
}

void GLWidget::keyPressEvent(QKeyEvent* e)
{
	switch ( e->key() )
	{
		case Qt::Key_Escape:
			QCoreApplication::instance()->quit();
			break;

		default:
			QGLWidget::keyPressEvent( e );
	}
}

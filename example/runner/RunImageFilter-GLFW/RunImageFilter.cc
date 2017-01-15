/**
 * @file
 * RunImageFilter implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

//#define GLFW_INCLUDE_GL3
#define GLFW_NO_GLU
#include <GL/glfw.h>

#import <OpenGL/CGLMacro.h>
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

static GLuint compileShader(CGLContextObj cgl_ctx, GLenum type, const char * source)
{
	GLint length = strlen(source);
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, (const GLchar**)&source, &length);
	glCompileShader(shader);
	return shader;
}

static const GLfloat quadPositions[] = { -1, -1, 1, -1, -1, 1, 1, 1 };
static const GLushort quadElements[] = { 0, 1, 2, 3 };


VuoRunner * runner;
GLuint vertexArray;
GLuint quadPositionBuffer;
GLuint quadElementBuffer;
GLuint inputTexture;
GLuint vertexShader;
GLuint fragmentShader;
GLuint program;
int windowWillClose(void);

int main(void)
{
	// Open a window with a GL Context
	glfwInit();
	glfwOpenWindowHint(GLFW_WINDOW_NO_RESIZE, GL_TRUE);
	// Disable the deprecated fixed-function pipeline.
	//glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
	//glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 2);
	//glfwOpenWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	//glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	glfwOpenWindow(640,480,8,8,8,8,8,0,GLFW_WINDOW);
	glfwSetWindowCloseCallback(windowWillClose);

	CGLContextObj cgl_ctx = CGLGetCurrentContext();

	// Share the GL Context with the Vuo Composition
	VuoGlContext_setGlobalRootContext((void *)cgl_ctx);

	// Compile, link, and run the composition
	runner = VuoCompiler::newCurrentProcessRunnerFromCompositionFile(EXAMPLE_PATH "/RippleImage.vuo");
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
	glGenTextures(1,&inputTexture);
	glBindTexture(GL_TEXTURE_2D, inputTexture);
	GLFWimage glfwi;
	glfwReadImage(EXAMPLE_PATH "/OttoOperatesTheRoller.tga",&glfwi,GLFW_NO_RESCALE_BIT);
	glfwLoadTextureImage2D(&glfwi,0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

	// Prepare a shader for displaying the GL Texture onscreen
	vertexShader = compileShader(cgl_ctx, GL_VERTEX_SHADER, vertexShaderSource);
	fragmentShader = compileShader(cgl_ctx, GL_FRAGMENT_SHADER, fragmentShaderSource);

	program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	GLint positionAttribute = glGetAttribLocation(program, "position");
	GLint textureUniform = glGetUniformLocation(program, "texture");

	// Pass the GL Texture to the Vuo Composition
	VuoRunner::Port * inputImagePort = runner->getPublishedInputPortWithName("inputImage");
	VuoImage t = VuoImage_make(inputTexture, glfwi.Format, glfwi.Width, glfwi.Height);
	VuoRetain(t);
	json_object *o = VuoImage_getJson(t);
	runner->setPublishedInputPortValue(inputImagePort, o);
	json_object_put(o);

	while(glfwGetWindowParam(GLFW_OPENED))
	{
		// Execute the Vuo Composition
		runner->firePublishedInputPortEvent(inputImagePort);
		runner->drainMainDispatchQueue();
		runner->waitForAnyPublishedOutputPortEvent();

		// Retrieve the output GL Texture from the Vuo Composition
		VuoRunner::Port * outputImagePort = runner->getPublishedOutputPortWithName("outputImage");
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

		glfwSwapBuffers();

		VuoRelease(outputImage);
	}

	glfwTerminate();

	return 0;
}

int windowWillClose(void)
{
	runner->stop();
	delete runner;

	CGLContextObj cgl_ctx = CGLGetCurrentContext();

	glDetachShader(program, vertexShader);
	glDetachShader(program, fragmentShader);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	glDeleteProgram(program);

	glDeleteTextures(1, &inputTexture);

	glDeleteBuffers(1, &quadElementBuffer);
	glDeleteBuffers(1, &quadPositionBuffer);
	glDeleteVertexArrays(1, &vertexArray);

	return true;
}

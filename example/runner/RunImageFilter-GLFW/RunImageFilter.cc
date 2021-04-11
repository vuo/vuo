/**
 * @file
 * Demonstrates using Vuo's C++ API to compile and run an image filter composition and display its output in a window using GLFW.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

// Choose one:
// #define RUNIMAGEFILTER_PROCESS_CURRENT   // Run the composition in the same process as the host.
// #define RUNIMAGEFILTER_PROCESS_SEPARATE  // Run the composition in a separate process.

// Choose one:
// #define RUNIMAGEFILTER_USE_MAKEFROMJSON                 // Use VuoImage_makeFromJson() to get a VuoImage from the composition.
// #define RUNIMAGEFILTER_USE_MAKEFROMJSONWITHDIMENSIONS   // Use VuoImage_makeFromJsonWithDimensions() to get a VuoImage from the composition.
// #define RUNIMAGEFILTER_USE_INTERPROCESS_TO_FRAMEBUFFER  // Use VuoImage_resolveInterprocessJsonOntoFramebuffer() to render the image.
// #define RUNIMAGEFILTER_USE_INTERPROCESS_TO_TEXTURE      // Use VuoImage_resolveInterprocessJsonUsingClientTexture() to get a texture from the composition.

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

#if defined(RUNIMAGEFILTER_PROCESS_CURRENT) && (defined(RUNIMAGEFILTER_USE_INTERPROCESS_TO_FRAMEBUFFER) || defined(RUNIMAGEFILTER_USE_INTERPROCESS_TO_TEXTURE))
#error To use RUNIMAGEFILTER_USE_INTERPROCESS_TO_FRAMEBUFFER or RUNIMAGEFILTER_USE_INTERPROCESS_TO_TEXTURE, you must use RUNIMAGEFILTER_PROCESS_SEPARATE.
#endif


#include <Vuo/VuoMacOSSDKWorkaround.h>
#include <CoreFoundation/CoreFoundation.h>

#include <GLFW/glfw3.h>

#import <OpenGL/CGLMacro.h>
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE

#include <OpenGL/CGLCurrent.h>

#include <Vuo/Vuo.h>


#define GLSL_STRING(version,source) "#version " #version "\n" #source

#if defined(RUNIMAGEFILTER_USE_MAKEFROMJSON) || defined(RUNIMAGEFILTER_USE_MAKEFROMJSONWITHDIMENSIONS)
static const char *vertexShaderSource = GLSL_STRING(120,
	attribute vec2 position;
	varying vec2 texcoord;

	void main()
	{
		gl_Position = vec4(position, 0.0, 1.0);
		texcoord = position * vec2(0.5) + vec2(0.5);
	}
);
static const char *fragmentShaderSource = GLSL_STRING(120,
	uniform sampler2D texture;
	varying vec2 texcoord;

	void main()
	{
		gl_FragColor = texture2D(texture, texcoord);
	}
);
#elif defined(RUNIMAGEFILTER_USE_INTERPROCESS_TO_TEXTURE)
static const char *vertexShaderSource = GLSL_STRING(120,
	uniform vec2 size;
	attribute vec2 position;
	varying vec2 texcoord;

	void main()
	{
		gl_Position = vec4(position, 0.0, 1.0);
		texcoord = (position + 1.) * vec2(size.x, size.y) / 2.;
	}
);
static const char *fragmentShaderSource = GLSL_STRING(120,
	uniform sampler2DRect texture;
	varying vec2 texcoord;

	void main()
	{
		gl_FragColor = texture2DRect(texture, texcoord);
	}
);
#endif

#ifndef RUNIMAGEFILTER_USE_INTERPROCESS_TO_FRAMEBUFFER
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

GLuint vertexArray;
GLuint quadPositionBuffer;
GLuint quadElementBuffer;
GLuint vertexShader;
GLuint fragmentShader;
GLuint program;
#endif

GLuint inputTexture;
#ifdef RUNIMAGEFILTER_USE_INTERPROCESS_TO_TEXTURE
GLuint outputTexture;
#endif

VuoRunner *runner;
void windowWillClose(GLFWwindow *);
CGLContextObj cgl_ctx;

int main(void)
{
	// Open a window with a GL Context
	if (!glfwInit())
		return -1;
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "RunImageFilter", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}
	glfwSetWindowCloseCallback(window, windowWillClose);

	glfwMakeContextCurrent(window);
	cgl_ctx = CGLGetCurrentContext();

	// Share the GL Context with the Vuo Composition
	VuoGlContext_setGlobalRootContext((void *)cgl_ctx);

#ifndef RUNIMAGEFILTER_USE_INTERPROCESS_TO_FRAMEBUFFER
	// Upload a quad, for rendering the texture later on
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);

	glGenBuffers(1, &quadPositionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, quadPositionBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadPositions), quadPositions, GL_STATIC_DRAW);

	glGenBuffers(1, &quadElementBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadElementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadElements), quadElements, GL_STATIC_DRAW);
#endif

	// Load an image from disk into a GL Texture
	glGenTextures(1,&inputTexture);
	glBindTexture(GL_TEXTURE_2D, inputTexture);
	FILE *fp = fopen(SOURCE_DIR "/OttoOperatesTheRoller.tga", "rb");
	if (!fp)
	{
		fprintf(stderr, "error: couldn't open image: %s\n", strerror(errno));
		glfwTerminate();
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	size_t fpSize = ftell(fp);
	rewind(fp);
	unsigned char *buffer = (unsigned char *)malloc(fpSize);
	fread(buffer, 1, fpSize, fp);
	fclose(fp);
	if (buffer[0] != 0
		|| buffer[1] != 0
		|| buffer[2] != 2
		|| buffer[3] != 0
		|| buffer[16] != 24
		|| buffer[17] != 0)
	{
		fprintf(stderr, "error: unknown image format.\n");
		glfwTerminate();
		return -1;
	}
	int imageWidth = buffer[12] + (buffer[13]<<8);
	int imageHeight = buffer[14] + (buffer[15]<<8);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, GL_BGR, GL_UNSIGNED_BYTE, buffer + 18);
	free(buffer);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

#ifdef RUNIMAGEFILTER_PROCESS_SEPARATE
	// Ensure the texture is availble for interprocess texture transfer.
	glFlushRenderAPPLE();
#endif


#ifndef RUNIMAGEFILTER_USE_INTERPROCESS_TO_FRAMEBUFFER
	// Prepare a shader for displaying the GL Texture onscreen
	vertexShader = compileShader(cgl_ctx, GL_VERTEX_SHADER, vertexShaderSource);
	fragmentShader = compileShader(cgl_ctx, GL_FRAGMENT_SHADER, fragmentShaderSource);

	program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

#ifdef RUNIMAGEFILTER_USE_INTERPROCESS_TO_TEXTURE
	GLint sizeUniform = glGetUniformLocation(program, "size");
#endif
	GLint positionAttribute = glGetAttribLocation(program, "position");
	GLint textureUniform = glGetUniformLocation(program, "texture");
#endif

	// Compile, link, and run the composition
	VuoCompilerIssues issues;
#ifdef RUNIMAGEFILTER_PROCESS_CURRENT
	runner = VuoCompiler::newCurrentProcessRunnerFromCompositionFile(SOURCE_DIR "/RippleImage.vuo", &issues);
#else
	runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile(SOURCE_DIR "/RippleImage.vuo", &issues);
#endif
	runner->start();

	// Pass the GL Texture to the Vuo Composition
	VuoRunner::Port * inputImagePort = runner->getPublishedInputPortWithName("image");
	VuoImage inputImage = VuoImage_make(inputTexture, GL_RGB, imageWidth, imageHeight);
	VuoRetain(inputImage);
#ifdef RUNIMAGEFILTER_PROCESS_CURRENT
	json_object *imageJson = VuoImage_getJson(inputImage);
#else
	json_object *imageJson = VuoImage_getInterprocessJson(inputImage);
#endif
	map<VuoRunner::Port *, json_object *> imageMap;
	imageMap[inputImagePort] = imageJson;
	runner->setPublishedInputPortValues(imageMap);
	json_object_put(imageJson);

#ifdef RUNIMAGEFILTER_USE_INTERPROCESS_TO_TEXTURE
	// Create a texture to receive the composition's output image onto
	glGenTextures(1, &outputTexture);
#endif

	VuoRunner::Port * timePort = runner->getPublishedInputPortWithName("time");
	bool isFirstEvent = true;

	while (!glfwWindowShouldClose(window))
	{
		// Pass the current time to the Vuo Composition
		VuoReal time = glfwGetTime();
		json_object *timeJson = VuoReal_getJson(time);
		map<VuoRunner::Port *, json_object *> timeMap;
		timeMap[timePort] = timeJson;
		runner->setPublishedInputPortValues(timeMap);
		json_object_put(timeJson);

		// Only fire an event through published input ports whose values have changed
		set<VuoRunner::Port *> changedPorts;
		changedPorts.insert(timePort);
		if (isFirstEvent)
		{
			changedPorts.insert(inputImagePort);
			isFirstEvent = false;
		}

		// Execute the Vuo Composition
		runner->firePublishedInputPortEvent(changedPorts);
#ifdef RUNIMAGEFILTER_PROCESS_CURRENT
		runner->drainMainDispatchQueue();
		if (glfwWindowShouldClose(window))
			break;
#endif
		runner->waitForFiredPublishedInputPortEvent();

		// Retrieve the output GL Texture from the Vuo Composition
		VuoRunner::Port * outputImagePort = runner->getPublishedOutputPortWithName("outputImage");
		json_object *o = runner->getPublishedOutputPortValue(outputImagePort);

#if defined(RUNIMAGEFILTER_USE_MAKEFROMJSON)
		VuoImage outputImage = VuoImage_makeFromJson(o);
		VuoRetain(outputImage);
#elif defined(RUNIMAGEFILTER_USE_MAKEFROMJSONWITHDIMENSIONS)
		VuoImage outputImage = VuoImage_makeFromJsonWithDimensions(o, WINDOW_WIDTH, WINDOW_HEIGHT);
		VuoRetain(outputImage);
#elif defined(RUNIMAGEFILTER_USE_INTERPROCESS_TO_TEXTURE)
		IOSurfaceRef ioSurface;
		if (!VuoImage_resolveInterprocessJsonUsingClientTexture(o, outputTexture, WINDOW_WIDTH, WINDOW_HEIGHT, &ioSurface))
		{
			fprintf(stderr, "error: couldn't resolve the composition's output image.\n");
			json_object_put(o);
			glfwPollEvents();
			continue;
		}
#endif


		// Display the GL Texture onscreen
#ifdef RUNIMAGEFILTER_USE_INTERPROCESS_TO_FRAMEBUFFER
		if (!VuoImage_resolveInterprocessJsonOntoFramebuffer(o, cgl_ctx, false, true))
		{
			fprintf(stderr, "error: couldn't resolve the composition's output image.\n");
			json_object_put(o);
			glfwPollEvents();
			continue;
		}
#else
		glUseProgram(program);
		{
			glActiveTexture(GL_TEXTURE0);
			glUniform1i(textureUniform, 0);

#if defined(RUNIMAGEFILTER_USE_MAKEFROMJSON) || defined(RUNIMAGEFILTER_USE_MAKEFROMJSONWITHDIMENSIONS)
			glBindTexture(GL_TEXTURE_2D, outputImage->glTextureName);
#else
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, outputTexture);
			glUniform2f(sizeUniform, WINDOW_WIDTH, WINDOW_HEIGHT);
#endif

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
#endif

		json_object_put(o);

		glfwSwapBuffers(window);
		glfwPollEvents();

#if defined(RUNIMAGEFILTER_USE_MAKEFROMJSON) || defined(RUNIMAGEFILTER_USE_MAKEFROMJSONWITHDIMENSIONS)
		VuoRelease(outputImage);
#endif

#ifdef RUNIMAGEFILTER_USE_INTERPROCESS_TO_TEXTURE
		VuoIoSurfacePool_signal(ioSurface);
		CFRelease(ioSurface);
#endif
	}

	glfwTerminate();

	return 0;
}

void windowWillClose(GLFWwindow *)
{
	runner->stop();
	delete runner;

#ifndef RUNIMAGEFILTER_USE_INTERPROCESS_TO_FRAMEBUFFER
	glDetachShader(program, vertexShader);
	glDetachShader(program, fragmentShader);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	glDeleteProgram(program);
	glDeleteBuffers(1, &quadElementBuffer);
	glDeleteBuffers(1, &quadPositionBuffer);
	glDeleteVertexArrays(1, &vertexArray);
#endif

	glDeleteTextures(1, &inputTexture);
#ifdef RUNIMAGEFILTER_USE_INTERPROCESS_TO_TEXTURE
	glDeleteTextures(1, &outputTexture);
#endif
}

/**
 * @file
 * TestShaderFile interface and implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <sysexits.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Winvalid-constexpr"
#include <QtTest/QtTest>
#pragma clang diagnostic pop

#include <Vuo/Vuo.h>

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(VuoFileUtilities::File);
Q_DECLARE_METATYPE(VuoShaderFile *);
Q_DECLARE_METATYPE(VuoShaderIssues *);

/**
 * Tests the compiling and linking of compositions.
 */
class TestShaderFile : public QObject
{
	Q_OBJECT

private slots:

	void testShaderFile_data()
	{
		QTest::addColumn<VuoShaderFile *>("shaderFile");
		QTest::addColumn<QString>("imageTarget");
		QTest::addColumn<VuoFileUtilities::File>("source");
		QTest::addColumn<int>("expectedType");

		// Templates
		{
			VuoShaderFile::Type t = VuoShaderFile::GLSLImageFilter;
			QTest::newRow("GLSLImageFilter 2D")        << new VuoShaderFile(t) << "2D"        << VuoShaderFile::getTemplate(t) << (int)t;
			QTest::newRow("GLSLImageFilter Rectangle") << new VuoShaderFile(t) << "Rectangle" << VuoShaderFile::getTemplate(t) << (int)t;

			t = VuoShaderFile::GLSLImageGenerator;
			QTest::newRow("GLSLImageGenerator")        << new VuoShaderFile(t) << ""          << VuoShaderFile::getTemplate(t) << (int)t;

			t = VuoShaderFile::GLSLImageTransition;
			QTest::newRow("GLSLImageTransition")       << new VuoShaderFile(t) << ""          << VuoShaderFile::getTemplate(t) << (int)t;

			t = VuoShaderFile::GLSLObjectRenderer;
			QTest::newRow("GLSLObjectRenderer")        << new VuoShaderFile(t) << ""          << VuoShaderFile::getTemplate(t) << (int)t;

//			t = VuoShaderFile::GLSLObjectFilter;
//			QTest::newRow("GLSLObjectFilter")          << new VuoShaderFile(t) << ""          << VuoShaderFile::getTemplate(t) << (int)t;
		}

		// Test shaders folder
		map<string, VuoFileUtilities::File *> shaders;
		{
			auto shaderFiles = VuoFileUtilities::findAllFilesInDirectory("shaders", set<string>(), true);
			for (auto shaderFile : shaderFiles)
			{
				string ext = shaderFile->extension();
				if (VuoFileUtilities::isIsfSourceExtension(ext))
					// Add to a set without an extension, so if the shader consists of multiple files, we only test it once.
					shaders[shaderFile->dir() + "/" + shaderFile->basename()] = shaderFile;
			}
		}
		for (auto i : shaders)
		{
			string name = i.second->basename();

			/// @todo Implement support for ISF multipass, which these shaders require.
			if (name == "VIDVOX/ISF files/Auto Levels"
			 || name == "VIDVOX/ISF files/Bloom"
			 || name == "VIDVOX/ISF files/Chroma Desaturation Mask"
			 || name == "VIDVOX/ISF files/Chroma Mask"
			 || name == "VIDVOX/ISF files/City Lights"
			 || name == "VIDVOX/ISF files/Color Replacement"
			 || name == "VIDVOX/ISF files/Dilate"
			 || name == "VIDVOX/ISF files/Dilate-Fast"
			 || name == "VIDVOX/ISF files/Edge Blur"
			 || name == "VIDVOX/ISF files/Erode"
			 || name == "VIDVOX/ISF files/Erode-Fast"
			 || name == "VIDVOX/ISF files/FFT Spectrogram"
			 || name == "VIDVOX/ISF files/Fast Blur"
			 || name == "VIDVOX/ISF files/Fast Mosh"
			 || name == "VIDVOX/ISF files/FastMosh"
			 || name == "VIDVOX/ISF files/Freeze Frame"
			 || name == "VIDVOX/ISF files/Ghosting"
			 || name == "VIDVOX/ISF files/Gloom"
			 || name == "VIDVOX/ISF files/Glow"
			 || name == "VIDVOX/ISF files/Glow-Fast"
			 || name == "VIDVOX/ISF files/HorizVertHold"
			 || name == "VIDVOX/ISF files/Interlace"
			 || name == "VIDVOX/ISF files/Lens Flare"
			 || name == "VIDVOX/ISF files/Micro Buffer"
			 || name == "VIDVOX/ISF files/Micro Buffer RGB"
			 || name == "VIDVOX/ISF files/Multi Pass Gaussian Blur"
			 || name == "VIDVOX/ISF files/MultiFrame 2x2"
			 || name == "VIDVOX/ISF files/MultiFrame 3x3"
			 || name == "VIDVOX/ISF files/Night Vision"
			 || name == "VIDVOX/ISF files/Optical Flow Distort"
			 || name == "VIDVOX/ISF files/Optical Flow Generator"
			 || name == "VIDVOX/ISF files/RGB Strobe"
			 || name == "VIDVOX/ISF files/RGB Trails 3.0"
			 || name == "VIDVOX/ISF files/Shockwave Pulse"
			 || name == "VIDVOX/ISF files/Slit Scan"
			 || name == "VIDVOX/ISF files/Soft Blur"
			 || name == "VIDVOX/ISF files/Strobe"
			 || name == "VIDVOX/ISF files/Time Glitch RGB"
			 || name == "VIDVOX/ISF files/Trail Mask"
			 || name == "VIDVOX/ISF files/VVMotionBlur 3.0"
			 || name == "VIDVOX/ISF tests+tutorials/Test-MultiPassRendering"
			 || name == "VIDVOX/ISF tests+tutorials/Test-PersistentBuffer"
			 || name == "VIDVOX/ISF tests+tutorials/Test-PersistentBufferDifferingSizes"
			 || name == "VIDVOX/ISF tests+tutorials/Test-TempBufferDifferingSizes")
			{
				QVERIFY_EXCEPTION_THROWN(VuoShaderFile(*i.second), VuoException);
				continue;
			}

			/// @todo Implement support for ISF image-importing, which these shaders require.
			if (name == "VIDVOX/ISF tests+tutorials/Test-ImportedImage")
			{
				QVERIFY_EXCEPTION_THROWN(VuoShaderFile(*i.second), VuoException);
				continue;
			}

			QTest::newRow(i.first.c_str()) << new VuoShaderFile(*i.second) << "" << *i.second << -1;
		}
	}
	void testShaderFile()
	{
		QFETCH(VuoShaderFile *, shaderFile);
		QFETCH(QString, imageTarget);
		QFETCH(VuoFileUtilities::File, source);
		QFETCH(int, expectedType);

		QVERIFY(shaderFile);

//		shaderFile->dump();

		// Did we correctly identify the loaded template?
		if (expectedType != -1)
			QCOMPARE((int)shaderFile->type(), expectedType);

		// When we save the shader, is it identical to the shader we loaded?
		// Don't test the VIDVOX shaders, since their ISF headers aren't necessarily in JSON_C_TO_STRING_PRETTY format.
		if (!VuoStringUtilities::beginsWith(source.path(), "shaders/VIDVOX/ISF files/")
		 && !VuoStringUtilities::beginsWith(source.path(), "shaders/VIDVOX/ISF tests+tutorials/")
		 && source.path() != "shaders/CRLF.fs")
		{
			string actualShaderPath = VuoFileUtilities::makeTmpFile("TestShaderFile", source.extension());
			shaderFile->save(actualShaderPath);

			QFileInfo actualShaderInfo(QString::fromStdString(actualShaderPath));
			string actualShaderBasename = (actualShaderInfo.path() + "/" + actualShaderInfo.baseName()).toStdString();
			QStringList extensions;
			extensions << "vs" << "gs" << "fs";
			foreach (QString extension, extensions)
			{
				string actualShaderFilename = actualShaderBasename + "." + extension.toStdString();
				if (!VuoFileUtilities::fileExists(actualShaderFilename))
					continue;

				string inputFilename = source.dir() + "/" + source.basename() + "." + extension.toStdString();

				int ret = system( ("/usr/bin/diff -u \"" + inputFilename + "\" \"" + actualShaderFilename + "\"").c_str() );
				QCOMPARE(ret, EX_OK);

				remove(actualShaderFilename.c_str());
			}
		}

		// Does the shader compile and link without any errors or warnings?
		{
			VuoShader shader = VuoShader_makeFromFile(shaderFile);
			VuoLocal(shader);

			// Provide an input image for Image Filters.
			if (shaderFile->type() == VuoShaderFile::GLSLImageFilter)
			{
				VuoImage image = VuoImage_makeColorImage(VuoColor_makeWithRGBA(.4,.5,.6,1.), 32, 32);
				if (imageTarget == "Rectangle")
					image = VuoImage_makeGlTextureRectangleCopy(image);
				VuoShader_setUniform_VuoImage(shader, "inputImage", image);
			}
			else if (shaderFile->type() == VuoShaderFile::GLSLImageTransition)
			{
				VuoImage image = VuoImage_makeColorImage(VuoColor_makeWithRGBA(.4,.5,.6,1.), 32, 32);
				if (imageTarget == "Rectangle")
					image = VuoImage_makeGlTextureRectangleCopy(image);
				VuoShader_setUniform_VuoImage(shader, "startImage", image);
				VuoShader_setUniform_VuoImage(shader, "endImage", image);
			}

			// Provide input image(s) for other image ports, if any.
			for (auto p : shaderFile->inputPorts())
			{
				if (p.vuoTypeName != "VuoImage")
					continue;
				if (p.key == "inputImage")
					continue;

				VuoImage image = VuoImage_makeColorImage(VuoColor_makeWithRGBA(.4,.5,.6,1.), 32, 32);
				if (imageTarget == "Rectangle")
					image = VuoImage_makeGlTextureRectangleCopy(image);
				VuoShader_setUniform_VuoImage(shader, p.key.c_str(), image);
			}

			// Compile and link the shader.
			VuoGlContext_perform(^(CGLContextObj cgl_ctx){
				VuoShaderIssues issues;
				bool ret = VuoShader_upload(shader, VuoMesh_IndividualTriangles, cgl_ctx, &issues);
				issues.dump();
				QVERIFY2(issues.issues().size() == 0, "Shader has issues.");
				QVERIFY2(ret, "Shader upload failed.");

				VuoGlProgram program;
				ret = VuoShader_activate(shader, VuoMesh_IndividualTriangles, cgl_ctx, &program);
				QVERIFY2(ret, "Shader activation failed.");

				VuoShader_deactivate(shader, VuoMesh_IndividualTriangles, cgl_ctx);
			});
		}

		delete shaderFile;
	}

	void testShaderFileIssues_data()
	{
		QTest::addColumn<QString>("file");
		QTest::addColumn<VuoShaderIssues *>("expectedIssues");
		QTest::addColumn<bool>("expectedOK");

		{
			VuoShaderIssues *issues = new VuoShaderIssues();
			issues->addIssue(VuoShaderFile::Program, VuoShaderIssues::NoLine, "No definition of main in fragment shader");
			QTest::newRow("NoFile") << "NoFile.fs" << issues << false;
		}

		{
			VuoShaderIssues *issues = new VuoShaderIssues();
			issues->addIssue(VuoShaderFile::Program, VuoShaderIssues::NoLine, "No definition of main in fragment shader");
			QTest::newRow("Empty") << "Empty.fs" << issues << false;
		}

		{
			VuoShaderIssues *issues = new VuoShaderIssues();
			issues->addIssue(VuoShaderFile::Fragment, 1, "syntax error: '!'");
			QTest::newRow("SyntaxError") << "SyntaxError.fs" << issues << false;
		}

		{
			VuoShaderIssues *issues = new VuoShaderIssues();
			QTest::newRow("NoIssues") << "NoIssues.fs" << issues << true;
		}

		{
			VuoShaderIssues *issues = new VuoShaderIssues();
			issues->addIssue(VuoShaderFile::Fragment, 1, "extension 'GL_ARB_shading_language_packing' is not supported");
			QTest::newRow("Warning") << "Warning.fs" << issues << true;
		}
	}
	void testShaderFileIssues()
	{
		QFETCH(QString, file);
		QFETCH(VuoShaderIssues *, expectedIssues);
		QFETCH(bool, expectedOK);

		VuoShaderFile *shaderFile = new VuoShaderFile(VuoFileUtilities::File("shaders-issues", file.toStdString()));
		QVERIFY(shaderFile);

		// Ensure the emitted compile/link issues match expectations.
		{
			VuoShader shader = VuoShader_makeFromFile(shaderFile);
			VuoLocal(shader);

			VuoGlContext_perform(^(CGLContextObj cgl_ctx){
				VuoShaderIssues actualIssues;
				bool ok = VuoShader_upload(shader, VuoMesh_IndividualTriangles, cgl_ctx, &actualIssues);

//				actualIssues.dump();
				vector<VuoShaderIssues::Issue> expected = expectedIssues->issues();
				vector<VuoShaderIssues::Issue> actual = actualIssues.issues();
				if (actual.size() != expected.size())
				{
					VUserLog("Expected issues:");
					expectedIssues->dump();
					VUserLog("Actual issues:");
					actualIssues.dump();
					QFAIL("Wrong number of issues found");
				}
				for (size_t i = 0; i < actual.size(); ++i)
					QCOMPARE(QString::fromStdString(actual[i].message), QString::fromStdString(expected[i].message));
				QCOMPARE(ok, expectedOK);
			});
		}

		delete shaderFile;
	}

	void testShowsTime_data()
	{
		QTest::addColumn<VuoShaderFile *>("shaderFile");
		QTest::addColumn<bool>("expectedShowsTime");

		QTest::newRow("GLSLImageFilter")    << new VuoShaderFile(VuoShaderFile::GLSLImageFilter)                   << false;
		QTest::newRow("GLSLImageGenerator") << new VuoShaderFile(VuoShaderFile::GLSLImageGenerator)                << false;
		QTest::newRow("GLSLObjectRenderer") << new VuoShaderFile(VuoShaderFile::GLSLObjectRenderer)                << false;
		QTest::newRow("GLSLObjectFilter")   << new VuoShaderFile(VuoShaderFile::GLSLObjectFilter)                  << false;
		QTest::newRow("UsesTime")           << new VuoShaderFile(VuoFileUtilities::File("shaders", "UsesTime.fs")) << true;
		QTest::newRow("UsesTime2")          << new VuoShaderFile(VuoFileUtilities::File("shaders", "UsesTime2.fs")) << true;
	}
	void testShowsTime()
	{
		QFETCH(VuoShaderFile *, shaderFile);
		QFETCH(bool, expectedShowsTime);

		QVERIFY(shaderFile);

		QCOMPARE(shaderFile->showsTime(), expectedShowsTime);

		delete shaderFile;
	}
};

QTEST_APPLESS_MAIN(TestShaderFile)
#include "TestShaderFile.moc"

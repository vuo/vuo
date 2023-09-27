/**
 * @file
 * TestVuoIsfModuleCompiler interface and implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <Vuo/Vuo.h>
#include "TestCompositionExecution.hh"

#include <OpenGL/gl.h>

/**
 * Tests the VuoIsfModuleCompiler class.
 */
class TestVuoIsfModuleCompiler : public TestCompositionExecution
{
	Q_OBJECT

private:

	class ModuleFileGroup
	{
	public:
		ModuleFileGroup(const string &sourcePath)
		{
			this->sourcePath = sourcePath;
		}

		~ModuleFileGroup(void)
		{
			VuoFileUtilities::deleteFile(getCompiledPath());
		}

		string getModuleKey(void)
		{
			return VuoCompiler::getModuleKeyForPath(sourcePath);
		}

		string getSourcePath(void)
		{
			return sourceDir.empty() ? sourcePath : sourceDir + "/" + sourcePath;
		}

		string getCompiledPath(void)
		{
			return getModuleDir() + "/" + getModuleKey() + ".vuonode";
		}

		string getCompositionDir(void)
		{
			string dir, file, ext;
			VuoFileUtilities::splitPath(moduleDir, dir, file, ext);
			return dir;
		}

		static void setSourceDir(const string &sourceDir)
		{
			ModuleFileGroup::sourceDir = sourceDir;
		}

	private:
		static string getModuleDir(void)
		{
			if (moduleDir.empty())
			{
				string testDir = VuoFileUtilities::makeTmpDir(testId);
				moduleDir = testDir + "/Modules";
				VuoFileUtilities::makeDir(moduleDir);
			}

			return moduleDir;
		}

		string sourcePath;
		static string testId;
		static string sourceDir;
		static string moduleDir;
	};

	class CompositionFileGroup
	{
	public:
		CompositionFileGroup(const string &sourcePath)
		{
			sourceFile = new VuoFileUtilities::File(sourceDir, sourcePath);
		}

		~CompositionFileGroup(void)
		{
			VuoFileUtilities::deleteFile(getCompiledPath());
			VuoFileUtilities::deleteFile(getExecutablePath());
			VuoFileUtilities::deleteFile(getDynamicLibraryPath());
			delete sourceFile;
		}

		string getCompiledPath(void)
		{
			return getDestinationDir() + "/" + sourceFile->basename() + ".bc";
		}

		string getExecutablePath(void)
		{
			return getDestinationDir() + "/" + sourceFile->basename();
		}

		string getDynamicLibraryPath(void)
		{
			return getDestinationDir() + "/" + sourceFile->basename() + ".dylib";
		}

		static void setSourceDir(const string &sourceDir)
		{
			CompositionFileGroup::sourceDir = sourceDir;
		}

		static void setDestinationDir(const string &destinationDir)
		{
			CompositionFileGroup::destinationDir = destinationDir;
		}

	private:
		static string getDestinationDir(void)
		{
			if (destinationDir.empty())
			{
				destinationDir = VuoFileUtilities::makeTmpDir(testId);
				VuoFileUtilities::makeDir(destinationDir);
			}

			return destinationDir;
		}

		VuoFileUtilities::File *sourceFile;
		static string testId;
		static string sourceDir;
		static string destinationDir;
	};

	void compileIsfToLlvmModule(ModuleFileGroup &m, Module *&llvmModule, VuoCompilerIssues *&issues)
	{
		VuoCompiler *compiler = new VuoCompiler(QDir::current().canonicalPath().toStdString() + "/composition");
		auto getType = [&compiler] (const string &moduleKey) { return compiler->getType(moduleKey); };

		VuoModuleCompiler *moduleCompiler = VuoModuleCompiler::newModuleCompiler("isf", m.getModuleKey(), m.getSourcePath(), VuoModuleCompilerSettings(), getType);
		QVERIFY(moduleCompiler);

		dispatch_queue_t llvmQueue = dispatch_queue_create("org.vuo.TestVuoIsfModuleCompiler.llvm", NULL);  // OK since VuoCompiler is used serially here
		issues = new VuoCompilerIssues();
		VuoModuleCompilerResults results = moduleCompiler->compile(llvmQueue, issues);
		llvmModule = results.module;

		delete moduleCompiler;
		delete compiler;
	}

	void loadLlvmModuleAsNodeClass(ModuleFileGroup &m, Module *llvmModule, VuoCompilerNodeClass *&nodeClass)
	{
		VuoCompilerModule *module = VuoCompilerModule::newModule(m.getModuleKey(), llvmModule, m.getCompiledPath(), VuoCompilerCompatibility::compatibilityWithAnySystem());
		QVERIFY(module);

		nodeClass = dynamic_cast<VuoCompilerNodeClass *>(module);
		QVERIFY(nodeClass);
	}

	void makePortsAndValues(VuoRunner *runner, const QStringList &inputPorts, const QStringList &inputValues, map<VuoRunner::Port *, json_object *> &portsAndValues)
	{
		for (int i = 0; i < inputPorts.size(); ++i)
		{
			VuoRunner::Port *port = runner->getPublishedInputPortWithName( inputPorts[i].toStdString() );
			QVERIFY(port);

			json_object *value = json_tokener_parse( inputValues[i].toStdString().c_str() );
			QVERIFY(value);

			portsAndValues[port] = value;
		}
	}

	void checkPixel(VuoImage image, int x, int y, float r, float g, float b, float a)
	{
		const unsigned char *pixels = VuoImage_getBuffer(image, GL_BGRA);
		QVERIFY(pixels);

		unsigned long pixelsWide = image->pixelsWide;
		unsigned long pixelsHigh = image->pixelsHigh;

		unsigned long row = pixelsHigh - x - 1;
		unsigned long col = y;
		QVERIFY(abs(pixels[row*pixelsWide*4 + col*4 + 2] - (unsigned char)(a*r*255)) < 2);
		QVERIFY(abs(pixels[row*pixelsWide*4 + col*4 + 1] - (unsigned char)(a*g*255)) < 2);
		QVERIFY(abs(pixels[row*pixelsWide*4 + col*4 + 0] - (unsigned char)(a*b*255)) < 2);
		QVERIFY(abs(pixels[row*pixelsWide*4 + col*4 + 3] - (unsigned char)(a  *255)) < 2);
	}

private slots:

	// TestShaderFile already tests for syntax errors. Here, just make sure they bubble up to VuoIsfModuleCompiler.
	void testParsingShader_data()
	{
		QTest::addColumn<QString>("sourcePath");
		QTest::addColumn<bool>("isValid");

		QTest::newRow("valid fragment shader") << "../TestShaderFile/shaders/AllMetadataAndInputTypes.fs" << true;
		QTest::newRow("syntax error in fragment shader") << "../TestShaderFile/shaders-issues/Empty.fs" << false;
	}
	void testParsingShader()
	{
		QFETCH(QString, sourcePath);
		QFETCH(bool, isValid);

		ModuleFileGroup::setSourceDir("");
		ModuleFileGroup m(sourcePath.toStdString());

		Module *llvmModule;
		VuoCompilerIssues *issues;
		compileIsfToLlvmModule(m, llvmModule, issues);

		QCOMPARE(llvmModule != nullptr, isValid);
		QCOMPARE(! issues->hasErrors(), isValid);

		delete issues;
		delete llvmModule;
	}

	void testNodeInterface_data()
	{
		QTest::addColumn<QString>("sourcePath");
		QTest::addColumn<QString>("expectedClassName");
		QTest::addColumn<QString>("expectedDefaultTitle");
		QTest::addColumn<QStringList>("expectedInputPortNames");
		QTest::addColumn<QStringList>("expectedInputPortDisplayNames");
		QTest::addColumn<QStringList>("expectedInputPortTypes");
		QTest::addColumn<QStringList>("expectedInputPortDefaults");
		QTest::addColumn<QStringList>("expectedOutputPortNames");
		QTest::addColumn<QStringList>("expectedOutputPortDisplayNames");
		QTest::addColumn<QStringList>("expectedOutputPortTypes");

		QStringList inputNames_Refresh("refresh");
		QStringList inputDisplayNames_Refresh("Refresh");
		QStringList inputTypes_Refresh("event");
		QStringList inputDefaults_Refresh("");

		QStringList inputNames_WidthHeight;
		QStringList inputDisplayNames_WidthHeight;
		QStringList inputTypes_WidthHeight;
		QStringList inputDefaults_WidthHeight;
		inputNames_WidthHeight << "vuoWidth" << "vuoHeight";
		inputDisplayNames_WidthHeight << "Width" << "Height";
		inputTypes_WidthHeight << "VuoInteger" << "VuoInteger";
		inputDefaults_WidthHeight << "640" << "480";

		QStringList outputNames_Image("outputImage");
		QStringList outputDisplayNames_Image("Output Image");
		QStringList outputTypes_Image("VuoImage");

		{
			QStringList inputNames = inputNames_Refresh;
			QStringList inputDisplayNames = inputDisplayNames_Refresh;
			QStringList inputTypes = inputTypes_Refresh;
			QStringList inputDefaults = inputDefaults_Refresh;
			inputNames << inputNames_WidthHeight;
			inputDisplayNames << inputDisplayNames_WidthHeight;
			inputTypes << inputTypes_WidthHeight;
			inputDefaults << inputDefaults_WidthHeight;
			QTest::newRow("image generator: no ports specified") << "vuo.test.imageGenerator.noPorts.fs"
																 << "vuo.test.imageGenerator.noPorts"
																 << "No Ports"
																 << inputNames << inputDisplayNames << inputTypes << inputDefaults
																 << outputNames_Image << outputDisplayNames_Image << outputTypes_Image;
		}
		{
			QStringList inputNames = inputNames_Refresh;
			QStringList inputDisplayNames = inputDisplayNames_Refresh;
			QStringList inputTypes = inputTypes_Refresh;
			QStringList inputDefaults = inputDefaults_Refresh;
			inputNames << "offset" << "colorList" << inputNames_WidthHeight;
			inputDisplayNames << "Offset" << "Color List" << inputDisplayNames_WidthHeight;
			inputTypes << "VuoPoint2d" << "VuoList_VuoColor" << inputTypes_WidthHeight;
			inputDefaults << "[0.5,0.5]" << "[[0.1,0.2,0.3,0.4],[0.5,0.6,0.7,0.8]]" << inputDefaults_WidthHeight;

			QTest::newRow("image generator: input ports specified") << "vuo.test.imageGenerator.inputPorts.fs"
																	<< "vuo.test.imageGenerator.inputPorts"
																	<< "Image Generator with Input Ports"
																	<< inputNames << inputDisplayNames << inputTypes << inputDefaults
																	<< outputNames_Image << outputDisplayNames_Image << outputTypes_Image;
		}
		{
			QStringList inputNames = inputNames_Refresh;
			QStringList inputDisplayNames = inputDisplayNames_Refresh;
			QStringList inputTypes = inputTypes_Refresh;
			QStringList inputDefaults = inputDefaults_Refresh;
			inputNames << "inputImage" << "invert";
			inputDisplayNames << "Image" << "Invert";
			inputTypes << "VuoImage" << "VuoBoolean";
			inputDefaults << "" << "true";

			QTest::newRow("image filter: input ports specified") << "vuo.test.imageFilter.inputPorts.fs"
																 << "vuo.test.imageFilter.inputPorts"
																 << "Image Filter with Input Ports"
																 << inputNames << inputDisplayNames << inputTypes << inputDefaults
																 << outputNames_Image << outputDisplayNames_Image << outputTypes_Image;
		}
		{
			QStringList inputNames = inputNames_Refresh;
			QStringList inputDisplayNames = inputDisplayNames_Refresh;
			QStringList inputTypes = inputTypes_Refresh;
			QStringList inputDefaults = inputDefaults_Refresh;
			inputNames << "curlAmount" << inputNames_WidthHeight;
			inputDisplayNames << "Curl Amount" << inputDisplayNames_WidthHeight;
			inputTypes << "VuoReal" << inputTypes_WidthHeight;
			inputDefaults << "0.5" << inputDefaults_WidthHeight;

			QStringList outputNames;
			QStringList outputDisplayNames;
			QStringList outputTypes;
			outputNames << "curledImage";
			outputDisplayNames << "The Curled Image";
			outputTypes << "VuoImage";

			QTest::newRow("image generator: input and output ports specified") << "vuo.test.imageGenerator.InputOutputPorts.fs"
																			   << "vuo.test.imageGenerator.inputOutputPorts"
																			   << "Image Filter with Input and Output Ports"
																			   << inputNames << inputDisplayNames << inputTypes << inputDefaults
																			   << outputNames << outputDisplayNames << outputTypes;
		}
		{
			QStringList inputNames = inputNames_Refresh;
			QStringList inputDisplayNames = inputDisplayNames_Refresh;
			QStringList inputTypes = inputTypes_Refresh;
			QStringList inputDefaults = inputDefaults_Refresh;
			inputNames << "image" << "inputImage" << "inputImage2";
			inputDisplayNames << "Less Important Image" << "Main Image" << "Another Image";
			inputTypes << "VuoImage" << "VuoImage" << "VuoImage";
			inputDefaults << "" << "" << "";

			QTest::newRow("image filter: multiple image input ports") << "vuo.test.imageFilter.multipleImageInputs.fs"
																	  << "vuo.test.imageFilter.multipleImageInputs"
																	  << "Multiple Image Inputs"
																	  << inputNames << inputDisplayNames << inputTypes << inputDefaults
																	  << outputNames_Image << outputDisplayNames_Image << outputTypes_Image;
		}
		{
			QStringList inputNames = inputNames_Refresh;
			QStringList inputDisplayNames = inputDisplayNames_Refresh;
			QStringList inputTypes = inputTypes_Refresh;
			QStringList inputDefaults = inputDefaults_Refresh;
			inputNames << "vuoTime" << inputNames_WidthHeight;
			inputDisplayNames << "Time" << inputDisplayNames_WidthHeight;
			inputTypes << "VuoReal" << inputTypes_WidthHeight;
			inputDefaults << "0.0" << inputDefaults_WidthHeight;

			QTest::newRow("image generator: time input port") << "vuo.test.imageGenerator.timeInput.fs"
															  << "vuo.test.imageGenerator.timeInput"
															  << "Time Input"
															  << inputNames << inputDisplayNames << inputTypes << inputDefaults
															  << outputNames_Image << outputDisplayNames_Image << outputTypes_Image;
		}
		{
			QStringList inputNames = inputNames_Refresh;
			QStringList inputDisplayNames = inputDisplayNames_Refresh;
			QStringList inputTypes = inputTypes_Refresh;
			QStringList inputDefaults = inputDefaults_Refresh;
			inputNames << inputNames_WidthHeight;
			inputDisplayNames << inputDisplayNames_WidthHeight;
			inputTypes << inputTypes_WidthHeight;
			inputDefaults << inputDefaults_WidthHeight;

			QTest::newRow("image generator: size input port") << "vuo.test.imageGenerator.sizeInput.fs"
															  << "vuo.test.imageGenerator.sizeInput"
															  << "Size Input"
															  << inputNames << inputDisplayNames << inputTypes << inputDefaults
															  << outputNames_Image << outputDisplayNames_Image << outputTypes_Image;
		}
		{
			QStringList inputNames = inputNames_Refresh;
			QStringList inputDisplayNames = inputDisplayNames_Refresh;
			QStringList inputTypes = inputTypes_Refresh;
			QStringList inputDefaults = inputDefaults_Refresh;
			inputNames << "inputImage" << inputNames_WidthHeight;
			inputDisplayNames << "Image" << inputDisplayNames_WidthHeight;
			inputTypes << "VuoImage" << inputTypes_WidthHeight;
			inputDefaults << "" << inputDefaults_WidthHeight;

			QTest::newRow("image filter: size input port") << "vuo.test.imageFilter.sizeInput.fs"
														   << "vuo.test.imageFilter.sizeInput"
														   << "Size Input"
														   << inputNames << inputDisplayNames << inputTypes << inputDefaults
														   << outputNames_Image << outputDisplayNames_Image << outputTypes_Image;
		}
		{
			QStringList inputNames = inputNames_Refresh;
			QStringList inputDisplayNames = inputDisplayNames_Refresh;
			QStringList inputTypes = inputTypes_Refresh;
			QStringList inputDefaults = inputDefaults_Refresh;
			inputNames << "vuoColorDepth" << inputNames_WidthHeight;
			inputDisplayNames << "Color Depth" << inputDisplayNames_WidthHeight;
			inputTypes << "VuoImageColorDepth" << inputTypes_WidthHeight;
			inputDefaults << "\"8bpc\"" << inputDefaults_WidthHeight;

			QTest::newRow("image generator: color depth input port") << "vuo.test.imageGenerator.colorDepthInput.fs"
																	 << "vuo.test.imageGenerator.colorDepthInput"
																	 << "Color Depth Input"
																	 << inputNames << inputDisplayNames << inputTypes << inputDefaults
																	 << outputNames_Image << outputDisplayNames_Image << outputTypes_Image;
		}
		{
			QStringList inputNames = inputNames_Refresh;
			QStringList inputDisplayNames = inputDisplayNames_Refresh;
			QStringList inputTypes = inputTypes_Refresh;
			QStringList inputDefaults = inputDefaults_Refresh;
			inputNames << "startImage" << "endImage" << "progress";
			inputDisplayNames << "Start Image" << "End Image" << "Progress";
			inputTypes << "VuoImage" << "VuoImage" << "VuoReal";
			inputDefaults << "" << "" << "";

			QTest::newRow("image transition: no extra ports specified") << "vuo.test.imageTransition.noExtraPorts.fs"
																		<< "vuo.test.imageTransition.noExtraPorts"
																		<< "No Extra Ports"
																		<< inputNames << inputDisplayNames << inputTypes << inputDefaults
																		<< outputNames_Image << outputDisplayNames_Image << outputTypes_Image;
		}
	}
	void testNodeInterface()
	{
		QFETCH(QString, sourcePath);
		QFETCH(QString, expectedClassName);
		QFETCH(QString, expectedDefaultTitle);
		QFETCH(QStringList, expectedInputPortNames);
		QFETCH(QStringList, expectedInputPortDisplayNames);
		QFETCH(QStringList, expectedInputPortTypes);
		QFETCH(QStringList, expectedInputPortDefaults);
		QFETCH(QStringList, expectedOutputPortNames);
		QFETCH(QStringList, expectedOutputPortDisplayNames);
		QFETCH(QStringList, expectedOutputPortTypes);

		ModuleFileGroup::setSourceDir("shader");
		ModuleFileGroup m(sourcePath.toStdString());

		Module *llvmModule;
		VuoCompilerIssues *issues;
		compileIsfToLlvmModule(m, llvmModule, issues);

		QVERIFY2(issues->isEmpty(), issues->getLongDescription(false).c_str());
		QVERIFY(!llvm::verifyModule(*llvmModule));

		VuoCompilerNodeClass *nodeClass;
		loadLlvmModuleAsNodeClass(m, llvmModule, nodeClass);
		QCOMPARE(QString::fromStdString(nodeClass->getBase()->getClassName()), expectedClassName);

		QCOMPARE(QString::fromStdString(nodeClass->getBase()->getClassName()), QString::fromStdString(m.getModuleKey()));
		QCOMPARE(QString::fromStdString(nodeClass->getBase()->getDefaultTitle()), expectedDefaultTitle);

		QStringList actualInputPortNames;
		QStringList actualInputPortDisplayNames;
		QStringList actualInputPortTypes;
		QStringList actualInputPortDefaults;
		for (VuoPortClass *portClass : nodeClass->getBase()->getInputPortClasses())
		{
			actualInputPortNames << QString::fromStdString( portClass->getName() );
			actualInputPortDisplayNames << QString::fromStdString( static_cast<VuoCompilerPortClass *>(portClass->getCompiler())->getDisplayName() );

			VuoType *actualType = static_cast<VuoCompilerPortClass *>(portClass->getCompiler())->getDataVuoType();
			actualInputPortTypes << QString::fromStdString( actualType ? actualType->getModuleKey() : "event" );

			string actualDefault = (actualType ? static_cast<VuoCompilerInputEventPortClass *>(portClass->getCompiler())->getDataClass()->getDefaultValue() : "");
			actualInputPortDefaults << QString::fromStdString( actualDefault );
		}
		QCOMPARE(actualInputPortNames.join(", "), expectedInputPortNames.join(", "));
		QCOMPARE(actualInputPortDisplayNames.join(", "), expectedInputPortDisplayNames.join(", "));
		QCOMPARE(actualInputPortTypes.join(", "), expectedInputPortTypes.join(", "));
		QCOMPARE(actualInputPortDefaults.join(", "), expectedInputPortDefaults.join(", "));

		QStringList actualOutputPortNames;
		QStringList actualOutputPortDisplayNames;
		QStringList actualOutputPortTypes;
		for (VuoPortClass *portClass : nodeClass->getBase()->getOutputPortClasses())
		{
			actualOutputPortNames << QString::fromStdString( portClass->getName() );
			actualOutputPortDisplayNames << QString::fromStdString( static_cast<VuoCompilerPortClass *>(portClass->getCompiler())->getDisplayName() );

			VuoType *actualType = static_cast<VuoCompilerPortClass *>(portClass->getCompiler())->getDataVuoType();
			actualOutputPortTypes << QString::fromStdString( actualType ? actualType->getModuleKey() : "event" );
		}
		QCOMPARE(actualOutputPortNames.join(", "), expectedOutputPortNames.join(", "));
		QCOMPARE(actualOutputPortDisplayNames.join(", "), expectedOutputPortDisplayNames.join(", "));
		QCOMPARE(actualOutputPortTypes.join(", "), expectedOutputPortTypes.join(", "));

		delete issues;
		delete llvmModule;
		delete nodeClass;
	}

	void testOutputImage_data()
	{
		QTest::addColumn<QString>("sourcePath");
		QTest::addColumn<QStringList>("inputPorts");
		QTest::addColumn<QStringList>("inputValues");
		QTest::addColumn<bool>("hasOutputImage");
		QTest::addColumn<int>("pixelsWide");      // expected output image properties
		QTest::addColumn<int>("pixelsHigh");      //
		QTest::addColumn<QString>("colorDepth");  //
		QTest::addColumn<int>("x");  // index of pixel to check, from top left
		QTest::addColumn<int>("y");  //
		QTest::addColumn<double>("r");  // expected color of pixel
		QTest::addColumn<double>("g");  //
		QTest::addColumn<double>("b");  //
		QTest::addColumn<double>("a");  //

		{
			QStringList inputPorts;
			QStringList inputValues;
			inputPorts << "fill" << "vuoWidth" << "vuoHeight";
			inputValues << "\"0.0, 1.0, 1.0, 1.0\"" << "987" << "654";
			QTest::newRow("generate solid color") << "vuo.test.imageGenerator.color.fs" << inputPorts << inputValues
												  << true << 987 << 654 << "8bpc"
												  << 100 << 100 << 0.0 << 1.0 << 1.0 << 1.0;
		}
		{
			QStringList inputPorts;
			QStringList inputValues;
			inputPorts << "fill" << "vuoWidth" << "vuoHeight";
			inputValues << "\"0.0, 0.0, 1.0, 1.0\"" << "123" << "456";
			QTest::newRow("generate solid color with size") << "vuo.test.imageGenerator.colorSize.fs" << inputPorts << inputValues
															<< true << 123 << 456 << "8bpc"
															<< 5 << 5 << 0.0 << 0.0 << 1.0 << 1.0;
		}
		{
			QStringList inputPorts;
			QStringList inputValues;
			inputPorts << "vuoTime" << "vuoWidth" << "vuoHeight";
			inputValues << "0.5" << "100" << "100";
			QTest::newRow("generate solid color with time") << "vuo.test.imageGenerator.colorTime.fs" << inputPorts << inputValues
															<< true << 100 << 100 << "8bpc"
															<< 0 << 0 << 0.0 << 0.5 << 0.0 << 1.0;
		}
		{
			QStringList inputPorts;
			QStringList inputValues;
			inputPorts << "fill" << "vuoWidth" << "vuoHeight" << "vuoColorDepth";
			inputValues << "\"1.0, 0.0, 1.0, 1.0\"" << "200" << "400" << "\"16bpc\"";
			QTest::newRow("generate solid color with color depth") << "vuo.test.imageGenerator.colorDepth.fs" << inputPorts << inputValues
																   << true << 200 << 400 << "16bpc"
																   << 0 << 0 << 1.0 << 0.0 << 1.0 << 1.0;
		}
		{
			QStringList inputPorts;
			QStringList inputValues;
			inputPorts << "inputImage";
			inputValues << "{\"color\":\"1.0, 0.0, 0.0, 1.0\", \"pixelsWide\":400, \"pixelsHigh\":300}";
			QTest::newRow("filter image") << "vuo.test.imageFilter.invert.fs" << inputPorts << inputValues
										  << true << 400 << 300 << "8bpc"
										  << 233 << 122 << 0.0 << 1.0 << 1.0 << 1.0;
		}
		{
			QStringList inputPorts;
			QStringList inputValues;
			inputPorts << "inputImage" << "vuoWidth" << "vuoHeight";
			inputValues << "{\"color\":\"0.0, 1.0, 0.0, 1.0\", \"pixelsWide\":400, \"pixelsHigh\":300}" << "500" << "600";
			QTest::newRow("filter image with size") << "vuo.test.imageFilter.invertSize.fs" << inputPorts << inputValues
													<< true << 500 << 600 << "8bpc"
													<< 233 << 122 << 1.0 << 0.0 << 1.0 << 1.0;
		}
		{
			QStringList inputPorts;
			QStringList inputValues;
			inputPorts << "inputImage" << "vuoColorDepth";
			inputValues << "{\"color\":\"0.0, 0.0, 1.0, 1.0\", \"pixelsWide\":400, \"pixelsHigh\":300}" << "\"32bpc\"";
			QTest::newRow("filter image with color depth") << "vuo.test.imageFilter.invertDepth.fs" << inputPorts << inputValues
														   << true << 400 << 300 << "32bpc"
														   << 233 << 122 << 1.0 << 1.0 << 0.0 << 1.0;
		}
		{
			QStringList inputPorts;
			QStringList inputValues;
			inputPorts << "inputImage";
			inputValues << "{\"pointer\":0}";
			QTest::newRow("filter null image") << "vuo.test.imageFilter.invert.fs" << inputPorts << inputValues
											   << false << 0 << 0 << ""
											   << 0 << 0 << 0.0 << 0.0 << 0.0 << 0.0;
		}
		{
			QStringList inputPorts;
			QStringList inputValues;
			inputPorts << "image1" << "inputImage" << "image2";
			inputValues << "{\"color\":\"0.1, 0.0, 0.7, 1.0\", \"pixelsWide\":100, \"pixelsHigh\":100}"
						<< "{\"color\":\"0.2, 0.6, 0.0, 1.0\", \"pixelsWide\":100, \"pixelsHigh\":100}"
						<< "{\"color\":\"0.5, 0.0, 0.3, 1.0\", \"pixelsWide\":100, \"pixelsHigh\":100}";
			QTest::newRow("multiple image inputs") << "vuo.test.image.lightest.fs" << inputPorts << inputValues
												   << true << 100 << 100 << "8bpc"
												   << 0 << 0 << 0.5 << 0.6 << 0.7 << 1.0;
		}
		{
			QStringList inputPorts;
			QStringList inputValues;
			inputPorts << "image1" << "inputImage" << "image2";
			inputValues << "{\"color\":\"0.1, 0.0, 0.7, 1.0\", \"pixelsWide\":300, \"pixelsHigh\":400}"
						<< "{\"color\":\"0.2, 0.6, 0.0, 1.0\", \"pixelsWide\":100, \"pixelsHigh\":200}"
						<< "{\"color\":\"0.5, 0.0, 0.3, 1.0\", \"pixelsWide\":500, \"pixelsHigh\":600}";
			QTest::newRow("multiple image inputs, different sizes") << "vuo.test.image.lightest.fs" << inputPorts << inputValues
																	<< true << 300 << 400 << "8bpc"
																	<< 0 << 0 << 0.5 << 0.6 << 0.7 << 1.0;
		}
		{
			QStringList inputPorts;
			QStringList inputValues;
			inputPorts << "image1" << "inputImage" << "image2";
			inputValues << "{\"color\":\"0.1, 0.0, 0.7, 1.0\", \"pixelsWide\":100, \"pixelsHigh\":100}"
						<< "{\"pointer\":0}"
						<< "{\"color\":\"0.5, 0.0, 0.3, 1.0\", \"pixelsWide\":100, \"pixelsHigh\":100}";
			QTest::newRow("multiple image inputs, inputImage null") << "vuo.test.image.lightest.fs" << inputPorts << inputValues
																	<< true << 100 << 100 << "8bpc"
																	<< 0 << 0 << 0.5 << 0.0 << 0.7 << 1.0;
		}
		{
			QStringList inputPorts;
			QStringList inputValues;
			inputPorts << "image1" << "inputImage" << "image2";
			inputValues << "{\"pointer\":0}"
						<< "{\"color\":\"0.2, 0.6, 0.0, 1.0\", \"pixelsWide\":100, \"pixelsHigh\":100}"
						<< "{\"color\":\"0.5, 0.0, 0.3, 1.0\", \"pixelsWide\":100, \"pixelsHigh\":100}";
			QTest::newRow("multiple image inputs, first null") << "vuo.test.image.lightest.fs" << inputPorts << inputValues
															   << true << 100 << 100 << "8bpc"
															   << 0 << 0 << 0.5 << 0.6 << 0.3 << 1.0;
		}
		{
			QStringList inputPorts;
			QStringList inputValues;
			inputPorts << "fill" << "vuoWidth" << "vuoHeight" << "vuoWidth2" << "vuoHeight2";
			inputValues << "\"0.0, 0.0, 1.0, 1.0\"" << "100" << "200" << "300" << "400";
			QTest::newRow("multiple size inputs") << "vuo.test.imageGenerator.colorMultiSize.fs" << inputPorts << inputValues
												  << true << 100 << 200 << "8bpc"
												  << 100 << 100 << 0.0 << 0.0 << 1.0 << 1.0;
		}
		{
			QStringList inputPorts;
			QStringList inputValues;
			inputPorts << "startImage" << "endImage" << "progress";
			inputValues << "{\"color\":\"1.0, 0.0, 0.0, 1.0\", \"pixelsWide\":400, \"pixelsHigh\":300}"
						<< "{\"color\":\"0.0, 1.0, 0.0, 1.0\", \"pixelsWide\":400, \"pixelsHigh\":300}"
						<< "0.5";
			QTest::newRow("transition") << "vuo.test.imageTransition.noExtraPorts.fs" << inputPorts << inputValues
										<< true << 400 << 300 << "8bpc"
										<< 100 << 100 << 0.5 << 0.5 << 0.0 << 1.0;
		}
	}
	void testOutputImage()
	{
		QFETCH(QString, sourcePath);
		QFETCH(QStringList, inputPorts);
		QFETCH(QStringList, inputValues);
		QFETCH(bool, hasOutputImage);
		QFETCH(int, pixelsWide);
		QFETCH(int, pixelsHigh);
		QFETCH(QString, colorDepth);
		QFETCH(int, x);
		QFETCH(int, y);
		QFETCH(double, r);
		QFETCH(double, g);
		QFETCH(double, b);
		QFETCH(double, a);

		ModuleFileGroup::setSourceDir("shader");
		ModuleFileGroup m(sourcePath.toStdString());

		Module *llvmModule;
		VuoCompilerIssues *issues;
		compileIsfToLlvmModule(m, llvmModule, issues);

		QVERIFY2(issues->isEmpty(), issues->getLongDescription(false).c_str());

		VuoCompiler::verifyModule(llvmModule, issues);
		VuoCompiler::writeModuleToBitcode(llvmModule, VuoCompiler::getProcessTarget(), m.getCompiledPath(), issues);

		VuoCompiler *compiler = new VuoCompiler(m.getCompositionDir() + "/composition");
		CompositionFileGroup::setDestinationDir(m.getCompositionDir());
		CompositionFileGroup c(m.getModuleKey());

		VuoCompilerNodeClass *nodeClass = compiler->getNodeClass(m.getModuleKey());
		QVERIFY(nodeClass);
		string compositionStr = wrapNodeInComposition(nodeClass, compiler);
		compiler->compileCompositionString(compositionStr, c.getCompiledPath(), true, issues);
		compiler->linkCompositionToCreateDynamicLibrary(c.getCompiledPath(), c.getDynamicLibraryPath(), VuoCompiler::Optimization_NoModuleCaches);

		VuoRunner *runner = VuoRunner::newCurrentProcessRunnerFromDynamicLibrary(c.getDynamicLibraryPath(), ".");
		runner->setRuntimeChecking(true);
		runner->start();

		map<VuoRunner::Port *, json_object *> portsAndValues;
		makePortsAndValues(runner, inputPorts, inputValues, portsAndValues);
		runner->setPublishedInputPortValues(portsAndValues);

		VuoRunner::Port *firePort = runner->getPublishedInputPortWithName( inputPorts[0].toStdString() );
		runner->firePublishedInputPortEvent(firePort);
		runner->waitForFiredPublishedInputPortEvent();

		VuoRunner::Port *outputPort = runner->getPublishedOutputPortWithName("outputImage");
		json_object *outputJson = runner->getPublishedOutputPortValue(outputPort);
		VuoImage outputImage = VuoImage_makeFromJson(outputJson);
		VuoRetain(outputImage);
		json_object_put(outputJson);

		QCOMPARE(outputImage != nullptr, hasOutputImage);
		if (outputImage)
		{
			QCOMPARE(outputImage->pixelsWide, (unsigned long)pixelsWide);
			QCOMPARE(outputImage->pixelsHigh, (unsigned long)pixelsHigh);

			json_object *colorDepthJson = VuoImageColorDepth_getJson(VuoImage_getColorDepth(outputImage));
			QString colorDepthStr = json_object_get_string(colorDepthJson);
			QCOMPARE(colorDepthStr, colorDepth);

			checkPixel(outputImage, x, y, r, g, b, a);
		}
		VuoRelease(outputImage);

		runner->stop();

		delete issues;
		delete compiler;
		delete runner;
		VuoCompiler::reset();
	}

};

string TestVuoIsfModuleCompiler::ModuleFileGroup::testId = "TestVuoIsfModuleCompiler";
string TestVuoIsfModuleCompiler::ModuleFileGroup::sourceDir = "";
string TestVuoIsfModuleCompiler::ModuleFileGroup::moduleDir = "";
string TestVuoIsfModuleCompiler::CompositionFileGroup::testId = "TestVuoIsfModuleCompiler";
string TestVuoIsfModuleCompiler::CompositionFileGroup::sourceDir = "";
string TestVuoIsfModuleCompiler::CompositionFileGroup::destinationDir = "";

QTEST_APPLESS_MAIN(TestVuoIsfModuleCompiler)
#include "TestVuoIsfModuleCompiler.moc"

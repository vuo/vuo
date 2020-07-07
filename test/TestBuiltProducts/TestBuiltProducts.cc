/**
 * @file
 * TestBuiltProducts interface and implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <stdio.h>
#include "TestCompositionExecution.hh"
#include "VuoMacAppExporter.hh"
#include "VuoRendererComposition.hh"


// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(string);


/**
 * Tests for build problems in Vuo.framework, Vuo.app, and compositions exported as apps
 * that might prevent them from running on some systems.
 */
class TestBuiltProducts : public TestCompositionExecution
{
	Q_OBJECT

private:

	void getLinesFromCommand(string command, vector<string> &linesFromCommand)
	{
		do
		{
			FILE *outputFromCommand = popen(command.c_str(), "r");
			QVERIFY(outputFromCommand != NULL);

			char *lineWithoutNullTerminator = NULL;
			size_t len = 0;
			while ((lineWithoutNullTerminator = fgetln(outputFromCommand, &len)) != NULL)
			{
				string line(lineWithoutNullTerminator, len);
				linesFromCommand.push_back(line);
			}

			if (lineWithoutNullTerminator)
				free(lineWithoutNullTerminator);

			pclose(outputFromCommand);
		} while (linesFromCommand.empty());
	}

	void getBinaryFilePaths(bool includeExecutables, bool includeStaticLibraries, bool includeSharedLibraries, set<string> &binaryFilePaths)
	{
		set<string> allFilePaths;

		// No need to separately check the framework, since it's included in Vuo.app below.
		// string frameworkDir                          = string(BINARY_DIR) + "/lib/Vuo.framework";
		// set<VuoFileUtilities::File *> frameworkFiles = VuoFileUtilities::findAllFilesInDirectory(frameworkDir, set<string>(), true);
		// foreach (VuoFileUtilities::File *file, frameworkFiles)
		//     allFilePaths.insert(frameworkDir + "/" + file->getRelativePath());

		allFilePaths.insert(string(BINARY_DIR) + "/bin/vuo-compile-for-framework");
		allFilePaths.insert(string(BINARY_DIR) + "/bin/vuo-debug");
		allFilePaths.insert(string(BINARY_DIR) + "/bin/vuo-export");
		allFilePaths.insert(string(BINARY_DIR) + "/bin/vuo-link");
		// No need to check vuo-compile since we don't distribute it.
		// allFilePaths.insert(string(BINARY_DIR) + "/bin/vuo-compile");

		string editorDir                          = string(BINARY_DIR) + "/bin/Vuo.app";
		set<VuoFileUtilities::File *> editorFiles = VuoFileUtilities::findAllFilesInDirectory(editorDir, set<string>(), true);
		foreach (VuoFileUtilities::File *file, editorFiles)
			allFilePaths.insert(editorDir + "/" + file->getRelativePath());

		string nodeDir                            = string(BINARY_DIR) + "/node";
		set<VuoFileUtilities::File *> nodeSetDirs = VuoFileUtilities::findAllFilesInDirectory(nodeDir, set<string>(), true);
		foreach (VuoFileUtilities::File *nodeSetDir, nodeSetDirs)
		{
			if (VuoStringUtilities::beginsWith(nodeSetDir->getRelativePath(), "vuo."))
			{
				string exampleDir = nodeDir + "/" + nodeSetDir->getRelativePath() + "/examples";
				if (VuoFileUtilities::fileExists(exampleDir))
				{
					set<VuoFileUtilities::File *> compositionFiles = VuoFileUtilities::findAllFilesInDirectory(exampleDir, set<string>(), true);
					foreach (VuoFileUtilities::File *compositionFile, compositionFiles)
					{
						allFilePaths.insert(exampleDir + "/" + compositionFile->getRelativePath());
					}
				}
			}
		}

		// Avoid running the (relatively expensive) `file` command on files/folders that definitely aren't code binaries.
		set<string> fileEndingsToIgnore;
		fileEndingsToIgnore.insert(".DS_Store");
		fileEndingsToIgnore.insert(".plist");
		fileEndingsToIgnore.insert(".h");
		fileEndingsToIgnore.insert(".hh");
		fileEndingsToIgnore.insert(".hpp");
		fileEndingsToIgnore.insert(".inc");
		fileEndingsToIgnore.insert(".d");
		fileEndingsToIgnore.insert(".td");
		fileEndingsToIgnore.insert(".def");
		fileEndingsToIgnore.insert(".defs");
		fileEndingsToIgnore.insert(".map");
		fileEndingsToIgnore.insert(".gen");
		fileEndingsToIgnore.insert(".bc");
		fileEndingsToIgnore.insert(".vuo");
		fileEndingsToIgnore.insert(".vuonode");
		fileEndingsToIgnore.insert(".md");
		fileEndingsToIgnore.insert(".txt");
		fileEndingsToIgnore.insert(".TXT");
		fileEndingsToIgnore.insert(".otf");
		fileEndingsToIgnore.insert(".png");
		fileEndingsToIgnore.insert(".pdf");
		fileEndingsToIgnore.insert(".icns");
		fileEndingsToIgnore.insert(".lproj");
		fileEndingsToIgnore.insert(".nib");
		fileEndingsToIgnore.insert(".strings");
		fileEndingsToIgnore.insert(".vs");
		fileEndingsToIgnore.insert(".gs");
		fileEndingsToIgnore.insert(".fs");
		fileEndingsToIgnore.insert(".glsl");
		fileEndingsToIgnore.insert(".jpg");
		fileEndingsToIgnore.insert(".ttf");
		fileEndingsToIgnore.insert(".metainfo");
		fileEndingsToIgnore.insert(".conf");
		fileEndingsToIgnore.insert(".qml");
		fileEndingsToIgnore.insert("/qmldir");
		fileEndingsToIgnore.insert("/PkgInfo");
		fileEndingsToIgnore.insert("/Current");
		fileEndingsToIgnore.insert("/Headers");
		fileEndingsToIgnore.insert("/Helpers");
		fileEndingsToIgnore.insert("/Resources");
		fileEndingsToIgnore.insert("/CodeResources");

		foreach (string path, allFilePaths)
		{
			bool skip = false;
			foreach (string ending, fileEndingsToIgnore)
				if (VuoStringUtilities::endsWith(path, ending))
				{
					skip = true;
					break;
				}
			if (skip)
				continue;

			if (path.find("/Headers/") != string::npos)
				continue;

			if (VuoFileUtilities::isSymlink(path))
				continue;

			string command = string("file \"") + path + string("\"");
			vector<string> linesFromCommand;
			getLinesFromCommand(command, linesFromCommand);
			string line = linesFromCommand.at(0);

			if ((includeExecutables && (line.find("Mach-O") != string::npos)) ||
					(includeStaticLibraries && (line.find("random library") != string::npos)) ||
					(includeSharedLibraries && (line.find("shared library") != string::npos)))
			{
				char *resolvedPath = realpath(path.c_str(), NULL);
				binaryFilePaths.insert(resolvedPath);
				free(resolvedPath);
			}
		}
	}

private slots:

	void testReferencesToLibraries_data()
	{
		QTest::addColumn< string >("binaryPath");

		set<string> binaryFilePaths;
		getBinaryFilePaths(true, false, true, binaryFilePaths);

		foreach (string path, binaryFilePaths)
			QTest::newRow(VuoStringUtilities::substrAfter(path, string(BINARY_DIR) + "/").c_str()) << path;
	}
	void testReferencesToLibraries()
	{
		QFETCH(string, binaryPath);

		// Ensure our binaries don't dynamically reference absolute paths to non-Apple-provided libraries.
		{
		vector<string> blacklistedLibraryPaths;
		blacklistedLibraryPaths.push_back("/usr/local");
		blacklistedLibraryPaths.push_back("/usr/X11");
		blacklistedLibraryPaths.push_back("Modules/libc++.dylib");  // Ensure this module references the system's libc++.
		char *userdir = getenv("HOME");
		QVERIFY(userdir != NULL);
		blacklistedLibraryPaths.push_back(userdir);

		string command = string("otool -L \"") + binaryPath + string("\"");
		vector<string> linesFromCommand;
		getLinesFromCommand(command, linesFromCommand);

		bool isFirst = true;
		foreach (string outputLine, linesFromCommand)
		{
			if (isFirst)
			{
				string expectedLine = VuoStringUtilities::endsWith(binaryPath, ".a") ?
										  "Archive : " + binaryPath + "\n" :
										  binaryPath + ":\n";
				QCOMPARE(QString(outputLine.c_str()), QString(expectedLine.c_str()));
				isFirst = false;
				continue;
			}

			foreach (string path, blacklistedLibraryPaths)
				QVERIFY2(outputLine.find(path) == string::npos, (binaryPath + " : " + outputLine).c_str());
		}
		QVERIFY(! isFirst);
		}


		// Ensure our binaries don't search any absolute rpaths.
		{
			// `otool -l` shows all load commands.  We're only interested in LC_RPATH.
			// Each LC_RPATH command looks like this:
			//
			//     Load command 42
			//               cmd LC_RPATH
			//           cmdsize 56
			//              path /Users/me/prj/vuo/trunk/framework/bin (offset 12)
			string command = string("otool -l \"") + binaryPath + "\"";

			vector<string> linesFromCommand;
			getLinesFromCommand(command, linesFromCommand);

			for (vector<string>::iterator it = linesFromCommand.begin(); it != linesFromCommand.end(); ++it)
			{
				string line = VuoStringUtilities::trim(*it);
				if (line != "cmd LC_RPATH")
					continue;

				for (++it; it != linesFromCommand.end(); ++it)
				{
					string line = VuoStringUtilities::trim(*it);
					if (!VuoStringUtilities::beginsWith(line, "path "))
						continue;

					string path = VuoStringUtilities::substrAfter(line, "path ");
					QVERIFY2(!VuoStringUtilities::beginsWith(path, "/"),
							 (string("The binary searches this absolute rpath: ") + path).c_str());
					break;
				}
			}
		}
	}

	/* Disabled for now, since there are no longer any known-bad load commands.
	void testLoadCommands_data()
	{
		QTest::addColumn< string >("binaryPath");

		set<string> binaryFilePaths;
		getBinaryFilePaths(true, true, true, binaryFilePaths);

		foreach (string path, binaryFilePaths)
			QTest::newRow(VuoStringUtilities::substrAfter(path, string(BINARY_DIR) + "/").c_str()) << path;
	}
	void testLoadCommands()
	{
		QFETCH(string, binaryPath);

		set<string> blacklistedLoadCommands;
		blacklistedLoadCommands.insert("LC_VERSION_MIN_MACOSX");	// 0x24; available on Mac OS 10.7+
		blacklistedLoadCommands.insert("LC_FUNCTION_STARTS");		// 0x26; available on Mac OS 10.7+
		blacklistedLoadCommands.insert("LC_MAIN");					// 0x28; available on Mac OS 10.8+

		map<string, set<string> > whitelistedLoadCommandsForPath;
		{
			set<string> qtPaths;
			qtPaths.insert("QtCore.framework");
			qtPaths.insert("QtGui.framework");
			qtPaths.insert("QtMacExtras.framework");
			qtPaths.insert("QtOpenGL.framework");
			qtPaths.insert("QtPrintSupport.framework");
			qtPaths.insert("QtWidgets.framework");
			qtPaths.insert("QtXml.framework");
			qtPaths.insert("QtNetwork.framework");
			qtPaths.insert("QtPlugins");
			for (set<string>::iterator i = qtPaths.begin(); i != qtPaths.end(); ++i)
			{
				string qtPath = *i;
				whitelistedLoadCommandsForPath[qtPath].insert("LC_VERSION_MIN_MACOSX");
				whitelistedLoadCommandsForPath[qtPath].insert("0x00000024");
				whitelistedLoadCommandsForPath[qtPath].insert("LC_FUNCTION_STARTS");
				whitelistedLoadCommandsForPath[qtPath].insert("0x00000026");
			}
		}
		{
			set<string> graphvizPaths;
			graphvizPaths.insert("libcdt.dylib");
			graphvizPaths.insert("libgraph.dylib");
			graphvizPaths.insert("libgvc.dylib");
			graphvizPaths.insert("libgvplugin_core.dylib");
			graphvizPaths.insert("libgvplugin_dot_layout.dylib");
			graphvizPaths.insert("libpathplan.dylib");
			graphvizPaths.insert("libxdot.dylib");
			for (set<string>::iterator i = graphvizPaths.begin(); i != graphvizPaths.end(); ++i)
			{
				string ffmpegPath = *i;
				whitelistedLoadCommandsForPath[ffmpegPath].insert("LC_VERSION_MIN_MACOSX");
				whitelistedLoadCommandsForPath[ffmpegPath].insert("0x00000024");
				whitelistedLoadCommandsForPath[ffmpegPath].insert("LC_FUNCTION_STARTS");
				whitelistedLoadCommandsForPath[ffmpegPath].insert("0x00000026");
			}
		}
		{
			string llvmPath = "Frameworks/llvm.framework";
			whitelistedLoadCommandsForPath[llvmPath].insert("LC_VERSION_MIN_MACOSX");
			whitelistedLoadCommandsForPath[llvmPath].insert("0x00000024");
			whitelistedLoadCommandsForPath[llvmPath].insert("LC_FUNCTION_STARTS");
			whitelistedLoadCommandsForPath[llvmPath].insert("0x00000026");
		}
		{
			string path = "libassimp.dylib";
			whitelistedLoadCommandsForPath[path].insert("LC_VERSION_MIN_MACOSX");
			whitelistedLoadCommandsForPath[path].insert("LC_FUNCTION_STARTS");
		}
		{
			set<string> ffmpegPaths;
			ffmpegPaths.insert("libavcodec.dylib");
			ffmpegPaths.insert("libavdevice.dylib");
			ffmpegPaths.insert("libavfilter.dylib");
			ffmpegPaths.insert("libavformat.dylib");
			ffmpegPaths.insert("libavutil.dylib");
			ffmpegPaths.insert("libswresample.dylib");
			ffmpegPaths.insert("libswscale.dylib");
			for (set<string>::iterator i = ffmpegPaths.begin(); i != ffmpegPaths.end(); ++i)
			{
				string ffmpegPath = *i;
				whitelistedLoadCommandsForPath[ffmpegPath].insert("LC_VERSION_MIN_MACOSX");
				whitelistedLoadCommandsForPath[ffmpegPath].insert("0x00000024");
				whitelistedLoadCommandsForPath[ffmpegPath].insert("LC_FUNCTION_STARTS");
				whitelistedLoadCommandsForPath[ffmpegPath].insert("0x00000026");
			}
		}
		{
			string path = "libfreeimage.dylib";
			whitelistedLoadCommandsForPath[path].insert("LC_VERSION_MIN_MACOSX");
			whitelistedLoadCommandsForPath[path].insert("LC_FUNCTION_STARTS");
		}
		{
			string freenectPath = "libfreenect.dylib";
			whitelistedLoadCommandsForPath[freenectPath].insert("LC_VERSION_MIN_MACOSX");
			whitelistedLoadCommandsForPath[freenectPath].insert("0x00000024");
			whitelistedLoadCommandsForPath[freenectPath].insert("LC_FUNCTION_STARTS");
			whitelistedLoadCommandsForPath[freenectPath].insert("0x00000026");
		}
		{
			string path = "libLeap.dylib";
			whitelistedLoadCommandsForPath[path].insert("LC_VERSION_MIN_MACOSX");
			whitelistedLoadCommandsForPath[path].insert("0x00000024");
			whitelistedLoadCommandsForPath[path].insert("LC_FUNCTION_STARTS");
			whitelistedLoadCommandsForPath[path].insert("0x00000026");
		}
		{
			string usbPath = "libusb.dylib";
			whitelistedLoadCommandsForPath[usbPath].insert("LC_VERSION_MIN_MACOSX");
			whitelistedLoadCommandsForPath[usbPath].insert("0x00000024");
			whitelistedLoadCommandsForPath[usbPath].insert("LC_FUNCTION_STARTS");
			whitelistedLoadCommandsForPath[usbPath].insert("0x00000026");
		}
		{
			string compositionLoaderPath = "VuoCompositionLoader.app/Contents/MacOS/VuoCompositionLoader";
			whitelistedLoadCommandsForPath[compositionLoaderPath].insert("LC_VERSION_MIN_MACOSX");
			whitelistedLoadCommandsForPath[compositionLoaderPath].insert("0x00000024");
			whitelistedLoadCommandsForPath[compositionLoaderPath].insert("LC_FUNCTION_STARTS");
			whitelistedLoadCommandsForPath[compositionLoaderPath].insert("0x00000026");
		}
		{
			string commandlineBinaryPath = "framework/vuo-";
			whitelistedLoadCommandsForPath[commandlineBinaryPath].insert("LC_VERSION_MIN_MACOSX");
			whitelistedLoadCommandsForPath[commandlineBinaryPath].insert("0x00000024");
			whitelistedLoadCommandsForPath[commandlineBinaryPath].insert("LC_FUNCTION_STARTS");
			whitelistedLoadCommandsForPath[commandlineBinaryPath].insert("0x00000026");
		}
		{
			string commandlineBinaryPath = "Helpers/vuo-";
			whitelistedLoadCommandsForPath[commandlineBinaryPath].insert("LC_VERSION_MIN_MACOSX");
			whitelistedLoadCommandsForPath[commandlineBinaryPath].insert("0x00000024");
			whitelistedLoadCommandsForPath[commandlineBinaryPath].insert("LC_FUNCTION_STARTS");
			whitelistedLoadCommandsForPath[commandlineBinaryPath].insert("0x00000026");
		}

		string command = string("otool -l \"") + binaryPath + string("\"");
		vector<string> linesFromCommand;
		getLinesFromCommand(command, linesFromCommand);

		foreach (string outputLine, linesFromCommand)
		{
			// Ignore unknown load commands that are known to be harmless.
			bool isWhitelisted = false;
			for (map<string, set<string> >::iterator i = whitelistedLoadCommandsForPath.begin(); i != whitelistedLoadCommandsForPath.end(); ++i)
				for (set<string>::iterator j = i->second.begin(); j != i->second.end(); ++j)
					if (binaryPath.find(i->first) != string::npos && outputLine.find(*j) != string::npos)
						isWhitelisted = true;
			if (isWhitelisted)
				continue;

			QVERIFY2(outputLine.find("Unknown") == string::npos, (binaryPath + " : " + outputLine).c_str());

			foreach (string loadCommand, blacklistedLoadCommands)
				QVERIFY2(outputLine.find(loadCommand) == string::npos, (binaryPath + " : " + outputLine).c_str());
		}
	}
*/

	void testInstructions_data()
	{
		QTest::addColumn<string>("binaryPath");

		set<string> binaryFilePaths;
		getBinaryFilePaths(true, true, true, binaryFilePaths);

		foreach (string path, binaryFilePaths)
			// Ignore libraries targeting specific processor features.
			if (path.find("_avx2.dylib") == string::npos)
				QTest::newRow(VuoStringUtilities::substrAfter(path, string(BINARY_DIR) + "/").c_str()) << path;
	}
	/**
	 * Ensures binaries do not contain CPU instructions that aren't implemented on some systems Vuo supports.
	 */
	void testInstructions()
	{
		QFETCH(string, binaryPath);
		printf("%s\n", binaryPath.c_str()); fflush(stdout);

		string command = string("/usr/local/bin/gobjdump --disassemble --wide --no-show-raw-insn --section=.text \"") + binaryPath + string("\"");
		command += " 2>&1 ";
		// Omit instruction address
		command += "| cut -f 2-";
		// Omit blank lines
		command += "| grep -v '^$'";
		vector<string> linesFromCommand;
		getLinesFromCommand(command, linesFromCommand);

		string unavailableInstructions[] = {
			// SSE4.1
			// https://b33p.net/kosada/node/8811 — removed BLENDPS, PACKUSDW, PMOVSXWD, PMULLD, PMINSD, PMAXSD, PEXTRD
				"mpsadbw", "phminposuw", "pmuldq", "dpps", "dppd", "blendpd", "blendvps", "blendvpd",
				"pblendvb", "pblendw", "pminsb", "pmaxsb", "pminuw", "pmaxuw", "pminud", "pmaxud",
				"roundps", "roundss", "roundpd", "roundsd", "insertps", "pinsrb", "pinsrd", "pinsrq",
				"extractps", "pextrb", "pextrq", "pmovsxbw", "pmovzxbw", "pmovsxbd", "pmovzxbd",
				"pmovsxbq", "pmovzxbq", "pmovzxwd", "pmovsxwq", "pmovzxwq", "pmovsxdq", "pmovzxdq",
				"ptest", "pcmpeqq", "movntdqa",

			// SSE4a
				"lzcnt", "popcnt", "extrq", "insertq", "movntsd", "movntss",

			// SSE4.2
				"crc32", "pcmpestri", "pcmpestrm", "pcmpistri", "pcmpistrm", "pcmpgtq",

			// AVX
				"broadcastss", "broadcastsd", "broadcastf128", "insertf128", "extractf128",
				"maskmovps", "maskmovpd", "permilps", "permilpd", "perm2f128", "zeroall", "zeroupper",
				"vxorps", "vmovups",

			// AVX2
				"vbroadcasti128",
				"vextracti128",
				"vgatherdpd", "vgatherdps", "vgatherqpd", "vgatherqps",
				"vinserti128",
				"vmovntdqa",
				"vmpsadbw",
				"vpabsb", "vpabsd", "vpabsw",
				"vpackssdw", "vpacksswb", "vpackusdw", "vpackuswb",
				"vpaddb", "vpaddd", "vpaddq", "vpaddsb", "vpaddsw", "vpaddusb", "vpaddusw", "vpaddw",
				"vpalignr",
				"vpand", "vpandn",
				"vpavgb", "vpavgw",
				"vpblendd", "vpblendvb", "vpblendw",
				"vpbroadcastb", "vpbroadcastd", "vpbroadcastq", "vpbroadcastw",
				"vpcmpeqb", "vpcmpeqd", "vpcmpeqq", "vpcmpeqw", "vpcmpgtb", "vpcmpgtd", "vpcmpgtq", "vpcmpgtw",
				"vperm2i128", "vpermd", "vpermpd", "vpermps", "vpermq",
				"vpgatherdd", "vpgatherdq", "vpgatherqd", "vpgatherqq",
				"vphaddd", "vphaddsw", "vphaddw",
				"vphsubd", "vphsubsw", "vphsubw",
				"vpmaddubsw",
				"vpmaddwd",
				"vpmaskmovd", "vpmaskmovq",
				"vpmaxsb", "vpmaxsd", "vpmaxsw", "vpmaxub", "vpmaxud", "vpmaxuw",
				"vpminsb", "vpminsd", "vpminsw", "vpminub", "vpminud", "vpminuw",
				"vpmovmskb", "vpmovsxbd", "vpmovsxbq", "vpmovsxbw", "vpmovsxdq", "vpmovsxwd", "vpmovsxwq", "vpmovzxbd", "vpmovzxbq", "vpmovzxbw", "vpmovzxdq", "vpmovzxwd", "vpmovzxwq",
				"vpmuldq", "vpmulhrsw", "vpmulhuw", "vpmulhw", "vpmulld", "vpmullw", "vpmuludq",
				"vpor",
				"vpsadbw",
				"vpshufb", "vpshufd", "vpshufhw", "vpshuflw",
				"vpsignb", "vpsignd", "vpsignw",
				"vpslld", "vpslldq", "vpsllq", "vpsllvd", "vpsllvq", "vpsllw",
				"vpsrad", "vpsravd", "vpsraw", "vpsrld", "vpsrldq", "vpsrlq", "vpsrlvd", "vpsrlvq", "vpsrlw",
				"vpsubb", "vpsubd", "vpsubq", "vpsubsb", "vpsubsw", "vpsubusb", "vpsubusw", "vpsubw",
				"vpunpckhbw", "vpunpckhdq", "vpunpckhqdq", "vpunpckhwd", "vpunpcklbw", "vpunpckldq", "vpunpcklqdq", "vpunpcklwd",
				"vpxor",
		};

		set<string> whitelistedInstructions;
		if (binaryPath.find("/Helpers/ld") != string::npos)
			// "It's always worked so far"
			whitelistedInstructions.insert("vmovups");
		// Not an instruction
		whitelistedInstructions.insert("crc32.o");

		foreach (string line, linesFromCommand)
		{
			bool isWhitelisted = false;
			for (set<string>::iterator j = whitelistedInstructions.begin(); j != whitelistedInstructions.end(); ++j)
				if (line.find(*j) != string::npos)
					isWhitelisted = true;
			if (isWhitelisted)
				continue;

			int foundUnavailableInstruction = -1;
			for (int i = 0; i < sizeof(unavailableInstructions)/sizeof(string); ++i)
			{
				if (VuoStringUtilities::beginsWith(line, unavailableInstructions[i])
				 || VuoStringUtilities::beginsWith(line, string("v") + unavailableInstructions[i]))
				{
					foundUnavailableInstruction = i;
					break;
				}
			}

			if (foundUnavailableInstruction != -1)
				QFAIL(("Found unavailable instruction: " + unavailableInstructions[foundUnavailableInstruction]).c_str());
		}
	}

	/**
	 * Tests conformance to the Export Administration Regulations of the U.S. Bureau of Industry and Security.
	 */
	void testEncryptionLibraries_data()
	{
		QTest::addColumn<QString>("compositionName");
		QTest::addColumn<bool>("expectedEncryption");

		QTest::newRow("composition with 'Fetch Image' node") << "FetchImage" << true;
		QTest::newRow("composition with Image ports but no 'Fetch Image' node") << "MakeTextImage" << false;
	}
	void testEncryptionLibraries()
	{
		QFETCH(QString, compositionName);
		QFETCH(bool, expectedEncryption);

		// Build and export composition.
		string exportedAppPath = VuoFileUtilities::makeTmpDir(compositionName.toStdString());
		string exportedExecutablePath = exportedAppPath + "/Contents/MacOS/" + compositionName.toStdString();
		{
			string compositionPath = getCompositionPath(compositionName.toStdString() + ".vuo");
			VuoCompiler *compiler = initCompiler();
			VuoMacAppExporter exporter;
			exporter.setComposition(VuoFileUtilities::readFileToString(compositionPath));
			exporter.setCompiler(compiler);
			exporter.performExport(exportedAppPath);
			delete compiler;
			QVERIFY2(VuoFileUtilities::fileExists(exportedExecutablePath), exportedExecutablePath.c_str());
		}

		// Check if the exported composition uses libcrypto and/or libopenssl.
		{
			string command = string("nm \"") + exportedExecutablePath + string("\"");
			vector<string> linesFromCommand;
			getLinesFromCommand(command, linesFromCommand);

			set<string> encryptionSymbols;
			encryptionSymbols.insert("curl"); // libcurl.dylib includes OpenSSL symbols (but doesn't export them).
			bool foundEncryption = false;

			foreach (string line, linesFromCommand)
			{
				foreach (string symbol, encryptionSymbols)
				{
					if (line.find(symbol) != string::npos)
					{
						foundEncryption = true;
						break;
					}
				}
			}

			QCOMPARE(foundEncryption, expectedEncryption);
		}

		system(("rm -rf \"" + exportedAppPath + "\"").c_str());
	}
};


int main(int argc, char *argv[])
{
	VuoRendererComposition::createAutoreleasePool();

	// Tell Qt where to find its plugins.
	QApplication::setLibraryPaths(QStringList((VuoFileUtilities::getVuoFrameworkPath() + "/../QtPlugins").c_str()));

	// https://bugreports.qt-project.org/browse/QTBUG-29197
	qputenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM", "1");

	QApplication app(argc, argv);
	TestBuiltProducts tc;
	return QTest::qExec(&tc, argc, argv);
}

#include "TestBuiltProducts.moc"

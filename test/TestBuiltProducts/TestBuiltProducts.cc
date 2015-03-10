/**
 * @file
 * TestBuiltProducts interface and implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <QtTest/QtTest>
#include "VuoFileUtilities.hh"
#include "VuoStringUtilities.hh"


// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(string);


/**
 * Sanity-check Vuo.framework and Vuo Editor.app.
 */
class TestBuiltProducts : public QObject
{
	Q_OBJECT

private:

	void getLinesFromCommand(string command, vector<string> &linesFromCommand)
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
	}

	void getBinaryFilePaths(bool includeExecutables, bool includeStaticLibraries, bool includeSharedLibraries, set<string> &binaryFilePaths)
	{
		set<string> allFilePaths;

		string frameworkDir = "../../framework/Vuo.framework";
		set<VuoFileUtilities::File *> frameworkFiles = VuoFileUtilities::findAllFilesInDirectory(frameworkDir);
		foreach (VuoFileUtilities::File *file, frameworkFiles)
			allFilePaths.insert(frameworkDir + "/" + file->getRelativePath());

		allFilePaths.insert("../../framework/vuo-debug");
		allFilePaths.insert("../../framework/vuo-compile");
		allFilePaths.insert("../../framework/vuo-link");
		allFilePaths.insert("../../framework/vuo-render");

		string editorDir = "../../editor/VuoEditorApp/Vuo Editor.app";
		set<VuoFileUtilities::File *> editorFiles = VuoFileUtilities::findAllFilesInDirectory(editorDir);
		foreach (VuoFileUtilities::File *file, editorFiles)
			allFilePaths.insert(editorDir + "/" + file->getRelativePath());

		string nodeDir = "../../node";
		set<VuoFileUtilities::File *> nodeSetDirs = VuoFileUtilities::findAllFilesInDirectory(nodeDir);
		foreach (VuoFileUtilities::File *nodeSetDir, nodeSetDirs)
		{
			if (VuoStringUtilities::beginsWith(nodeSetDir->getRelativePath(), "vuo."))
			{
				string exampleDir = nodeDir + "/" + nodeSetDir->getRelativePath() + "/examples";
				if (VuoFileUtilities::fileExists(exampleDir))
				{
					set<VuoFileUtilities::File *> compositionFiles = VuoFileUtilities::findAllFilesInDirectory(exampleDir);
					foreach (VuoFileUtilities::File *compositionFile, compositionFiles)
					{
						allFilePaths.insert(exampleDir + "/" + compositionFile->getRelativePath());
					}
				}
			}
		}

		/// @todo This is a temporary workaround for VuoFileUtilities::findAllFilesInDirectory() not searching recursively. - https://b33p.net/kosada/node/2468
		vector<string> subDirs;
		subDirs.push_back(frameworkDir + "/Frameworks/CRuntime.framework");
		subDirs.push_back(frameworkDir + "/Frameworks/ICU.framework");
		subDirs.push_back(frameworkDir + "/Frameworks/JSON-C.framework");
		subDirs.push_back(frameworkDir + "/Frameworks/graphviz.framework");
		subDirs.push_back(frameworkDir + "/Frameworks/graphviz.framework/graphviz");
		subDirs.push_back(frameworkDir + "/Frameworks/zmq.framework");
		subDirs.push_back(frameworkDir + "/Frameworks/QtGui.framework");
		subDirs.push_back(frameworkDir + "/Frameworks/QtCore.framework");
		subDirs.push_back(frameworkDir + "/Frameworks/QtWidgets.framework");
		subDirs.push_back(frameworkDir + "/Frameworks/QtMacExtras.framework");
		subDirs.push_back(frameworkDir + "/Frameworks/QtXml.framework");
		subDirs.push_back(frameworkDir + "/Frameworks/QtPrintSupport.framework");
		subDirs.push_back(frameworkDir + "/Frameworks/QtOpenGL.framework");
		subDirs.push_back(frameworkDir + "/MacOS/Clang/bin");
		subDirs.push_back(editorDir + "/Contents/MacOS");
		subDirs.push_back(editorDir + "/Contents/plugins/accessible");
		foreach (string subDir, subDirs)
		{
			set<VuoFileUtilities::File *> subDirFiles = VuoFileUtilities::findAllFilesInDirectory(subDir);
			foreach (VuoFileUtilities::File *file, subDirFiles)
				allFilePaths.insert(subDir + "/" + file->getRelativePath());
		}

		foreach (string path, allFilePaths)
		{
			string command = string("file \"") + path + string("\"");
			vector<string> linesFromCommand;
			getLinesFromCommand(command, linesFromCommand);

			QVERIFY(! linesFromCommand.empty());
			string line = linesFromCommand.at(0);

			if ((includeExecutables && (line.find("Mach-O") != string::npos)) ||
					(includeStaticLibraries && (line.find("random library") != string::npos)) ||
					(includeSharedLibraries && (line.find("shared library") != string::npos)))
				binaryFilePaths.insert(path);
		}
	}

private slots:

	void testReferencesToLibraries_data()
	{
		QTest::addColumn< string >("binaryPath");

		set<string> binaryFilePaths;
		getBinaryFilePaths(true, true, true, binaryFilePaths);

		foreach (string path, binaryFilePaths)
			QTest::newRow(path.c_str()) << path;
	}
	void testReferencesToLibraries()
	{
		QFETCH(string, binaryPath);

		vector<string> blacklistedLibraryPaths;
		blacklistedLibraryPaths.push_back("/usr/local");
		blacklistedLibraryPaths.push_back("/usr/X11");
		char *userName = getenv("USER");
		QVERIFY(userName != NULL);
		blacklistedLibraryPaths.push_back(userName);
		blacklistedLibraryPaths.push_back("libc++");

		string command = string("otool -L \"") + binaryPath + string("\"");
		vector<string> linesFromCommand;
		getLinesFromCommand(command, linesFromCommand);

		bool isFirst = true;
		foreach (string line, linesFromCommand)
		{
			if (isFirst)
			{
				string expectedLine = VuoStringUtilities::endsWith(binaryPath, ".a") ?
										  "Archive : " + binaryPath + "\n" :
										  binaryPath + ":\n";
				QCOMPARE(QString(line.c_str()), QString(expectedLine.c_str()));
				isFirst = false;
				continue;
			}

			foreach (string path, blacklistedLibraryPaths)
				QVERIFY2(line.find(path) == string::npos, (binaryPath + " : " + line).c_str());
		}
		QVERIFY(! isFirst);
	}

	void testLoadCommands_data()
	{
		QTest::addColumn< string >("binaryPath");

		set<string> binaryFilePaths;
		getBinaryFilePaths(true, true, true, binaryFilePaths);

		foreach (string path, binaryFilePaths)
			QTest::newRow(path.c_str()) << path;
	}
	void testLoadCommands()
	{
		QFETCH(string, binaryPath);

		set<string> blacklistedLoadCommands;
		blacklistedLoadCommands.insert("LC_VERSION_MIN_MACOSX");	// 0x24
		blacklistedLoadCommands.insert("LC_FUNCTION_STARTS");		// 0x26
		blacklistedLoadCommands.insert("LC_MAIN");					// 0x28
		blacklistedLoadCommands.insert("LC_DATA_IN_CODE");			// 0x29
		blacklistedLoadCommands.insert("LC_SOURCE_VERSION");		// 0x2a
		blacklistedLoadCommands.insert("LC_DYLIB_CODE_SIGN_DRS");	// 0x2b

		map<string, set<string> > whitelistedLoadCommandsForPath;
		string qtPath = "Vuo.framework/Frameworks/Qt";
		whitelistedLoadCommandsForPath[qtPath].insert("LC_VERSION_MIN_MACOSX");
		whitelistedLoadCommandsForPath[qtPath].insert("0x00000024");
		whitelistedLoadCommandsForPath[qtPath].insert("LC_FUNCTION_STARTS");
		whitelistedLoadCommandsForPath[qtPath].insert("0x00000026");
		whitelistedLoadCommandsForPath[qtPath].insert("LC_DATA_IN_CODE");
		whitelistedLoadCommandsForPath[qtPath].insert("0x00000029");
		string graphvizPath = "Vuo.framework/Frameworks/graphviz.framework";
		whitelistedLoadCommandsForPath[graphvizPath].insert("LC_VERSION_MIN_MACOSX");
		whitelistedLoadCommandsForPath[graphvizPath].insert("0x00000024");
		whitelistedLoadCommandsForPath[graphvizPath].insert("LC_FUNCTION_STARTS");
		whitelistedLoadCommandsForPath[graphvizPath].insert("0x00000026");
		whitelistedLoadCommandsForPath[graphvizPath].insert("LC_DATA_IN_CODE");
		whitelistedLoadCommandsForPath[graphvizPath].insert("0x00000029");
		whitelistedLoadCommandsForPath[graphvizPath].insert("LC_SOURCE_VERSION");
		whitelistedLoadCommandsForPath[graphvizPath].insert("0x0000002a");
		whitelistedLoadCommandsForPath[graphvizPath].insert("LC_DYLIB_CODE_SIGN_DRS");
		whitelistedLoadCommandsForPath[graphvizPath].insert("0x0000002b");
		string clangPath = "Vuo.framework/MacOS/Clang/bin/";
		whitelistedLoadCommandsForPath[clangPath].insert("LC_VERSION_MIN_MACOSX");
		whitelistedLoadCommandsForPath[clangPath].insert("0x00000024");
		whitelistedLoadCommandsForPath[clangPath].insert("LC_FUNCTION_STARTS");
		whitelistedLoadCommandsForPath[clangPath].insert("0x00000026");
		whitelistedLoadCommandsForPath[clangPath].insert("LC_DATA_IN_CODE");
		whitelistedLoadCommandsForPath[clangPath].insert("0x00000029");
		string commandlineBinaryPath = "framework/vuo-";
		whitelistedLoadCommandsForPath[commandlineBinaryPath].insert("LC_VERSION_MIN_MACOSX");
		whitelistedLoadCommandsForPath[commandlineBinaryPath].insert("0x00000024");
		whitelistedLoadCommandsForPath[commandlineBinaryPath].insert("LC_FUNCTION_STARTS");
		whitelistedLoadCommandsForPath[commandlineBinaryPath].insert("0x00000026");
		whitelistedLoadCommandsForPath[commandlineBinaryPath].insert("LC_DATA_IN_CODE");
		whitelistedLoadCommandsForPath[commandlineBinaryPath].insert("0x00000029");
		string vuoFrameworkBinaryPath = "Vuo.framework/Vuo";
		whitelistedLoadCommandsForPath[vuoFrameworkBinaryPath].insert("LC_DATA_IN_CODE");
		whitelistedLoadCommandsForPath[vuoFrameworkBinaryPath].insert("0x00000029");
		whitelistedLoadCommandsForPath[vuoFrameworkBinaryPath].insert("LC_SOURCE_VERSION");
		whitelistedLoadCommandsForPath[vuoFrameworkBinaryPath].insert("0x0000002a");
		whitelistedLoadCommandsForPath[vuoFrameworkBinaryPath].insert("LC_DYLIB_CODE_SIGN_DRS");
		whitelistedLoadCommandsForPath[vuoFrameworkBinaryPath].insert("0x0000002b");
		string editorPath = "Vuo Editor.app/Contents/MacOS/Vuo Editor";
		whitelistedLoadCommandsForPath[editorPath].insert("LC_DATA_IN_CODE");
		whitelistedLoadCommandsForPath[editorPath].insert("0x00000029");

		string command = string("otool -l \"") + binaryPath + string("\"");
		vector<string> linesFromCommand;
		getLinesFromCommand(command, linesFromCommand);

		foreach (string line, linesFromCommand)
		{
			// Ignore unknown load commands that are known to be harmless.
			bool isWhitelisted = false;
			for (map<string, set<string> >::iterator i = whitelistedLoadCommandsForPath.begin(); i != whitelistedLoadCommandsForPath.end(); ++i)
				for (set<string>::iterator j = i->second.begin(); j != i->second.end(); ++j)
					if (binaryPath.find(i->first) != string::npos && line.find(*j) != string::npos)
						isWhitelisted = true;
			if (isWhitelisted)
				continue;

			QVERIFY2(line.find("Unknown") == string::npos, (binaryPath + " : " + line).c_str());

			foreach (string loadCommand, blacklistedLoadCommands)
				QVERIFY2(line.find(loadCommand) == string::npos, (binaryPath + " : " + line).c_str());
		}
	}

};

QTEST_APPLESS_MAIN(TestBuiltProducts)
#include "TestBuiltProducts.moc"

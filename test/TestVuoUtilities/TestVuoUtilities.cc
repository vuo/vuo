/**
 * @file
 * TestVuoUtilities implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <Vuo/Vuo.h>

#include <CoreFoundation/CoreFoundation.h>

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(string);

/**
 * Tests for the @c Vuo*Utilities classes.
 */
class TestVuoUtilities : public QObject
{
	Q_OBJECT

private slots:

	void testSplitPath_data()
	{
		QTest::addColumn< QString >("path");
		QTest::addColumn< QString >("expectedDirectory");
		QTest::addColumn< QString >("expectedFile");
		QTest::addColumn< QString >("expectedExtension");

		QTest::newRow("Absolute path.") << "/path/to/file.ext" << "/path/to/" << "file" << "ext";
		QTest::newRow("File.") << "file" << "" << "file" << "";
		QTest::newRow("Directory.") << "dir/" << "dir/" << "" << "";
		QTest::newRow("Directory and file.") << "dir/file" << "dir/" << "file" << "";
		QTest::newRow("Directory and file ending in '.'.") << "dir/file." << "dir/" << "file" << "";
		QTest::newRow("Hidden file. The file name ends up getting parsed as an extension, and that's OK.") << "~/.bashrc" << "~/" << "" << "bashrc";
		QTest::newRow("Relative path with '..'.") << "../../f.e" << "../../" << "f" << "e";
	}
	void testSplitPath()
	{
		QFETCH(QString, path);
		QFETCH(QString, expectedDirectory);
		QFETCH(QString, expectedFile);
		QFETCH(QString, expectedExtension);

		string actualDirectory;
		string actualFile;
		string actualExtension;
		VuoFileUtilities::splitPath(qPrintable(path), actualDirectory, actualFile, actualExtension);

		QCOMPARE(QString(actualDirectory.c_str()), expectedDirectory);
		QCOMPARE(QString(actualFile.c_str()), expectedFile);
		QCOMPARE(QString(actualExtension.c_str()), expectedExtension);
	}

	void testCanonicalPath_data()
	{
		QTest::addColumn< QString >("path");
		QTest::addColumn< QString >("canonicalPath");

		QTest::newRow("Empty path.") << "" << "";
		QTest::newRow("Root directory.") << "/" << "/";
		QTest::newRow("Absolute path, already canonical.") << "/path/to/file.ext" << "/path/to/file.ext";
		QTest::newRow("Relative path, already canonical.") << "path/to/file.ext" << "path/to/file.ext";
		QTest::newRow("Trailing file separator.") << "dir/" << "dir";
		QTest::newRow("Doubled file separators.") << "//path//to//file//" << "/path/to/file";
		QTest::newRow("Tripled file separators.") << "///path///to///file///" << "/path/to/file";
	}
	void testCanonicalPath()
	{
		QFETCH(QString, path);
		QFETCH(QString, canonicalPath);

		string actualCanonicalPath = path.toStdString();
		VuoFileUtilities::canonicalizePath(actualCanonicalPath);

		QCOMPARE(QString::fromStdString(actualCanonicalPath), canonicalPath);
	}

	void testStringEndsWith()
	{
		QVERIFY(VuoStringUtilities::endsWith("",""));
		QVERIFY(VuoStringUtilities::endsWith("a",""));
		QVERIFY(VuoStringUtilities::endsWith("a","a"));
		QVERIFY(VuoStringUtilities::endsWith("aa","a"));
		QVERIFY(!VuoStringUtilities::endsWith("","a"));
		QVERIFY(!VuoStringUtilities::endsWith("a","aa"));
		QVERIFY(!VuoStringUtilities::endsWith("a","b"));
	}

	void testStringReplaceAll_data()
	{
		QTest::addColumn< QString >("wholeString");
		QTest::addColumn< char >("originalChar");
		QTest::addColumn< char >("replacementChar");
		QTest::addColumn< QString >("expectedOutputString");

		QTest::newRow("Single character to replace.") << "vuo_integer" << '_' << '.' << "vuo.integer";
		QTest::newRow("Multiple characters to replace.") << "a.a.aa..aaa" << 'a' << 'b' << "b.b.bb..bbb";
		QTest::newRow("No characters to replace.") << "x" << 'y' << 'z' << "x";
		QTest::newRow("Empty string.") << "" << 'y' << 'z' << "";
		QTest::newRow("Original same as replacement.") << "haha" << 'a' << 'a' << "haha";
	}
	void testStringReplaceAll()
	{
		QFETCH(QString, wholeString);
		QFETCH(char, originalChar);
		QFETCH(char, replacementChar);
		QFETCH(QString, expectedOutputString);

		string actualOutputString = VuoStringUtilities::replaceAll(wholeString.toUtf8().constData(), originalChar, replacementChar);
		QCOMPARE(QString(actualOutputString.c_str()), expectedOutputString);
	}

	void testConvertToCamelCase_data()
	{
		QTest::addColumn<QString>("original");
		QTest::addColumn<bool>("forceFirstLetterToUpper");
		QTest::addColumn<bool>("forceFirstLetterToLower");
		QTest::addColumn<bool>("forceInterveningLettersToLower");
		QTest::addColumn<bool>("allowDot");
		QTest::addColumn<QString>("camelCase");

		QTest::newRow("emptystring")                              << ""                      << false << false << false << false << "";
		QTest::newRow("spaces")                                   << " the cat on the  mat " << false << false << false << false << "theCatOnTheMat";
		QTest::newRow("acronyms, no forcing")                     << "VHS or DVD"            << false << false << false << false << "VHSOrDVD";
		QTest::newRow("acronyms, first upper")                    << "VHS or DVD"            << true  << false << false << false << "VHSOrDVD";
		QTest::newRow("acronyms, first lower")                    << "VHS or DVD"            << false << true  << false << false << "vHSOrDVD";
		QTest::newRow("acronyms, first upper, intervening lower") << "VHS or DVD"            << true  << false << true  << false << "VhsOrDvd";
		QTest::newRow("acronyms, first lower, intervening lower") << "VHS or DVD"            << false << true  << true  << false << "vhsOrDvd";
		QTest::newRow("numbers")                                  << "123 Main Street 45701" << false << true  << false << false << "mainStreet45701";
		QTest::newRow("dots")                                     << "a.b.c"                 << false << true  << false << true  << "a.b.c";
		QTest::newRow("dots, edge")                               << ".a.b.c."               << false << true  << false << true  << "a.b.c";
		QTest::newRow("dots, consecutive")                        << "..a..b..c.."           << false << true  << false << true  << "a.b.c";
	}
	void testConvertToCamelCase()
	{
		QFETCH(QString, original);
		QFETCH(bool, forceFirstLetterToUpper);
		QFETCH(bool, forceFirstLetterToLower);
		QFETCH(bool, forceInterveningLettersToLower);
		QFETCH(bool, allowDot);
		QFETCH(QString, camelCase);

		string converted = VuoStringUtilities::convertToCamelCase(original.toStdString(), forceFirstLetterToUpper, forceFirstLetterToLower, forceInterveningLettersToLower, allowDot);
		QCOMPARE(QString::fromStdString(converted), camelCase);
	}

	void testExpandCamelCase_data()
	{
		QTest::addColumn< QString >("inputString");
		QTest::addColumn< QString >("expectedOutputString");

		QTest::newRow("emptystring") << "" << "";
		QTest::newRow("camel") << "camel" << "Camel";
		QTest::newRow("camelCase") << "camelCase" << "Camel Case";
		QTest::newRow("camelCaseString") << "camelCaseString" << "Camel Case String";
		QTest::newRow("ShowWindowsOn2Screens") << "ShowWindowsOn2Screens" << "Show Windows On 2 Screens";
		QTest::newRow("xylophone")             << "xylophone"             << "Xylophone";
		QTest::newRow("xy")                    << "xy"                    << "XY";
		QTest::newRow("rgb")                   << "rgb"                   << "RGB";
		QTest::newRow("rgbMinimum")            << "rgbMinimum"            << "RGB Minimum";
	}
	void testExpandCamelCase()
	{
		QFETCH(QString, inputString);
		QFETCH(QString, expectedOutputString);

		string actualOutputString = VuoStringUtilities::expandCamelCase(inputString.toUtf8().constData());
		QCOMPARE(QString(actualOutputString.c_str()), expectedOutputString);
	}

	void testMakeFromCFString_data()
	{
		QTest::addColumn<void *>("cfString");
		QTest::addColumn<string>("expectedStdString");

		QTest::newRow("null")        << (void *)nullptr     << string("");
		QTest::newRow("emptystring") << (void *)CFSTR("")   << string("");
		QTest::newRow("ascii")       << (void *)CFSTR("hi") << string("hi");
		QTest::newRow("utf8")        << (void *)CFSTR("å")  << string("å");
		QTest::newRow("utf8mb4")     << (void *)CFSTR("💯") << string("💯");
	}
	void testMakeFromCFString()
	{
		QFETCH(void *, cfString);
		QFETCH(string, expectedStdString);

		string actualStdString = VuoStringUtilities::makeFromCFString((CFStringRef)cfString);
		QCOMPARE(actualStdString, expectedStdString);
	}

	void testFindFilesInDirectory_data()
	{
		QTest::addColumn< QString >("dirPath");
		QTest::addColumn< QString >("extension");
		QTest::addColumn< QString >("fileRelativePath");
		QTest::addColumn< bool >("shouldFind");

		QTest::newRow("File in top level of directory.") << "." << "cc" << "TestVuoUtilities.cc" << true;
		QTest::newRow("File below top level of directory.") << ".." << "cc" << "TestVuoUtilities/TestVuoUtilities.cc" << true;
		QTest::newRow("File with wrong extension.") << "." << "c" << "TestVuoUtilities.cc" << false;
		QTest::newRow("Find files, not directories.") << "." << "fs" << "directory-with-extension.fs" << false;
	}
	void testFindFilesInDirectory()
	{
		QFETCH(QString, dirPath);
		QFETCH(QString, extension);
		QFETCH(QString, fileRelativePath);
		QFETCH(bool, shouldFind);

		set<string> extensions;
		extensions.insert(extension.toStdString());
		set<VuoFileUtilities::File *> files = VuoFileUtilities::findFilesInDirectory(dirPath.toUtf8().constData(), extensions);
		set<string> fileRelativePaths;
		foreach (VuoFileUtilities::File *file, files)
		{
			fileRelativePaths.insert( file->getRelativePath() );
			delete file;
		}
		QEXPECT_FAIL("File below top level of directory.", "Known bug: findFilesInDirectory does not search recursively", Continue);
		QVERIFY((fileRelativePaths.find(fileRelativePath.toUtf8().constData()) != fileRelativePaths.end()) == shouldFind);
	}

	void testFindAllFilesInDirectory_data()
	{
		QTest::addColumn<QString>("path");
		QTest::addColumn<QSet<QString>>("expectedFiles");

		QString dir(getcwd(nullptr, 0) + QStringLiteral("/aliases-directory"));

		QTest::newRow("Empty directory")                           << "empty-directory"                     << QSet<QString>();
		QTest::newRow("Plain directory with a single file")        << "aliases-directory/targetDir"         << QSet<QString>({"aliases-directory/targetDir/someFile"});
		QTest::newRow("Symlink to a directory with a single file") << "aliases-directory/targetDir symlink" << QSet<QString>({"aliases-directory/targetDir symlink/someFile"});
		QTest::newRow("Alias to a directory with a single file")   << "aliases-directory/targetDir alias"   << QSet<QString>({dir + "/targetDir/someFile"});
	}
	void testFindAllFilesInDirectory()
	{
		QFETCH(QString, path);
		QFETCH(QSet<QString>, expectedFiles);

		if (string(QTest::currentDataTag()) == "Empty directory")
			unlink("empty-directory/.gitignore");

		auto foundFilesV = VuoFileUtilities::findAllFilesInDirectory(path.toStdString(), set<string>(), true);

		QSet<QString> foundFiles;
		for (auto file : foundFilesV)
			foundFiles << QString::fromStdString(file->path());

		QCOMPARE(foundFiles, expectedFiles);

		if (string(QTest::currentDataTag()) == "Empty directory")
			VuoFileUtilities::writeStringToFile("", "empty-directory/.gitignore");
	}

	void testCopyDirectory_data()
	{
		QTest::addColumn<QString>("sourcePath");
		QTest::addColumn<QSet<QString>>("expectedFiles");
		{
			QSet<QString> expectedFiles;
			QTest::newRow("Empty directory")
				<< "empty-directory"
				<< expectedFiles;
		}

		{
			QSet<QString> expectedFiles;
			expectedFiles << "Contents/Resources/widget.png";
			QTest::newRow("Single deep file")
				<< "../TestCompositions/composition/ListFiles_folder/folder2/Calculator.app"
				<< expectedFiles;
		}

		{
			QSet<QString> expectedFiles;
			expectedFiles << "scene3.m3";
			expectedFiles << "image3.tiff";
			expectedFiles << "movie3.h264.mov";
			expectedFiles << "folder4/scene4.md3";
			expectedFiles << "folder4/movie4.quicktime.mov";
			expectedFiles << "folder4/folder5/scene5.ac3d";
			QTest::newRow("Directory with files and subdirectories")
				<< "../TestCompositions/composition/ListFiles_folder/folder2/folder3"
				<< expectedFiles;
		}

		{
			QSet<QString> expectedFiles;
			expectedFiles << "a";
			expectedFiles << "b/a2->";
			expectedFiles << "b/nonexistent->";
			QTest::newRow("Directory with symlinks")
				<< "symlinks-directory"
				<< expectedFiles;
		}
	}
	void testCopyDirectory()
	{
		QFETCH(QString, sourcePath);
		QFETCH(QSet<QString>, expectedFiles);

		if (string(QTest::currentDataTag()) == "Empty directory")
			unlink("empty-directory/.gitignore");

		string destPath =  VuoFileUtilities::makeTmpDir("testCopyDirectory");
		VuoFileUtilities::copyDirectory(sourcePath.toStdString(), destPath);

		auto copiedFiles = VuoFileUtilities::findAllFilesInDirectory(destPath, set<string>(), true);
		QSet<QString> copiedFilesRelative;
		for (auto file : copiedFiles)
			copiedFilesRelative << QString::fromStdString(VuoStringUtilities::substrAfter(file->path(), destPath + "/"))
				+ (VuoFileUtilities::isSymlink(file->path()) ? "->" : "");

		QDir(QString::fromStdString(destPath)).removeRecursively();

		QCOMPARE(copiedFilesRelative, expectedFiles);

		if (string(QTest::currentDataTag()) == "Empty directory")
			VuoFileUtilities::writeStringToFile("", "empty-directory/.gitignore");
	}

	void testLockFile_data()
	{
		QTest::addColumn<int>("testNum");

		int testNum = 0;
		QTest::newRow("Process locks for reading multiple times.") << testNum++;
		QTest::newRow("Process locks for reading and writing multiple times.") << testNum++;
		QTest::newRow("Process locks for reading with different file descriptors.") << testNum++;
		QTest::newRow("Process A locks for reading. Process B can lock for reading.") << testNum++;
		QTest::newRow("Process A locks for reading. Process B can't lock for writing.") << testNum++;
		QTest::newRow("Process A locks for writing. Process B can lock for reading after Process A downgrades its lock to reading.") << testNum++;
		QTest::newRow("Process B locks for writing. Process A can lock for reading and writing after Process B terminates.") << testNum++;
	}
	void testLockFile()
	{
		QFETCH(int, testNum);

		string path = VuoFileUtilities::makeTmpFile("TestVuoUtilities", "lock");
		VuoFileUtilities::File parentFile("", path);

		if (testNum == 0)
		{
			// Process locks for reading multiple times.

			bool gotLock;
			for (int nonBlocking = 0; nonBlocking < 2; ++nonBlocking)
			{
				for (int i = 0; i < 5; ++i)
				{
					gotLock = parentFile.lockForReading(nonBlocking);
					QVERIFY(gotLock);
				}
			}
		}
		else if (testNum == 1)
		{
			// Process locks for reading and writing multiple times.

			bool gotLock;
			for (int nonBlocking = 0; nonBlocking < 2; ++nonBlocking)
			{
				for (int i = 0; i < 5; ++i)
				{
					gotLock = parentFile.lockForReading(nonBlocking);
					QVERIFY(gotLock);
					gotLock = parentFile.lockForWriting(nonBlocking);
					QVERIFY(gotLock);
				}
			}
		}
		else if (testNum == 2)
		{
			// Process locks for reading with different PIDs.

			VuoFileUtilities::File otherParentFile("", path);
			bool origGotLock = parentFile.lockForReading(true);
			QVERIFY(origGotLock);
			bool otherGotLock = otherParentFile.lockForWriting(true);
			QEXPECT_FAIL("", "Not supported by flock", Continue);
			QVERIFY(otherGotLock);
		}
		else if (testNum == 3)
		{
			// Process A locks for reading. Process B can lock for reading.

			for (int nonBlocking = 0; nonBlocking < 2; ++nonBlocking)
			{
				bool parentGotLock = parentFile.lockForReading(nonBlocking);
				QVERIFY(parentGotLock);

				pid_t pid = fork();
				QVERIFY(pid >= 0);

				if (pid == 0)
				{
					VuoFileUtilities::File childFile("", path);
					bool childGotLock = childFile.lockForReading(nonBlocking);
					QVERIFY(childGotLock);
					_exit(0);
				}
			}
		}
		else if (testNum == 4)
		{
			// Process A locks for reading. Process B can't lock for writing.

			for (int nonBlocking = 0; nonBlocking < 2; ++nonBlocking)
			{
				bool parentGotLock = parentFile.lockForReading(nonBlocking);
				QVERIFY(parentGotLock);

				pid_t pid = fork();
				QVERIFY(pid >= 0);

				if (pid == 0)
				{
					VuoFileUtilities::File childFile("", path);
					bool childGotLock = childFile.lockForWriting(true);
					QVERIFY(! childGotLock);
					_exit(0);
				}
			}
		}
		else if (testNum == 5)
		{
			// Process A locks for writing. Process B can lock for reading after Process A downgrades its lock to reading.

			bool parentGotLock = parentFile.lockForWriting();
			QVERIFY(parentGotLock);

			pid_t pid = fork();
			QVERIFY(pid >= 0);

			if (pid == 0)
			{
				VuoFileUtilities::File childFile("", path);
				bool childGotLock = childFile.lockForReading(true);
				QVERIFY(! childGotLock);
				sleep(2);
				childGotLock = childFile.lockForReading();
				QVERIFY(childGotLock);
				_exit(0);
			}
			else
			{
				sleep(1);
				parentGotLock = parentFile.lockForReading();
				QVERIFY(parentGotLock);
			}
		}
		else if (testNum == 6)
		{
			// Process B locks for writing. Process A can lock for reading and writing after Process B terminates.

			pid_t pid = fork();
			QVERIFY(pid >= 0);

			if (pid == 0)
			{
				VuoFileUtilities::File childFile("", path);
				bool childGotLock = childFile.lockForWriting();
				QVERIFY(childGotLock);
				sleep(2);
				_exit(0);
			}
			else
			{
				sleep(1);
				bool parentGotLock = parentFile.lockForReading(true);
				QVERIFY(! parentGotLock);
				sleep(2);
				parentGotLock = parentFile.lockForReading();
				QVERIFY(parentGotLock);
				parentGotLock = parentFile.lockForWriting();
				QVERIFY(parentGotLock);
			}
		}
	}

	void testTranscodeToIdentifier_data()
	{
		QTest::addColumn<QString>("originalString");
		QTest::addColumn<QString>("transcodedString");

		QTest::newRow("identifier") << "azAZ09_" << "azAZ09_";
		QTest::newRow(".") << ".a.b..c." << "_a_b__c_";
		QTest::newRow("/") << "/A/B//C/" << "__A__B____C__";
		QTest::newRow("UTF-8") << "⓪x①y②③z④" << "xyz";
	}
	void testTranscodeToIdentifier()
	{
		QFETCH(QString, originalString);
		QFETCH(QString, transcodedString);

		QCOMPARE(QString::fromStdString(VuoStringUtilities::transcodeToIdentifier(originalString.toStdString())), transcodedString);
	}

	void testGraphvizEscaping_data()
	{
		QTest::addColumn<QString>("originalString");
		QTest::addColumn<QString>("escapedString");
		QTest::addColumn<bool>("testEscaping");
		QTest::addColumn<bool>("testUnescaping");

		QTest::newRow("lessthan") << "a<b" << "a\\<b" << true << true;
		QTest::newRow("pipe") << "a|b" << "a\\|b" << true << true;
		QTest::newRow("greaterthan") << "a>b" << "a\\>b" << true << true;
		QTest::newRow("backslash") << "a\\b" << "a\\\\b" << true << true;
		QTest::newRow("doublequoted backslash") << "\"\\\"" << "\\\"\\\\\\\"" << true << true;

		// Graphviz's agget() returns strings with the double-quotes unescaped, but leaves everything else escaped.  Make sure we handle that situation.
		QTest::newRow("agget()") << "\"\\\\\"" << "\"\\\\\\\\\"" << false << true;

		// https://b33p.net/kosada/node/15336
		QTest::newRow("consecutive spaces") << "    code singlespace doublespace  triplespace   trailingspaces    " << " \\  \\ code singlespace doublespace \\ triplespace \\  trailingspaces \\  \\ " << true << true;
	}
	void testGraphvizEscaping()
	{
		QFETCH(QString, originalString);
		QFETCH(QString, escapedString);
		QFETCH(bool, testEscaping);
		QFETCH(bool, testUnescaping);

		if (testEscaping)
			QCOMPARE(QString::fromStdString(VuoStringUtilities::transcodeToGraphvizIdentifier(originalString.toStdString())), escapedString);
		if (testUnescaping)
			QCOMPARE(QString::fromStdString(VuoStringUtilities::transcodeFromGraphvizIdentifier(escapedString.toStdString())), originalString);
	}

	void testUniqueIdentifier_data()
	{
		QTest::addColumn<QString>("preferredIdentifier");
		QTest::addColumn<QString>("identifierPrefix");
		QTest::addColumn<QStringList>("takenIdentifiers");
		QTest::addColumn<QString>("expectedIdentifier");

		QString preferredIdentifier("TestIdentifier999");
		QString identifierPrefix("TestIdentifier");
		QStringList takenIdentifiers;

		QTest::newRow("preferred available") << preferredIdentifier << identifierPrefix << takenIdentifiers << preferredIdentifier;

		takenIdentifiers.append(preferredIdentifier);
		QTest::newRow("preferred taken") << preferredIdentifier << identifierPrefix << takenIdentifiers << identifierPrefix + "2";

		takenIdentifiers.append(identifierPrefix + "2");
		QTest::newRow("preferred and first suffix taken") << preferredIdentifier << identifierPrefix << takenIdentifiers << identifierPrefix + "3";

		for (int i = 3; i <= 9; ++i)
			takenIdentifiers.append( QString(identifierPrefix + "%1").arg(i) );
		QTest::newRow("preferred and single-digit suffixes taken") << preferredIdentifier << identifierPrefix << takenIdentifiers << identifierPrefix + "10";
	}
	void testUniqueIdentifier()
	{
		QFETCH(QString, preferredIdentifier);
		QFETCH(QString, identifierPrefix);
		QFETCH(QStringList, takenIdentifiers);
		QFETCH(QString, expectedIdentifier);

		auto isIdentifierAvailable = [&takenIdentifiers] (const string &identifier)
		{
			return (! takenIdentifiers.contains( QString::fromStdString(identifier) ));
		};

		string uniqueUsingLambda = VuoStringUtilities::formUniqueIdentifier(isIdentifierAvailable,
																			preferredIdentifier.toStdString(), identifierPrefix.toStdString());
		QCOMPARE(QString::fromStdString(uniqueUsingLambda), expectedIdentifier);

		set<string> takenIdentifiersSet;
		foreach (QString s, takenIdentifiers)
			takenIdentifiersSet.insert(s.toStdString());

		string uniqueUsingSet = VuoStringUtilities::formUniqueIdentifier(takenIdentifiersSet,
																		 preferredIdentifier.toStdString(), identifierPrefix.toStdString());
		QCOMPARE(QString::fromStdString(uniqueUsingSet), expectedIdentifier);
		QVERIFY(takenIdentifiersSet.find(uniqueUsingSet) != takenIdentifiersSet.end());
	}

	void testMarkdown()
	{
		QCOMPARE(VuoStringUtilities::generateHtmlFromMarkdown("   - ***Ka-pow!***"), string("<ul>\n<li> <strong><em>Ka-pow!</em></strong></li>\n</ul>\n"));
	}

	void testFileExists_data()
	{
		QTest::addColumn<bool>("expectedFileExists");
		QTest::addColumn<bool>("expectedIsReadable");
		QTest::addColumn<bool>("expectedContainsReadableData");
		QTest::addColumn<bool>("expectedDirExists");
		QTest::addColumn<bool>("expectedIsSymlink");
		QTest::addColumn<bool>("expectedIsMacAlias");

		QString dir(getcwd(nullptr, 0) + QStringLiteral("/aliases-directory"));
		//                                                              exists   readable data     dir      symlink  alias
		QTest::newRow("")                                            << false << false << false << false << false << false;  // Emptystring (nonexistent)
		QTest::newRow((dir + "/blah").toUtf8().data())               << false << false << false << false << false << false;  // Nonexistent
		QTest::newRow((dir + "/targetFile").toUtf8().data())         << true  << true  << true  << false << false << false;  // A file
		QTest::newRow((dir + "/targetFile symlink").toUtf8().data()) << true  << true  << true  << false << true  << false;  // A symlink to a file
		QTest::newRow((dir + "/targetFile alias").toUtf8().data())   << true  << true  << true  << false << false << true ;  // A macOS Alias to a file
		QTest::newRow((dir + "/targetDir").toUtf8().data())          << true  << true  << false << true  << false << false;  // A dir
		QTest::newRow((dir + "/targetDir symlink").toUtf8().data())  << true  << true  << false << true  << true  << false;  // A symlink to a dir
		QTest::newRow((dir + "/targetDir alias").toUtf8().data())    << true  << true  << false << true  << false << true ;  // A macOS Alias to a dir
	}
	void testFileExists()
	{
		QFETCH(bool, expectedFileExists);
		QFETCH(bool, expectedIsReadable);
		QFETCH(bool, expectedContainsReadableData);
		QFETCH(bool, expectedDirExists);
		QFETCH(bool, expectedIsSymlink);
		QFETCH(bool, expectedIsMacAlias);

		string path = QTest::currentDataTag();
		QCOMPARE(VuoFileUtilities::fileExists(path), expectedFileExists);
		QCOMPARE(VuoFileUtilities::fileIsReadable(path), expectedIsReadable);
		QCOMPARE(VuoFileUtilities::fileContainsReadableData(path), expectedContainsReadableData);
		QCOMPARE(VuoFileUtilities::dirExists(path),  expectedDirExists);
		QCOMPARE(VuoFileUtilities::isSymlink(path),  expectedIsSymlink);
		QCOMPARE(VuoFileUtilitiesCocoa_isMacAlias(path),  expectedIsMacAlias);
	}

	void testFileTimes()
	{
		auto compareTimes = [](QString description, double actual, double expected)
		{
			double tolerance = 0.05;
			QVERIFY2(fabs(actual - expected) < tolerance,
					 QString("%1: %2 should be within %3 of %4").arg(description).arg(actual, 0, 'f').arg(tolerance).arg(expected, 0, 'f').toStdString().c_str());
		};

		useconds_t waitMicroseconds = USEC_PER_SEC/5;
		double waitSeconds = waitMicroseconds/(double)USEC_PER_SEC;

		// Create a directory.
		string dirPath = VuoFileUtilities::makeTmpDir("TestVuoUtilities-testFileTimes");
		double justAfterDirCreated = VuoTimeUtilities::getCurrentTimeInSeconds();

		compareTimes("created dir (dir modified)", VuoFileUtilities::getFileLastModifiedInSeconds(dirPath), justAfterDirCreated);
		compareTimes("created dir (dir accessed)", VuoFileUtilities::getSecondsSinceFileLastAccessed(dirPath), 0);

		// Wait a moment for the time since last accessed to tick up.
		usleep(waitMicroseconds);

		compareTimes("waited (dir modified)", VuoFileUtilities::getFileLastModifiedInSeconds(dirPath), justAfterDirCreated);
		compareTimes("waited (dir accessed)", VuoFileUtilities::getSecondsSinceFileLastAccessed(dirPath), waitSeconds);

		// Create a file within the directory.
		usleep(waitMicroseconds);
		string filePath = dirPath + "/file";
		VuoFileUtilities::writeStringToFile("test", filePath);
		double justAfterFileCreated = VuoTimeUtilities::getCurrentTimeInSeconds();

		compareTimes("created file (file modified)", VuoFileUtilities::getFileLastModifiedInSeconds(filePath), justAfterFileCreated);
		compareTimes("created file (file accessed)", VuoFileUtilities::getSecondsSinceFileLastAccessed(filePath), 0);
		compareTimes("created file (dir modified)", VuoFileUtilities::getFileLastModifiedInSeconds(dirPath), justAfterFileCreated);
		compareTimes("created file (dir accessed)", VuoFileUtilities::getSecondsSinceFileLastAccessed(dirPath), 0);

		// Read the file.
		usleep(waitMicroseconds);
		string s = VuoFileUtilities::readFileToString(filePath);
		QVERIFY(! s.empty());

		compareTimes("read file (file modified)", VuoFileUtilities::getFileLastModifiedInSeconds(filePath), justAfterFileCreated);
		compareTimes("read file (file accessed)", VuoFileUtilities::getSecondsSinceFileLastAccessed(filePath), 0);
		compareTimes("read file (dir modified)", VuoFileUtilities::getFileLastModifiedInSeconds(dirPath), justAfterFileCreated);
		compareTimes("read file (dir accessed)", VuoFileUtilities::getSecondsSinceFileLastAccessed(dirPath), waitSeconds);

		// Write to the file.
		usleep(waitMicroseconds);
		VuoFileUtilities::writeStringToFile("test", filePath);
		double justAfterFileWritten = VuoTimeUtilities::getCurrentTimeInSeconds();

		compareTimes("wrote file (file modified)", VuoFileUtilities::getFileLastModifiedInSeconds(filePath), justAfterFileWritten);
		compareTimes("wrote file (file accessed)", VuoFileUtilities::getSecondsSinceFileLastAccessed(filePath), 0);
		compareTimes("wrote file (dir modified)", VuoFileUtilities::getFileLastModifiedInSeconds(dirPath), justAfterFileCreated);
		compareTimes("wrote file (dir accessed)", VuoFileUtilities::getSecondsSinceFileLastAccessed(dirPath), 2*waitSeconds);

		// Delete the file.
		usleep(waitMicroseconds);
		VuoFileUtilities::deleteFile(filePath);
		double justAfterFileDeleted = VuoTimeUtilities::getCurrentTimeInSeconds();

		compareTimes("deleted file (dir modified)", VuoFileUtilities::getFileLastModifiedInSeconds(dirPath), justAfterFileDeleted);
		compareTimes("deleted file (dir accessed)", VuoFileUtilities::getSecondsSinceFileLastAccessed(dirPath), 0);

		VuoFileUtilities::deleteDir(dirPath);
	}
};

QTEST_APPLESS_MAIN(TestVuoUtilities)

#include "TestVuoUtilities.moc"

/**
 * @file
 * TestVuoUtilities implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <QtCore/QString>
#include <QtTest/QtTest>

#include "VuoFileUtilities.hh"
#include "VuoStringUtilities.hh"

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

	void testExpandCamelCase_data()
	{
		QTest::addColumn< QString >("inputString");
		QTest::addColumn< QString >("expectedOutputString");

		QTest::newRow("emptystring") << "" << "";
		QTest::newRow("camel") << "camel" << "Camel";
		QTest::newRow("camelCase") << "camelCase" << "Camel Case";
		QTest::newRow("camelCaseString") << "camelCaseString" << "Camel Case String";
		QTest::newRow("ShowWindowsOn2Screens") << "ShowWindowsOn2Screens" << "Show Windows On 2 Screens";
	}
	void testExpandCamelCase()
	{
		QFETCH(QString, inputString);
		QFETCH(QString, expectedOutputString);

		string actualOutputString = VuoStringUtilities::expandCamelCase(inputString.toUtf8().constData());
		QCOMPARE(QString(actualOutputString.c_str()), expectedOutputString);
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

	void testGraphvizEscaping_data()
	{
		QTest::addColumn<QString>("originalString");
		QTest::addColumn<QString>("escapedString");
		QTest::addColumn<bool>("testEscaping");
		QTest::addColumn<bool>("testUnescaping");

		QTest::newRow("<") << "a<b" << "a\\<b" << true << true;
		QTest::newRow("|") << "a|b" << "a\\|b" << true << true;
		QTest::newRow(">") << "a>b" << "a\\>b" << true << true;
		QTest::newRow("\\") << "a\\b" << "a\\\\b" << true << true;
		QTest::newRow("\"\\\"") << "\"\\\"" << "\\\"\\\\\\\"" << true << true;

		// Graphviz's agget() returns strings with the double-quotes unescaped, but leaves everything else escaped.  Make sure we handle that situation.
		QTest::newRow("agget()") << "\"\\\\\"" << "\"\\\\\\\\\"" << false << true;
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

	void testMarkdown()
	{
		QCOMPARE(VuoStringUtilities::generateHtmlFromMarkdown("   - ***Ka-pow!***"), string("<ul>\n<li> <strong><em>Ka-pow!</em></strong></li>\n</ul>\n"));
	}
};

QTEST_APPLESS_MAIN(TestVuoUtilities)

#include "TestVuoUtilities.moc"

/**
 * @file
 * TestVuoUrl implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoUrlFetch.h"
}

/**
 * Tests the VuoUrl type.
 */
class TestVuoUrl : public QObject
{
	Q_OBJECT

private slots:
	void initTestCase()
	{
		VuoHeap_init();
	}

	void testNormalize_data()
	{
		QTest::addColumn<QString>("url");
		QTest::addColumn<QString>("expectedNormalizedUrl");
		QTest::addColumn<bool>("expectedValidPosixPath");
		QTest::addColumn<QString>("expectedPosixPath");

		QString fileScheme = "file://";
		QString basePath = getcwd(NULL,0);
		QString baseUrl = fileScheme + basePath;
		QString homeDir = getenv("HOME");

		QTest::newRow("empty string")		<< ""						<< baseUrl								<< true		<< basePath;
		QTest::newRow("absolute URL")		<< "http://vuo.org"			<< "http://vuo.org"						<< false	<< "";
		QTest::newRow("absolute URL/")		<< "http://vuo.org/"		<< "http://vuo.org"						<< false	<< "";
		QTest::newRow("relative file")		<< "file"					<< baseUrl + "/file"					<< true		<< basePath + "/file";
		QTest::newRow("relative file ?")	<< "file?.wav"				<< baseUrl + "/file%3f.wav"				<< true		<< basePath + "/file?.wav";
		QTest::newRow("relative dir/")		<< "dir/"					<< baseUrl + "/dir"						<< true		<< basePath + "/dir";
		QTest::newRow("relative dir/file")	<< "dir/file"				<< baseUrl + "/dir/file"				<< true		<< basePath + "/dir/file";
		QTest::newRow("absolute file")		<< "/mach_kernel"			<< fileScheme + "/mach_kernel"			<< true		<< "/mach_kernel";
		QTest::newRow("absolute dir/")		<< "/usr/include/"			<< fileScheme + "/usr/include"			<< true		<< "/usr/include";
		QTest::newRow("absolute dir/file")	<< "/usr/include/stdio.h"	<< fileScheme + "/usr/include/stdio.h"	<< true		<< "/usr/include/stdio.h";
		QTest::newRow("user homedir")		<< "~"						<< fileScheme + homeDir					<< true		<< homeDir;
		QTest::newRow("user homedir/")		<< "~/"						<< fileScheme + homeDir					<< true		<< homeDir;
		QTest::newRow("user homedir/file")	<< "~/.DS_Store"			<< fileScheme + homeDir + "/.DS_Store"	<< true		<< homeDir + "/.DS_Store";
	}
	void testNormalize()
	{
		QFETCH(QString, url);
		QFETCH(QString, expectedNormalizedUrl);
		QFETCH(bool, expectedValidPosixPath);
		QFETCH(QString, expectedPosixPath);

		VuoUrl normalizedUrl = VuoUrl_normalize(url.toUtf8().data(), false);
		VuoRetain(normalizedUrl);
		QCOMPARE(normalizedUrl, expectedNormalizedUrl.toUtf8().data());

		VuoText posixPath = VuoUrl_getPosixPath(normalizedUrl);
		VuoRetain(posixPath);

		if (expectedValidPosixPath)
			QCOMPARE(posixPath, expectedPosixPath.toUtf8().data());
		else
			QCOMPARE(posixPath, (VuoText)NULL);

		VuoRelease(posixPath);
		VuoRelease(normalizedUrl);
	}
};

QTEST_APPLESS_MAIN(TestVuoUrl)

#include "TestVuoUrl.moc"

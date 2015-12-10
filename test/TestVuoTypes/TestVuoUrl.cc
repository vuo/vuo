/**
 * @file
 * TestVuoUrl implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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
		QTest::addColumn<bool>("shouldEscapeSpaces");
		QTest::addColumn<QString>("expectedNormalizedUrl");

		QString fileScheme = "file://";
		QString baseUrl = fileScheme + getcwd(NULL,0);
		QString homeDir = getenv("HOME");

		QTest::newRow("empty string")		<< ""						<< false << baseUrl;
		QTest::newRow("absolute URL")		<< "http://vuo.org"			<< false << "http://vuo.org";
		QTest::newRow("absolute URL/")		<< "http://vuo.org/"		<< false << "http://vuo.org";
		QTest::newRow("relative file")		<< "file"					<< false << baseUrl + "/file";
		QTest::newRow("relative dir/")		<< "dir/"					<< false << baseUrl + "/dir";
		QTest::newRow("relative dir/file")	<< "dir/file"				<< false << baseUrl + "/dir/file";
		QTest::newRow("absolute file")		<< "/mach_kernel"			<< false << fileScheme + "/mach_kernel";
		QTest::newRow("absolute dir/")		<< "/usr/include/"			<< false << fileScheme + "/usr/include";
		QTest::newRow("absolute dir/file")	<< "/usr/include/stdio.h"	<< false << fileScheme + "/usr/include/stdio.h";
		QTest::newRow("user homedir")		<< "~"						<< false << fileScheme + homeDir;
		QTest::newRow("user homedir/")		<< "~/"						<< false << fileScheme + homeDir;
		QTest::newRow("user homedir/file")	<< "~/.DS_Store"			<< false << fileScheme + homeDir + "/.DS_Store";
	}
	void testNormalize()
	{
		QFETCH(QString, url);
		QFETCH(bool, shouldEscapeSpaces);
		QFETCH(QString, expectedNormalizedUrl);

		VuoText normalizedUrl = VuoUrl_normalize(url.toUtf8().data(), shouldEscapeSpaces);
		VuoRetain(normalizedUrl);
		QCOMPARE(normalizedUrl, expectedNormalizedUrl.toUtf8().data());
		VuoRelease(normalizedUrl);
	}
};

QTEST_APPLESS_MAIN(TestVuoUrl)

#include "TestVuoUrl.moc"

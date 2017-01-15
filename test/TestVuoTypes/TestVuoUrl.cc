/**
 * @file
 * TestVuoUrl implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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
		QTest::newRow("absolute URL space")	<< "http://vuo.org/a b.png"	<< "http://vuo.org/a%20b.png"			<< false	<< "";
		QTest::newRow("relative file")		<< "file"					<< baseUrl + "/file"					<< true		<< basePath + "/file";
		QTest::newRow("relative file ?")	<< "file?.wav"				<< baseUrl + "/file%3f.wav"				<< true		<< basePath + "/file?.wav";
		QTest::newRow("relative dir/")		<< "dir/"					<< baseUrl + "/dir"						<< true		<< basePath + "/dir";
		QTest::newRow("relative dir/file")	<< "dir/file"				<< baseUrl + "/dir/file"				<< true		<< basePath + "/dir/file";
		QTest::newRow("absolute file")		<< "/mach_kernel"			<< fileScheme + "/mach_kernel"			<< true		<< "/mach_kernel";
		QTest::newRow("absolute dir/")		<< "/usr/include/"			<< fileScheme + "/usr/include"			<< true		<< "/usr/include";
		QTest::newRow("absolute dir space")	<< "/Library/Desktop Pictures" << "file:///Library/Desktop%20Pictures" << true	<< "/Library/Desktop Pictures";
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
		{
			QCOMPARE(posixPath, expectedPosixPath.toUtf8().data());

			VuoText escapedPosixPath = VuoUrl_escapePosixPath(expectedPosixPath.toUtf8().data());
			QCOMPARE(QString("file://") + escapedPosixPath, QString(expectedNormalizedUrl));
		}
		else
			QCOMPARE(posixPath, (VuoText)NULL);

		{
			VuoText scheme;
			VuoText user;
			VuoText host;
			VuoInteger port;
			VuoText path;
			VuoText query;
			VuoText fragment;
			// Just make sure it doesn't crash.
			QVERIFY(VuoUrl_getParts(normalizedUrl, &scheme, &user, &host, &port, &path, &query, &fragment));
		}

		VuoRelease(posixPath);
		VuoRelease(normalizedUrl);
	}

	void testParts_data()
	{
		QTest::addColumn<QString>("url");
		QTest::addColumn<QString>("expectedScheme");
		QTest::addColumn<QString>("expectedUser");
		QTest::addColumn<QString>("expectedHost");
		QTest::addColumn<int>("expectedPort");
		QTest::addColumn<QString>("expectedPath");
		QTest::addColumn<QString>("expectedQuery");
		QTest::addColumn<QString>("expectedFragment");

		QTest::newRow("http")				<< "http://example.com"											<< "http"	<< ""		<< "example.com"	<< 80	<< ""													<< ""			<< "";
		QTest::newRow("http numeric")		<< "http://127.0.0.1"											<< "http"	<< ""		<< "127.0.0.1"		<< 80	<< ""													<< ""			<< "";
		QTest::newRow("http explicit port")	<< "http://example.com:8080"									<< "http"	<< ""		<< "example.com"	<< 8080	<< ""													<< ""			<< "";
		QTest::newRow("http everything")	<< "http://user@example.com:8080/a?b=c&d=e#f"					<< "http"	<< "user"	<< "example.com"	<< 8080	<< "/a"													<< "b=c&d=e"	<< "f";
		QTest::newRow("https")				<< "https://example.com/"										<< "https"	<< ""		<< "example.com"	<< 443	<< "/"													<< ""			<< "";
		QTest::newRow("file")				<< "file:///System/Library/CoreServices/SystemVersion.plist"	<< "file"	<< ""		<< ""				<< 0	<< "/System/Library/CoreServices/SystemVersion.plist"	<< ""			<< "";
	}
	void testParts()
	{
		QFETCH(QString, url);
		QFETCH(QString, expectedScheme);
		QFETCH(QString, expectedUser);
		QFETCH(QString, expectedHost);
		QFETCH(int, expectedPort);
		QFETCH(QString, expectedPath);
		QFETCH(QString, expectedQuery);
		QFETCH(QString, expectedFragment);

		VuoText scheme;
		VuoText user;
		VuoText host;
		VuoInteger port;
		VuoText path;
		VuoText query;
		VuoText fragment;
		QVERIFY(VuoUrl_getParts(url.toUtf8().data(), &scheme, &user, &host, &port, &path, &query, &fragment));

		QCOMPARE(QString(scheme), expectedScheme);
		QCOMPARE(QString(user), expectedUser);
		QCOMPARE(QString(host), expectedHost);
		QCOMPARE(port, expectedPort);
		QCOMPARE(QString(path), expectedPath);
		QCOMPARE(QString(query), expectedQuery);
		QCOMPARE(QString(fragment), expectedFragment);
	}

	void testFileParts_data()
	{
		QTest::addColumn<QString >("url");
		QTest::addColumn<QString >("expectedPath");
		QTest::addColumn<QString >("expectedFolder");
		QTest::addColumn<QString >("expectedFilename");
		QTest::addColumn<QString >("expectedExtension");

		QTest::newRow("no path")			<< "file://"							<< ""							<< "" << "" << "";
		QTest::newRow("root")				<< "file:///"							<< "/"							<< "/" << "" << "";
		QTest::newRow("root folder")		<< "file:///Applications/"				<< "/Applications/"				<< "/Applications/" << "" << "";
		QTest::newRow("root file")			<< "file:///somefile"					<< "/somefile"					<< "/" << "somefile" << "";
		QTest::newRow("root file ext")		<< "file:///command.com"				<< "/command.com"				<< "/" << "command" << "com";
		QTest::newRow("root dotfile")		<< "file:///.DS_Store"					<< "/.DS_Store"					<< "/" << "" << "DS_Store";
		QTest::newRow("nonroot folder")		<< "file:///Applications/Utilities/"	<< "/Applications/Utilities/"	<< "/Applications/Utilities/" << "" << "";
		QTest::newRow("nonroot file")		<< "file:///Applications/somefile"		<< "/Applications/somefile"		<< "/Applications/" << "somefile" << "";
		QTest::newRow("nonroot file ext")	<< "file:///dos/qbasic.exe"				<< "/dos/qbasic.exe"			<< "/dos/" << "qbasic" << "exe";
		QTest::newRow("nonroot filedot")	<< "file:///dir/file."					<< "/dir/file."					<< "/dir/" << "file" << "";
	}
	void testFileParts()
	{
		QFETCH(QString, url);
		QFETCH(QString, expectedPath);
		QFETCH(QString, expectedFolder);
		QFETCH(QString, expectedFilename);
		QFETCH(QString, expectedExtension);

		VuoText path;
		VuoText folder;
		VuoText filename;
		VuoText extension;
		QVERIFY(VuoUrl_getFileParts(url.toUtf8().data(), &path, &folder, &filename, &extension));

		QCOMPARE(QString(path), expectedPath);
		QCOMPARE(QString(folder), expectedFolder);
		QCOMPARE(QString(filename), expectedFilename);
		QCOMPARE(QString(extension), expectedExtension);
	}

	void testBundle_data()
	{
		QTest::addColumn<QString>("url");
		QTest::addColumn<bool>("expectedBundle");

		QTest::newRow("empty string")		<< ""													<< false;
		QTest::newRow("HTTP")				<< "http://vuo.org"										<< false;
		QTest::newRow("non-bundle folder")	<< "file:///"											<< false;
		QTest::newRow("non-bundle folder2")	<< "file:///Applications"								<< false;
		QTest::newRow("app bundle")			<< "file:///Applications/Calculator.app"				<< true;
		QTest::newRow("framework bundle")	<< "file:///System/Library/Frameworks/AppKit.framework"	<< false;
	}
	void testBundle()
	{
		QFETCH(QString, url);
		QFETCH(bool, expectedBundle);

		QCOMPARE(VuoUrl_isBundle(url.toUtf8().data()), expectedBundle);
	}
};

QTEST_APPLESS_MAIN(TestVuoUrl)

#include "TestVuoUrl.moc"

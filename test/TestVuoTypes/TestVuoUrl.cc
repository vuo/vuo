/**
 * @file
 * TestVuoUrl implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <sys/stat.h>

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
		QTest::addColumn<QString>("expectedAppLoadPath");
		QTest::addColumn<QString>("expectedAppSavePath");

		QString fileScheme = "file://";
		QString basePath = getcwd(NULL,0);
		QString baseUrl = fileScheme + basePath;
		QString homeDir = getenv("HOME");

		QString basePathParent = basePath;
		basePathParent.truncate(basePath.lastIndexOf("/TestVuoTypes"));
		QString baseUrlParent = fileScheme + basePathParent;

		QString basePathParentParent = basePathParent;
		basePathParentParent.truncate(basePathParent.lastIndexOf("/test"));
		QString baseUrlParentParent = fileScheme + basePathParentParent;

		QString resourcesPath = basePathParent + "/Resources";
		QString resourcesPathParent = basePathParent;
		QString resourcesPathParentParent = basePathParentParent;

		QString desktopPath = homeDir + "/Desktop";

		//											   url							   expectedNormalizedUrl					   posix?   expectedPosixPath					   expectedAppLoadPath						   expectedAppSavePath
		QTest::newRow("empty string")				<< ""							<< baseUrl									<< true  << basePath							<< resourcesPath							<< desktopPath;
		QTest::newRow("absolute URL")				<< "http://vuo.org"				<< "http://vuo.org"							<< false << ""									<< ""										<< "";
		QTest::newRow("absolute URL/")				<< "http://vuo.org/"			<< "http://vuo.org"							<< false << ""									<< ""										<< "";
		QTest::newRow("absolute URL space")			<< "http://vuo.org/a b.png"		<< "http://vuo.org/a%20b.png"				<< false << ""									<< ""										<< "";
		QTest::newRow("relative file")				<< "file"						<< baseUrl + "/file"						<< true  << basePath + "/file"					<< resourcesPath + "/file"					<< desktopPath + "/file";
		QTest::newRow("relative file ?")			<< "file?.wav"					<< baseUrl + "/file%3f.wav"					<< true  << basePath + "/file?.wav"				<< resourcesPath + "/file?.wav"				<< desktopPath + "/file?.wav";
		QTest::newRow("relative dir/")				<< "dir/"						<< baseUrl + "/dir"							<< true  << basePath + "/dir"					<< resourcesPath + "/dir"					<< desktopPath + "/dir";
		QTest::newRow("relative dir/file")			<< "dir/file"					<< baseUrl + "/dir/file"					<< true  << basePath + "/dir/file"				<< resourcesPath + "/dir/file"				<< desktopPath + "/dir/file";
		QTest::newRow("relative ./file")			<< "./Makefile"					<< baseUrl + "/Makefile"					<< true  << basePath + "/Makefile"				<< resourcesPath + "/Makefile"				<< desktopPath + "/Makefile";
		QTest::newRow("relative ../file")			<< "../Makefile"				<< baseUrlParent + "/Makefile"				<< true  << basePathParent + "/Makefile"		<< resourcesPathParent + "/Makefile"		<< desktopPath + "/../Makefile" /* Doesn't exist, so ".." remains. */;
		QTest::newRow("relative ../../file")		<< "../../Makefile"				<< baseUrlParentParent + "/Makefile"		<< true  << basePathParentParent + "/Makefile"	<< resourcesPathParentParent + "/Makefile"	<< desktopPath + "/../../Makefile" /* Doesn't exist, so "../.." remains. */;
		QTest::newRow("absolute file")				<< "/mach_kernel"				<< fileScheme + "/mach_kernel"				<< true  << "/mach_kernel"						<< "/mach_kernel"							<< "/mach_kernel";
		QTest::newRow("absolute colon")				<< "/ScreenShot 09:41:00"		<< fileScheme + "/ScreenShot%2009%3a41%3a00"<< true  << "/ScreenShot 09꞉41꞉00"				<< "/ScreenShot 09꞉41꞉00"					<< "/ScreenShot 09꞉41꞉00";
		QTest::newRow("absolute dir/")				<< "/usr/include/"				<< fileScheme + "/usr/include"				<< true  << "/usr/include"						<< "/usr/include"							<< "/usr/include";
		QTest::newRow("absolute dir space")			<< "/Library/Desktop Pictures"	<< "file:///Library/Desktop%20Pictures"		<< true  << "/Library/Desktop Pictures"			<< "/Library/Desktop Pictures"				<< "/Library/Desktop Pictures";
		QTest::newRow("absolute dir/file")			<< "/usr/include/stdio.h"		<< fileScheme + "/usr/include/stdio.h"		<< true  << "/usr/include/stdio.h"				<< "/usr/include/stdio.h"					<< "/usr/include/stdio.h";
		QTest::newRow("absolute link/dir")			<< "/var/tmp"					<< fileScheme + "/private/var/tmp"			<< true  << "/private/var/tmp"					<< "/private/var/tmp"						<< "/private/var/tmp";
		QTest::newRow("absolute dir/../link")		<< "/usr/../var"				<< fileScheme + "/private/var"				<< true  << "/private/var"						<< "/private/var"							<< "/private/var";
		QTest::newRow("user homedir")				<< "~"							<< fileScheme + homeDir						<< true  << homeDir								<< homeDir									<< homeDir;
		QTest::newRow("user homedir/")				<< "~/"							<< fileScheme + homeDir						<< true  << homeDir								<< homeDir									<< homeDir;
		QTest::newRow("user homedir/file")			<< "~/.DS_Store"				<< fileScheme + homeDir + "/.DS_Store"		<< true  << homeDir + "/.DS_Store"				<< homeDir + "/.DS_Store"					<< homeDir + "/.DS_Store";
		QTest::newRow("user homedir/filenex")		<< "~/nonexistent"				<< fileScheme + homeDir + "/nonexistent"	<< true  << homeDir + "/nonexistent"			<< homeDir + "/nonexistent"					<< homeDir + "/nonexistent";
		QTest::newRow("user homedir/dir/../dir")	<< "~/Library/../Downloads"		<< fileScheme + homeDir + "/Downloads"		<< true  << homeDir + "/Downloads"				<< homeDir + "/Downloads"					<< homeDir + "/Downloads";
	}
	void testNormalize()
	{
		QFETCH(QString, url);
		QFETCH(QString, expectedNormalizedUrl);
		QFETCH(bool, expectedValidPosixPath);
		QFETCH(QString, expectedPosixPath);
		QFETCH(QString, expectedAppLoadPath);
		QFETCH(QString, expectedAppSavePath);

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


			// Simulate being inside an exported app bundle.
			{
				QString resourcesDir = "../Resources";
				mkdir(resourcesDir.toUtf8().data(), 0755);
				QVERIFY(QDir(resourcesDir).exists());

				// Create makefiles where the test data expect them to exist
				QString resourcesMakefile = resourcesDir + "/Makefile";
				fclose(fopen(resourcesMakefile.toUtf8().data(), "w"));
				QVERIFY(QFile(resourcesMakefile).exists());

				QString desktopMakefile = QString(getenv("HOME")) + "/Desktop/Makefile";
				fclose(fopen(desktopMakefile.toUtf8().data(), "w"));
				QVERIFY(QFile(desktopMakefile).exists());

				char *formerWorkingDir = getcwd(NULL, 0);
				chdir("/");

				{
					VuoUrl appLoadUrl = VuoUrl_normalize(url.toUtf8().data(), false);
					VuoLocal(appLoadUrl);

					VuoText appLoadPath = VuoUrl_getPosixPath(appLoadUrl);
					VuoLocal(appLoadPath);
					QCOMPARE(appLoadPath, expectedAppLoadPath.toUtf8().data());
				}

				{
					VuoUrl appSaveUrl = VuoUrl_normalize(url.toUtf8().data(), true);
					VuoLocal(appSaveUrl);

					VuoText appSavePath = VuoUrl_getPosixPath(appSaveUrl);
					VuoLocal(appSavePath);
					QCOMPARE(appSavePath, expectedAppSavePath.toUtf8().data());
				}

				chdir(formerWorkingDir);

				unlink(desktopMakefile.toUtf8().data());
				QVERIFY(!QFile(desktopMakefile).exists());

				unlink(resourcesMakefile.toUtf8().data());
				QVERIFY(!QFile(resourcesMakefile).exists());
				rmdir(resourcesDir.toUtf8().data());
				QVERIFY(!QDir(resourcesDir).exists());
			}
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

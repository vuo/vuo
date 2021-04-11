/**
 * @file
 * TestVuoCompilerCompatibility interface and implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <Vuo/Vuo.h>

/**
 * Tests of the VuoCompilerCompatibility class.
 */
class TestVuoCompilerCompatibility : public QObject
{
	Q_OBJECT

private slots:

	void testIsCompatibleWith_data()
	{
		QTest::addColumn< QString >("compatibilityStr1");
		QTest::addColumn< QString >("compatibilityStr2");
		QTest::addColumn< bool >("shouldBeCompatible");

		QString current = "current";
		QString macos__10_15__x86_64 = VUO_QSTRINGIFY({"macos":{"arch":["x86_64"],"min":"10.15","max":"10.15"}});
		QString macos__11_0__arm64 = VUO_QSTRINGIFY({"macos":{"arch":["arm64"],"min":"11.0","max":"11.0"}});

		QString any = "any";
		QString macos = "any";
		QString macos__10_15 = VUO_QSTRINGIFY({"macos":{"min":"10.15","max":"10.15"}});
		QString macos__x86_64 = VUO_QSTRINGIFY({"macos":{"arch":["x86_64"]}});
		QString macos__10_15_and_up = VUO_QSTRINGIFY({"macos":{"min":"10.15"}});
		QString macos__10_14_and_down = VUO_QSTRINGIFY({"macos":{"max":"10.14"}});
		QString macos__10_13_thru_10_15 = VUO_QSTRINGIFY({"macos":{"min":"10.13","max":"10.15"}});
		QString macos__x86_64__arm64 = VUO_QSTRINGIFY({"macos":{"arch":["x86_64","arm64"]}});

		QTest::newRow("any -> any")                                   << any                     << any                  << true;
		QTest::newRow("any -> current system")                        << any                     << current              << true;
		QTest::newRow("current system -> any")                        << current                 << any                  << false;
		QTest::newRow("any -> macOS 10.15 x86_64")                    << any                     << macos__10_15__x86_64 << true;
		QTest::newRow("macOS -> macOS 10.15 x86_64")                  << macos                   << macos__10_15__x86_64 << true;
		QTest::newRow("macOS 10.15 -> macOS 10.15 x86_64")            << macos__10_15            << macos__10_15__x86_64 << true;
		QTest::newRow("macOS x86_64 -> macOS 10.15 x86_64")           << macos__x86_64           << macos__10_15__x86_64 << true;
		QTest::newRow("macOS 10.15 x86_64 -> macOS 10.15 x86_64")     << macos__10_15__x86_64    << macos__10_15__x86_64 << true;
		QTest::newRow("macOS 10.15 and up -> macOS 10.15 x86_64")     << macos__10_15_and_up     << macos__10_15__x86_64 << true;
		QTest::newRow("macOS 10.14 and down -> macOS 10.15 x86_64")   << macos__10_14_and_down   << macos__10_15__x86_64 << false;
		QTest::newRow("macOS 10.13 thru 10.15 -> macOS 10.15 x86_64") << macos__10_13_thru_10_15 << macos__10_15__x86_64 << true;
		QTest::newRow("macOS -> macOS 11.0 arm64")                    << macos                   << macos__11_0__arm64   << true;
		QTest::newRow("macOS 10.15 -> macOS 11.0 arm64")              << macos__10_15            << macos__11_0__arm64   << false;
		QTest::newRow("macOS x86_64 -> macOS 11.0 arm64")             << macos__x86_64           << macos__11_0__arm64   << false;
		QTest::newRow("macOS 10.15 and up -> macOS 11.0 arm64")       << macos__10_15_and_up     << macos__11_0__arm64   << true;
		QTest::newRow("macOS 10.14 and down -> macOS 11.0 arm64")     << macos__10_14_and_down   << macos__11_0__arm64   << false;
		QTest::newRow("macOS 10.13 thru 10.15 -> macOS 11.0 arm64")   << macos__10_13_thru_10_15 << macos__11_0__arm64   << false;
		QTest::newRow("macOS 10.15 -> macOS x86_64")                  << macos__10_15            << macos__x86_64        << false;
		QTest::newRow("macOS x86_64 -> macOS x86_64 arm64")           << macos__x86_64           << macos__x86_64__arm64 << false;
		QTest::newRow("macOS x86_64 arm64 -> macOS x86_64")           << macos__x86_64__arm64    << macos__x86_64        << true;
	}
	void testIsCompatibleWith()
	{
		QFETCH(QString, compatibilityStr1);
		QFETCH(QString, compatibilityStr2);
		QFETCH(bool, shouldBeCompatible);

		auto makeCompatibility = [](QString s)
		{
			if (s == "current")
				return VuoCompilerCompatibility::currentSystem();
			if (s == "any")
				return VuoCompilerCompatibility(nullptr);

			return VuoCompilerCompatibility(json_tokener_parse(s.toUtf8().constData()));
		};

		VuoCompilerCompatibility compatibility1 = makeCompatibility(compatibilityStr1);
		VuoCompilerCompatibility compatibility2 = makeCompatibility(compatibilityStr2);

		QCOMPARE(compatibility1.isCompatibleWith(compatibility2), shouldBeCompatible);
	}

	void testToString_data()
	{
		QTest::addColumn< QString >("jsonStr");
		QTest::addColumn< QString >("expected");

		QTest::newRow("single architecture")      << VUO_QSTRINGIFY({"macos":{"arch":["x86_64"]}})               << "macOS on x86_64";
		QTest::newRow("multiple architectures")   << VUO_QSTRINGIFY({"macos":{"arch":["x86_64","arm64"]}})       << "macOS on x86_64,arm64";
		QTest::newRow("single version")           << VUO_QSTRINGIFY({"macos":{"min":"10.14","max":"10.14"}})     << "macOS 10.14";
		QTest::newRow("range of versions")        << VUO_QSTRINGIFY({"macos":{"min":"10.13","max":"10.15"}})     << "macOS 10.13 through 10.15";
		QTest::newRow("min version")              << VUO_QSTRINGIFY({"macos":{"min":"10.12"}})                   << "macOS 10.12 and above";
		QTest::newRow("max version")              << VUO_QSTRINGIFY({"macos":{"max":"10.11"}})                   << "macOS 10.11 and below";
		QTest::newRow("version and architecture") << VUO_QSTRINGIFY({"macos":{"min":"10.14","arch":["x86_64"]}}) << "macOS 10.14 and above on x86_64";
	}
	void testToString()
	{
		QFETCH(QString, jsonStr);
		QFETCH(QString, expected);

		json_object *json = (jsonStr == "null" ? nullptr : json_tokener_parse(jsonStr.toUtf8().constData()));

		VuoCompilerCompatibility c(json);

		QCOMPARE(QString::fromStdString(c.toString()), expected);
	}
};

QTEST_APPLESS_MAIN(TestVuoCompilerCompatibility)
#include "TestVuoCompilerCompatibility.moc"

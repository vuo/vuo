/**
 * @file
 * TestVuoUiTheme implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoUiTheme.h"
#include "VuoList_VuoUiTheme.h"
}

#include "VuoUiThemeBase.hh"

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(VuoUiTheme);

/**
 * Tests the VuoUiTheme type.
 */
class TestVuoUiTheme : public QObject
{
	Q_OBJECT

private slots:

	void testPolymorphicSerialization()
	{
		// Create a rounded theme for a button.
		VuoFont font = VuoFont_make(VuoText_make("HelveticaNeue-Bold"), 72, false, VuoColor_makeWithRGBA(.1,.1,.1,1), VuoHorizontalAlignment_Left, 1, 1);
		VuoUiTheme t = VuoUiTheme_makeButtonRounded(.1, .1, font,
													VuoAnchor_make(VuoHorizontalAlignment_Center, VuoVerticalAlignment_Center),
													(VuoPoint2d){0,0},
													(VuoColor){1, 1,1,1},
													(VuoColor){1, 1,1,1},
													(VuoColor){1, 1,1,1},
													(VuoColor){.5,0,0,1},
													(VuoColor){.6,0,0,1},
													(VuoColor){.4,0,0,1},
													(VuoColor){1, 0,0,1},
													(VuoColor){1, 0,0,1},
													(VuoColor){1, 0,0,1},
													.01, .01);
		VuoLocal(t);
		QVERIFY(t);

		// Test serializing and unserializing it.
		json_object *js = VuoUiTheme_getJson(t);
		QVERIFY(js);

		VuoUiTheme t2 = VuoUiTheme_makeFromJson(js);
		VuoLocal(t2);
		QVERIFY(t2);

		// Make sure the unserialized instance matches the original instance.
		QVERIFY(VuoUiTheme_areEqual(t, t2));
		QVERIFY(!VuoUiTheme_isLessThan(t, t2));
		QVERIFY(!VuoUiTheme_isLessThan(t2, t));


		// Create a theme group.
		VuoList_VuoUiTheme themes = VuoListCreate_VuoUiTheme();
		VuoLocal(themes);
		QVERIFY(themes);

		VuoListAppendValue_VuoUiTheme(themes, t);
		VuoListAppendValue_VuoUiTheme(themes, t2);

		VuoUiTheme themeGroup = VuoUiTheme_makeGroup(themes);
		VuoLocal(themeGroup);
		QVERIFY(themeGroup);

		// Test serializing and unserializing it.
		js = VuoUiTheme_getJson(themeGroup);
		QVERIFY(js);

		VuoUiTheme themeGroup2 = VuoUiTheme_makeFromJson(js);
		VuoLocal(themeGroup2);
		QVERIFY(themeGroup2);

		// Make sure the unserialized instance matches the original instance.
		QVERIFY(VuoUiTheme_areEqual(themeGroup, themeGroup2));
		QVERIFY(!VuoUiTheme_isLessThan(themeGroup, themeGroup2));
		QVERIFY(!VuoUiTheme_isLessThan(themeGroup2, themeGroup));


		// Make sure we can compare different subclasses without crashing.
		QVERIFY(!VuoUiTheme_areEqual(themeGroup, t));
		QVERIFY(!VuoUiTheme_areEqual(t, themeGroup));
		QVERIFY(VuoUiTheme_isLessThan(t, themeGroup));
		QVERIFY(!VuoUiTheme_isLessThan(themeGroup, t));


		// Render a button directly using the button theme.
		{
			VuoUiThemeButton *buttonTheme = static_cast<VuoUiThemeButton *>(VuoUiTheme_getSpecificTheme(t, "VuoUiThemeButton"));
			VuoLayer buttonLayer = buttonTheme->render(VuoRenderedLayers_makeEmpty(),
														   VuoText_make("Hi"),
														   (VuoPoint2d){0,0},
														   VuoAnchor_make(VuoHorizontalAlignment_Center, VuoVerticalAlignment_Center),
														   false, false);
			VuoLayer_retain(buttonLayer);
			VuoLayer_release(buttonLayer);
		}


		// Render a button using a theme group containing a button theme.
		{
			VuoUiThemeButton *buttonTheme = static_cast<VuoUiThemeButton *>(VuoUiTheme_getSpecificTheme(themeGroup, "VuoUiThemeButton"));
			VuoLayer buttonLayer = buttonTheme->render(VuoRenderedLayers_makeEmpty(),
														   VuoText_make("Hi"),
														   (VuoPoint2d){0,0},
														   VuoAnchor_make(VuoHorizontalAlignment_Center, VuoVerticalAlignment_Center),
														   false, false);
			VuoLayer_retain(buttonLayer);
			VuoLayer_release(buttonLayer);
		}


		// Render a button using an empty theme group.
		{
			VuoList_VuoUiTheme themes = VuoListCreate_VuoUiTheme();
			VuoLocal(themes);
			QVERIFY(themes);

			VuoUiTheme themeGroup = VuoUiTheme_makeGroup(themes);
			VuoLocal(themeGroup);
			QVERIFY(themeGroup);

			VuoUiThemeButton *buttonTheme = static_cast<VuoUiThemeButton *>(VuoUiTheme_getSpecificTheme(themeGroup, "VuoUiThemeButton"));
			VuoLayer buttonLayer = buttonTheme->render(VuoRenderedLayers_makeEmpty(),
														   VuoText_make("Hi"),
														   (VuoPoint2d){0,0},
														   VuoAnchor_make(VuoHorizontalAlignment_Center, VuoVerticalAlignment_Center),
														   false, false);
			VuoLayer_retain(buttonLayer);
			VuoLayer_release(buttonLayer);
		}


		// Render a button using a theme group with 2 null themes (default state of vuo.ui.make.theme).
		{
			VuoList_VuoUiTheme themes = VuoListCreate_VuoUiTheme();
			VuoLocal(themes);
			QVERIFY(themes);
			VuoListAppendValue_VuoUiTheme(themes, NULL);
			VuoListAppendValue_VuoUiTheme(themes, NULL);

			VuoUiTheme themeGroup = VuoUiTheme_makeGroup(themes);
			VuoLocal(themeGroup);
			QVERIFY(themeGroup);

			VuoUiThemeButton *buttonTheme = static_cast<VuoUiThemeButton *>(VuoUiTheme_getSpecificTheme(themeGroup, "VuoUiThemeButton"));
			VuoLayer buttonLayer = buttonTheme->render(VuoRenderedLayers_makeEmpty(),
														   VuoText_make("Hi"),
														   (VuoPoint2d){0,0},
														   VuoAnchor_make(VuoHorizontalAlignment_Center, VuoVerticalAlignment_Center),
														   false, false);
			VuoLayer_retain(buttonLayer);
			VuoLayer_release(buttonLayer);
		}
	}
};

QTEST_APPLESS_MAIN(TestVuoUiTheme)

#include "TestVuoUiTheme.moc"

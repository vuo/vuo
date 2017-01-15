/**
 * @file
 * TestVuoTime implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoTime.h"
#include "VuoList_VuoTime.h"
#include "VuoRelativeTime.h"
#include "VuoRoundingMethod.h"
#include <CoreFoundation/CoreFoundation.h>
}

// Be able to use this type in QTest::addColumn()
Q_DECLARE_METATYPE(VuoWeekday);
Q_DECLARE_METATYPE(VuoTimeFormat);
Q_DECLARE_METATYPE(VuoTimeUnit);

/**
 * Tests the VuoTime type.
 */
class TestVuoTime : public QObject
{
	Q_OBJECT

private slots:

	void testCurrent()
	{
		VuoTime t = VuoTime_getCurrent();
		CFAbsoluteTime cftime = CFAbsoluteTimeGetCurrent();

		// Make sure it returns a value after 2015…
		QVERIFY(t > (2015 - 2001) * 365 * 24 * 60 * 60);
		// …and before 2025 (10 years should be enough for anyone).
		QVERIFY(t < (2025 - 2001) * 365 * 24 * 60 * 60);

		// Make sure it agrees with CFTime, which uses the same epoch.
		QVERIFY(fabs(t - cftime) < 0.1);

		// Make sure it's increasing at about the right speed.
		t = VuoTime_getCurrent();
		sleep(1);
		VuoTime t2 = VuoTime_getCurrent();
		QVERIFY(fabs(t2 - t - 1) < 0.1);
	}

	void testComponents_data()
	{
		QTest::addColumn<VuoTime   >("time");
		QTest::addColumn<bool      >("expectedSuccess");
		QTest::addColumn<int       >("expectedYear");
		QTest::addColumn<int       >("expectedDayOfYear");
		QTest::addColumn<int       >("expectedMonth");
		QTest::addColumn<int       >("expectedDayOfMonth");
		QTest::addColumn<int       >("expectedWeek");
		QTest::addColumn<VuoWeekday>("expectedDayOfWeek");
		QTest::addColumn<int       >("expectedHour");
		QTest::addColumn<int       >("expectedMinute");
		QTest::addColumn<VuoReal   >("expectedSecond");
		QTest::addColumn<QString   >("expectedSummary");

		// This test is currently assumed to run in the EST time zone; all outputs are in EST.
		//                                                                                    YYYY    DoY    MM    DD    week                          HH    MM    SS.SS
		QTest::newRow("NaN")                                     <<     nan(NULL) << false << 1969 << 365 << 12 << 31 <<  1 << VuoWeekday_Wednesday << 19 << 00 << 00.00 << "(unknown)";
		QTest::newRow("Transatlantic telegraph")                 <<-4493041200.00 << true  << 1858 << 228 <<  8 << 16 << 33 << VuoWeekday_Monday    << 00 << 00 << 00.00 << "1858-08-16 00:00:00.00";
		QTest::newRow("Phone call")                              <<-3938698800.00 << true  << 1876 <<  70 <<  3 << 10 << 10 << VuoWeekday_Friday    << 00 << 00 << 00.00 << "1876-03-10 00:00:00.00";
		QTest::newRow("Unix epoch")                              << -978307200.00 << true  << 1969 << 365 << 12 << 31 <<  1 << VuoWeekday_Wednesday << 19 << 00 << 00.00 << "1969-12-31 19:00:00.00";
		QTest::newRow("62 seconds before Mac (and Vuo) epoch")   <<        -62.00 << true  << 2000 << 366 << 12 << 31 << 52 << VuoWeekday_Sunday    << 18 << 58 << 58.00 << "2000-12-31 18:58:58.00";
		QTest::newRow("61.5 seconds before Mac (and Vuo) epoch") <<        -61.50 << true  << 2000 << 366 << 12 << 31 << 52 << VuoWeekday_Sunday    << 18 << 58 << 58.50 << "2000-12-31 18:58:58.50";
		QTest::newRow("2 seconds before Mac (and Vuo) epoch")    <<         -2.00 << true  << 2000 << 366 << 12 << 31 << 52 << VuoWeekday_Sunday    << 18 << 59 << 58.00 << "2000-12-31 18:59:58.00";
		QTest::newRow("1 second before Mac (and Vuo) epoch")     <<         -1.00 << true  << 2000 << 366 << 12 << 31 << 52 << VuoWeekday_Sunday    << 18 << 59 << 59.00 << "2000-12-31 18:59:59.00";
		QTest::newRow("Mac (and Vuo) epoch")                     <<          0.00 << true  << 2000 << 366 << 12 << 31 << 52 << VuoWeekday_Sunday    << 19 << 00 << 00.00 << "2000-12-31 19:00:00.00";
		QTest::newRow("Vuo 0.5 release")                         <<  404339744.00 << true  << 2013 << 297 << 10 << 24 << 43 << VuoWeekday_Thursday  << 16 << 35 << 44.00 << "2013-10-24 16:35:44.00";
		QTest::newRow("Vuo 1.0 release")                         <<  439971643.00 << true  << 2014 << 345 << 12 << 11 << 50 << VuoWeekday_Thursday  << 01 << 20 << 43.00 << "2014-12-11 01:20:43.00";

		QTest::newRow("second rollover: 19:00:59.00")            <<         59.00 << true  << 2000 << 366 << 12 << 31 << 52 << VuoWeekday_Sunday    << 19 << 00 << 59.00 << "2000-12-31 19:00:59.00";
		QTest::newRow("second rollover: 19:00:59.99")            <<         59.99 << true  << 2000 << 366 << 12 << 31 << 52 << VuoWeekday_Sunday    << 19 << 00 << 59.99 << "2000-12-31 19:00:59.99";
		QTest::newRow("second rollover: 19:01:00.00")            <<         60.00 << true  << 2000 << 366 << 12 << 31 << 52 << VuoWeekday_Sunday    << 19 << 01 << 00.00 << "2000-12-31 19:01:00.00";

		QTest::newRow("second rollover: 20:00:59.00")            <<       3659.00 << true  << 2000 << 366 << 12 << 31 << 52 << VuoWeekday_Sunday    << 20 << 00 << 59.00 << "2000-12-31 20:00:59.00";
		QTest::newRow("second rollover: 20:00:59.99")            <<       3659.99 << true  << 2000 << 366 << 12 << 31 << 52 << VuoWeekday_Sunday    << 20 << 00 << 59.99 << "2000-12-31 20:00:59.99";
		QTest::newRow("second rollover: 20:01:00.00")            <<       3660.00 << true  << 2000 << 366 << 12 << 31 << 52 << VuoWeekday_Sunday    << 20 << 01 << 00.00 << "2000-12-31 20:01:00.00";
	}
	void testComponents()
	{
		QFETCH(VuoTime,    time);
		QFETCH(bool,       expectedSuccess);
		QFETCH(int,        expectedYear);
		QFETCH(int,        expectedDayOfYear);
		QFETCH(int,        expectedMonth);
		QFETCH(int,        expectedDayOfMonth);
		QFETCH(int,        expectedWeek);
		QFETCH(VuoWeekday, expectedDayOfWeek);
		QFETCH(int,        expectedHour);
		QFETCH(int,        expectedMinute);
		QFETCH(VuoReal,    expectedSecond);
		QFETCH(QString,    expectedSummary);


		// Test VuoTime_getComponents().
		VuoInteger actualYear;
		VuoInteger actualMonth;
		VuoInteger actualDayOfYear;
		VuoInteger actualDayOfMonth;
		VuoInteger actualWeek;
		VuoWeekday actualDayOfWeek;
		VuoInteger actualHour;
		VuoInteger actualMinute;
		VuoReal    actualSecond;
		bool ret = VuoTime_getComponents(time, &actualYear, &actualDayOfYear, &actualMonth, &actualDayOfMonth, &actualWeek, &actualDayOfWeek, &actualHour, &actualMinute, &actualSecond);
		QCOMPARE(ret,              expectedSuccess);
		if (ret)
		{
			QCOMPARE(actualYear,       expectedYear);
			QCOMPARE(actualDayOfYear,  expectedDayOfYear);
			QCOMPARE(actualMonth,      expectedMonth);
			QCOMPARE(actualDayOfMonth, expectedDayOfMonth);
			QCOMPARE(actualWeek,       expectedWeek);
			QCOMPARE(actualDayOfWeek,  expectedDayOfWeek);
			QCOMPARE(actualHour,       expectedHour);
			QCOMPARE(actualMinute,     expectedMinute);
			QCOMPARE(actualSecond,     expectedSecond);
		}

		// Test VuoTime_getSummary().
		char *actualSummary = VuoTime_getSummary(time);
		QCOMPARE(QString(actualSummary), expectedSummary);

		// Test roundtrip VuoTime_make() -> VuoTime_getComponents().
		if (expectedSuccess)
		{
			VuoTime actualTime = VuoTime_make(expectedYear, expectedMonth, expectedDayOfMonth, expectedHour, expectedMinute, expectedSecond);
			QCOMPARE(actualTime, time);
		}
	}

	void testWeekNumber_data()
	{
		QTest::addColumn<VuoTime>("time");
		QTest::addColumn<int    >("expectedWeek");

		// Based on http://www.adsb.co.uk/date_and_time/week_numbers/
		QTest::newRow("1998W01-mon") << VuoTime_make(1997,12,29,12,0,0) <<  1;
		QTest::newRow("1998W01-wed") << VuoTime_make(1997,12,31,12,0,0) <<  1;
		QTest::newRow("1998W01-thu") << VuoTime_make(1998, 1, 1,12,0,0) <<  1;
		QTest::newRow("1998W01-sun") << VuoTime_make(1998, 1, 4,12,0,0) <<  1;
		QTest::newRow("1998W02-mon") << VuoTime_make(1998, 1, 5,12,0,0) <<  2;
		QTest::newRow("1998W53-mon") << VuoTime_make(1998,12,28,12,0,0) << 53;
		QTest::newRow("1998W53-thu") << VuoTime_make(1998,12,31,12,0,0) << 53;
		QTest::newRow("1998W53-fri") << VuoTime_make(1999, 1, 1,12,0,0) << 53;
		QTest::newRow("1998W53-sun") << VuoTime_make(1999, 1, 3,12,0,0) << 53;
		QTest::newRow("1999W01-mon") << VuoTime_make(1999, 1, 4,12,0,0) <<  1;
		QTest::newRow("1999W52-mon") << VuoTime_make(1999,12,27,12,0,0) << 52;
		QTest::newRow("1999W52-fri") << VuoTime_make(1999,12,31,12,0,0) << 52;
		QTest::newRow("1999W52-sat") << VuoTime_make(2000, 1, 1,12,0,0) << 52;
		QTest::newRow("1999W52-sun") << VuoTime_make(2000, 1, 2,12,0,0) << 52;
		QTest::newRow("2000W01-mon") << VuoTime_make(2000, 1, 3,12,0,0) <<  1;
	}
	void testWeekNumber()
	{
		QFETCH(VuoTime,    time);
		QFETCH(int,        expectedWeek);
		VuoInteger actualWeek;
		VuoTime_getComponents(time, NULL, NULL, NULL, NULL, &actualWeek, NULL, NULL, NULL, NULL);
		QCOMPARE(actualWeek,  expectedWeek);
	}

	void testAreEqualWithinTolerance_data()
	{
		QTest::addColumn<VuoList_VuoTime>("times");
		QTest::addColumn<VuoReal        >("tolerance");
		QTest::addColumn<VuoTimeUnit    >("toleranceUnit");
		QTest::addColumn<bool           >("expectedEquality");

		QTest::newRow("no times") << VuoListCreate_VuoTime() << 0.0 << VuoTimeUnit_Second << true;

		{
			VuoTime t = VuoTime_getCurrent();
			VuoList_VuoTime times = VuoListCreate_VuoTime();
			VuoListAppendValue_VuoTime(times, t);
			QTest::newRow("one time") << times << 0.0 << VuoTimeUnit_Second << true;
		}

		{
			VuoTime t = VuoTime_getCurrent();
			VuoList_VuoTime times = VuoListCreate_VuoTime();
			VuoListAppendValue_VuoTime(times, t);
			VuoListAppendValue_VuoTime(times, nan(NULL));
			QTest::newRow("time vs. NaN") << times << 1.0 << VuoTimeUnit_Second << false;
		}

		{
			VuoTime t0 = VuoTime_make(2015, 11, 1, 21, 7, 0);
			VuoTime t1 = VuoTime_make(2015, 11, 1, 22, 8, 0);
			VuoList_VuoTime sixtyOneMinutes = VuoListCreate_VuoTime();
			VuoListAppendValue_VuoTime(sixtyOneMinutes, t0);
			VuoListAppendValue_VuoTime(sixtyOneMinutes, t1);

			QTest::newRow("61min, tolerance 0")               << sixtyOneMinutes << 0.     << VuoTimeUnit_Second      << false;
			QTest::newRow("61min, tolerance 1 second")        << sixtyOneMinutes << 1.     << VuoTimeUnit_Second      << false;
			QTest::newRow("61min, tolerance 1 minute")        << sixtyOneMinutes << 1.     << VuoTimeUnit_Minute      << false;
			QTest::newRow("61min, tolerance 1 quarter-hour")  << sixtyOneMinutes << 1.     << VuoTimeUnit_QuarterHour << false;
			QTest::newRow("61min, tolerance 1 half-hour")     << sixtyOneMinutes << 1.     << VuoTimeUnit_HalfHour    << false;
			QTest::newRow("61min, tolerance 1 hour")          << sixtyOneMinutes << 1.     << VuoTimeUnit_Hour        << false;
			QTest::newRow("61min, tolerance 1/24 day")        << sixtyOneMinutes << 1./24. << VuoTimeUnit_Day         << false;
			QTest::newRow("61min, tolerance 1/23 day")        << sixtyOneMinutes << 1./23. << VuoTimeUnit_Day         << true;
			QTest::newRow("61min, tolerance 5 quarter-hours") << sixtyOneMinutes << 5.     << VuoTimeUnit_QuarterHour << true;
			QTest::newRow("61min, tolerance 3 half-hours")    << sixtyOneMinutes << 3.     << VuoTimeUnit_HalfHour    << true;
			QTest::newRow("61min, tolerance 1 day")           << sixtyOneMinutes << 1.     << VuoTimeUnit_Day         << true;
			QTest::newRow("61min, tolerance 1 weekSunday")    << sixtyOneMinutes << 1.     << VuoTimeUnit_WeekSunday  << true;
			QTest::newRow("61min, tolerance 1 weekMonday")    << sixtyOneMinutes << 1.     << VuoTimeUnit_WeekMonday  << true;
			QTest::newRow("61min, tolerance 1 month")         << sixtyOneMinutes << 1.     << VuoTimeUnit_Month       << true;
			QTest::newRow("61min, tolerance 1 quarter")       << sixtyOneMinutes << 1.     << VuoTimeUnit_Quarter     << true;
			QTest::newRow("61min, tolerance 1 year")          << sixtyOneMinutes << 1.     << VuoTimeUnit_Year        << true;
			QTest::newRow("61min, tolerance 1 decade")        << sixtyOneMinutes << 1.     << VuoTimeUnit_Decade      << true;
			QTest::newRow("61min, tolerance 1 century")       << sixtyOneMinutes << 1.     << VuoTimeUnit_Century     << true;
			QTest::newRow("61min, tolerance 1 millennium")    << sixtyOneMinutes << 1.     << VuoTimeUnit_Millennium  << true;
		}

		{
			VuoTime t0 = VuoTime_make(2015, 11, 1, 21, 7, 0);
			VuoTime t1 = VuoTime_make(2015, 11, 1, 22, 6, 0);
			VuoTime t2 = VuoTime_make(2015, 11, 1, 23, 5, 0);
			VuoList_VuoTime times = VuoListCreate_VuoTime();
			VuoListAppendValue_VuoTime(times, t1);
			VuoListAppendValue_VuoTime(times, t2);
			VuoListAppendValue_VuoTime(times, t0);

			QTest::newRow("three times, tolerance 0")        << times << 0. << VuoTimeUnit_Second << false;
			QTest::newRow("three times, tolerance 1 second") << times << 1. << VuoTimeUnit_Second << false;
			QTest::newRow("three times, tolerance 1 hour")   << times << 1. << VuoTimeUnit_Hour   << false;
			QTest::newRow("three times, tolerance 2 hours")  << times << 2. << VuoTimeUnit_Hour   << true;
		}
	}
	void testAreEqualWithinTolerance()
	{
		QFETCH(VuoList_VuoTime, times);
		QFETCH(VuoReal,         tolerance);
		QFETCH(VuoTimeUnit,     toleranceUnit);
		QFETCH(bool,            expectedEquality);
		bool actualEquality = VuoTime_areEqualWithinTolerance(times, tolerance, toleranceUnit);
		QCOMPARE(actualEquality, expectedEquality);
	}


	void testAreTimesOfDayEqualWithinTolerance_data()
	{
		QTest::addColumn<VuoList_VuoTime>("times");
		QTest::addColumn<VuoReal        >("tolerance");
		QTest::addColumn<VuoTimeUnit    >("toleranceUnit");
		QTest::addColumn<bool           >("expectedEquality");

		QTest::newRow("no times") << VuoListCreate_VuoTime() << 0.0 << VuoTimeUnit_Second << true;

		{
			VuoTime t = VuoTime_getCurrent();
			VuoList_VuoTime times = VuoListCreate_VuoTime();
			VuoListAppendValue_VuoTime(times, t);
			QTest::newRow("one time") << times << 0.0 << VuoTimeUnit_Second << true;
		}

		{
			VuoTime t = VuoTime_getCurrent();
			VuoList_VuoTime times = VuoListCreate_VuoTime();
			VuoListAppendValue_VuoTime(times, t);
			VuoListAppendValue_VuoTime(times, nan(NULL));
			QTest::newRow("time vs. NaN") << times << 1.0 << VuoTimeUnit_Second << false;
		}

		// Test 2 times, with midnight wraparound.
		// This test is currently assumed to run in the EST time zone; all outputs are in EST.
		{
			VuoTime t0 = VuoTime_make(2010, 6, 1, 19,  1, 0);
			VuoTime t1 = VuoTime_make(2015, 6, 1, 20, 19, 0);
			VuoList_VuoTime times = VuoListCreate_VuoTime();
			VuoListAppendValue_VuoTime(times, t0);
			VuoListAppendValue_VuoTime(times, t1);

			QTest::newRow("two times, tolerance 0")               << times << 0.     << VuoTimeUnit_Second      << false;
			QTest::newRow("two times, tolerance 1 second")        << times << 1.     << VuoTimeUnit_Second      << false;
			QTest::newRow("two times, tolerance 1 minute")        << times << 1.     << VuoTimeUnit_Minute      << false;
			QTest::newRow("two times, tolerance 1 quarter-hour")  << times << 1.     << VuoTimeUnit_QuarterHour << false;
			QTest::newRow("two times, tolerance 1 half-hour")     << times << 1.     << VuoTimeUnit_HalfHour    << false;
			QTest::newRow("two times, tolerance 1 hour")          << times << 1.     << VuoTimeUnit_Hour        << false;
			QTest::newRow("two times, tolerance 2 hours")         << times << 2.     << VuoTimeUnit_Hour        << true;
			QTest::newRow("two times, tolerance 6 quarter-hours") << times << 6.     << VuoTimeUnit_QuarterHour << true;
			QTest::newRow("two times, tolerance 3 half-hours")    << times << 3.     << VuoTimeUnit_HalfHour    << true;
		}

		// Test 3 times, with midnight wraparound.
		// This test is currently assumed to run in the EST time zone; all outputs are in EST.
		{
			VuoTime t0 = VuoTime_make(1970,  1,  7, 19, 59, 59.5);
			VuoTime t1 = VuoTime_make(2040,  2, 19, 19, 59, 59.9);
			VuoTime t2 = VuoTime_make(2015,  3,  1, 20,  0,  0.1);
			VuoList_VuoTime times = VuoListCreate_VuoTime();
			VuoListAppendValue_VuoTime(times, t1);
			VuoListAppendValue_VuoTime(times, t2);
			VuoListAppendValue_VuoTime(times, t0);

			QTest::newRow("three times, tolerance 0")           << times << 0.  << VuoTimeUnit_Second << false;
			QTest::newRow("three times, tolerance 0.5 seconds") << times <<  .5 << VuoTimeUnit_Second << false;
			QTest::newRow("three times, tolerance 1 second")    << times << 1.  << VuoTimeUnit_Second << true;
		}
	}
	void testAreTimesOfDayEqualWithinTolerance()
	{
		QFETCH(VuoList_VuoTime, times);
		QFETCH(VuoReal,         tolerance);
		QFETCH(VuoTimeUnit,     toleranceUnit);
		QFETCH(bool,            expectedEquality);
		bool actualEquality = VuoTime_areTimesOfDayEqualWithinTolerance(times, tolerance, toleranceUnit);
		QCOMPARE(actualEquality, expectedEquality);
	}

	void testIsTimeOfDayLessThan_data()
	{
		QTest::addColumn<VuoTime>("timeA");
		QTest::addColumn<VuoTime>("timeB");
		QTest::addColumn<VuoTime>("startOfDay");
		QTest::addColumn<bool   >("expectedLessThan");

		QTest::newRow("same time")                   << VuoTime_make(2015, 11, 2, 19, 0, 59.5) << VuoTime_make(2015, 11, 2, 19, 0, 59.5) << VuoTime_make(2015, 11, 2, 19, 0, 0) << false;
		QTest::newRow("within day, less")            << VuoTime_make(2015, 11, 2,  5, 0,  0  ) << VuoTime_make(2015, 11, 2,  7, 0,  0  ) << VuoTime_make(2015, 11, 2, 19, 0, 0) << true;
		QTest::newRow("within day, not less")        << VuoTime_make(2015, 11, 2,  7, 0,  0  ) << VuoTime_make(2015, 11, 2,  5, 0,  0  ) << VuoTime_make(2015, 11, 2, 19, 0, 0) << false;
		QTest::newRow("span midnight, less")         << VuoTime_make(2015, 11, 2, 18, 0,  0  ) << VuoTime_make(2015, 11, 2, 20, 0,  0  ) << VuoTime_make(2015, 11, 2, 21, 0, 0) << true;
		QTest::newRow("span midnight, not less")     << VuoTime_make(2015, 11, 2, 20, 0,  0  ) << VuoTime_make(2015, 11, 2, 18, 0,  0  ) << VuoTime_make(2015, 11, 2, 21, 0, 0) << false;
		QTest::newRow("span old midnight, less")     << VuoTime_make(1960, 11, 2, 18, 0,  0  ) << VuoTime_make(1960, 11, 2, 20, 0,  0  ) << VuoTime_make(1960, 11, 2, 21, 0, 0) << true;
		QTest::newRow("span old midnight, not less") << VuoTime_make(1960, 11, 2, 20, 0,  0  ) << VuoTime_make(1960, 11, 2, 18, 0,  0  ) << VuoTime_make(1960, 11, 2, 21, 0, 0) << false;
		QTest::newRow("span 06:00, less")            << VuoTime_make(2015, 11, 2,  1, 0,  0  ) << VuoTime_make(2015, 11, 2,  3, 0,  0  ) << VuoTime_make(2015, 11, 2,  4, 0, 0) << true;
		QTest::newRow("span 06:00, not less")        << VuoTime_make(2015, 11, 2,  3, 0,  0  ) << VuoTime_make(2015, 11, 2,  1, 0,  0  ) << VuoTime_make(2015, 11, 2,  4, 0, 0) << false;
	}
	void testIsTimeOfDayLessThan()
	{
		QFETCH(VuoTime, timeA);
		QFETCH(VuoTime, timeB);
		QFETCH(VuoTime, startOfDay);
		QFETCH(bool,    expectedLessThan);
		bool actualLessThan = VuoTime_isTimeOfDayLessThan(timeA, timeB, startOfDay);
		QCOMPARE(actualLessThan, expectedLessThan);
	}

	void testRelativeTime_data()
	{
		QTest::addColumn<int    >("years");
		QTest::addColumn<int    >("months");
		QTest::addColumn<int    >("days");
		QTest::addColumn<int    >("hours");
		QTest::addColumn<int    >("minutes");
		QTest::addColumn<VuoReal>("seconds");
		QTest::addColumn<VuoTime>("expectedTime");
		QTest::addColumn<QString>("expectedSummary");

		QTest::newRow("zero")                    <<  0 <<  0 <<   0 <<  0 <<   0 <<   0.00 <<         0.0 << "no offset";
		QTest::newRow("1 second")                <<  0 <<  0 <<   0 <<  0 <<   0 <<   1.00 <<         1.0 << "1 second after";
		QTest::newRow("-1 second")               <<  0 <<  0 <<   0 <<  0 <<   0 <<  -1.00 <<        -1.0 << "1 second before";
		QTest::newRow("1.5 seconds")             <<  0 <<  0 <<   0 <<  0 <<   0 <<   1.50 <<         1.5 << "1.5 seconds after";
		QTest::newRow("-1.5 seconds")            <<  0 <<  0 <<   0 <<  0 <<   0 <<  -1.50 <<        -1.5 << "1.5 seconds before";
		QTest::newRow("20 minutes")              <<  0 <<  0 <<   0 <<  0 <<  20 <<   0.00 <<      1200.0 << "20 minutes after";
		QTest::newRow("1 hour")                  <<  0 <<  0 <<   0 <<  1 <<   0 <<   0.00 <<      3600.0 << "1 hour after";
		QTest::newRow("1 day")                   <<  0 <<  0 <<   1 <<  0 <<   0 <<   0.00 <<     86400.0 << "1 day after";
		QTest::newRow("1 month")                 <<  0 <<  1 <<   0 <<  0 <<   0 <<   0.00 <<   2592000.0 << "1 month after";
		QTest::newRow("1 year")                  <<  1 <<  0 <<   0 <<  0 <<   0 <<   0.00 <<  31536000.0 << "1 year after";
		QTest::newRow("Between Vuo 0.5 and 1.0") <<  1 <<  1 <<  17 <<  9 <<  44 <<  59.00 <<  35631899.0 << "1 year, 1 month, 17 days, 9 hours, 44 minutes, and 59 seconds after";
		QTest::newRow("Between Vuo 1.0 and 0.5") << -1 << -1 << -17 << -9 << -44 << -59.00 << -35631899.0 << "1 year, 1 month, 17 days, 9 hours, 44 minutes, and 59 seconds before";
	}
	void testRelativeTime()
	{
		QFETCH(int,     years);
		QFETCH(int,     months);
		QFETCH(int,     days);
		QFETCH(int,     hours);
		QFETCH(int,     minutes);
		QFETCH(VuoReal, seconds);
		QFETCH(VuoTime, expectedTime);
		QFETCH(QString, expectedSummary);

		VuoRelativeTime actualTime = VuoRelativeTime_make(years,months,days,hours,minutes,seconds);
		QCOMPARE(actualTime, expectedTime);

		VuoInteger actualYears;
		VuoInteger actualMonths;
		VuoInteger actualDays;
		VuoInteger actualHours;
		VuoInteger actualMinutes;
		VuoReal    actualSeconds;
		VuoRelativeTime_getComponents(expectedTime, &actualYears, &actualMonths, &actualDays, &actualHours, &actualMinutes, &actualSeconds);
		QCOMPARE(actualYears, years);
		QCOMPARE(actualMonths, months);
		QCOMPARE(actualDays, days);
		QCOMPARE(actualHours, hours);
		QCOMPARE(actualMinutes, minutes);
		QCOMPARE(actualSeconds, seconds);

		char *actualSummary = VuoRelativeTime_getSummary(expectedTime);
		QCOMPARE(QString(actualSummary), expectedSummary);
		free(actualSummary);
	}

	void testRFC822_data()
	{
		QTest::addColumn<QString>("rfc822");
		QTest::addColumn<VuoTime>("expectedTime");

		QTest::newRow("UTC") << "Wed, 14 Oct 2015 13:02:17 +0000" << 466520537.;
		QTest::newRow("-4")  << "Fri, 16 Oct 2015 06:42:34 -0400" << 466684954.;
	}
	void testRFC822()
	{
		QFETCH(QString, rfc822);
		QFETCH(VuoTime, expectedTime);

		VuoTime actualTime = VuoTime_makeFromRFC822(rfc822.toUtf8().data());
		QCOMPARE(actualTime, expectedTime);
	}

	void testRound_data()
	{
		QTest::addColumn<VuoTime>("time");
		QTest::addColumn<VuoTimeUnit>("unit");
		QTest::addColumn<VuoTime>("expectedRoundedDownTime");
		QTest::addColumn<VuoTime>("expectedRoundedNearestTime");
		QTest::addColumn<VuoTime>("expectedRoundedUpTime");

		VuoTime t = VuoTime_make(2015, 6, 1, 20, 19, 32.6);

		QTest::newRow("Millennium")  << t << VuoTimeUnit_Millennium  << VuoTime_make(2000, 1,  1,  0,  0,  0)
																	 << VuoTime_make(2000, 1,  1,  0,  0,  0)
																	 << VuoTime_make(3000, 1,  1,  0,  0,  0);
		QTest::newRow("Century")     << t << VuoTimeUnit_Century     << VuoTime_make(2000, 1,  1,  0,  0,  0)
																	 << VuoTime_make(2000, 1,  1,  0,  0,  0)
																	 << VuoTime_make(2100, 1,  1,  0,  0,  0);
		QTest::newRow("Decade")      << t << VuoTimeUnit_Decade      << VuoTime_make(2010, 1,  1,  0,  0,  0)
																	 << VuoTime_make(2010, 1,  1,  0,  0,  0)
																	 << VuoTime_make(2020, 1,  1,  0,  0,  0);
		QTest::newRow("Year")        << t << VuoTimeUnit_Year        << VuoTime_make(2015, 1,  1,  0,  0,  0)
																	 << VuoTime_make(2015, 1,  1,  0,  0,  0)
																	 << VuoTime_make(2016, 1,  1,  0,  0,  0);
		QTest::newRow("Quarter")     << t << VuoTimeUnit_Quarter     << VuoTime_make(2015, 4,  1,  0,  0,  0)
																	 << VuoTime_make(2015, 7,  1,  0,  0,  0)
																	 << VuoTime_make(2015, 7,  1,  0,  0,  0);
		QTest::newRow("Month")       << t << VuoTimeUnit_Month       << VuoTime_make(2015, 6,  1,  0,  0,  0)
																	 << VuoTime_make(2015, 6,  1,  0,  0,  0)
																	 << VuoTime_make(2015, 7,  1,  0,  0,  0);
		QTest::newRow("WeekSunday")  << t << VuoTimeUnit_WeekSunday  << VuoTime_make(2015, 5, 31,  0,  0,  0)
																	 << VuoTime_make(2015, 5, 31,  0,  0,  0)
																	 << VuoTime_make(2015, 6,  7,  0,  0,  0);
		QTest::newRow("WeekMonday")  << t << VuoTimeUnit_WeekMonday  << VuoTime_make(2015, 6,  1,  0,  0,  0)
																	 << VuoTime_make(2015, 6,  1,  0,  0,  0)
																	 << VuoTime_make(2015, 6,  8,  0,  0,  0);
		QTest::newRow("Day")         << t << VuoTimeUnit_Day         << VuoTime_make(2015, 6,  1,  0,  0,  0)
																	 << VuoTime_make(2015, 6,  2,  0,  0,  0)
																	 << VuoTime_make(2015, 6,  2,  0,  0,  0);
		QTest::newRow("Hour")        << t << VuoTimeUnit_Hour        << VuoTime_make(2015, 6,  1, 20,  0,  0)
																	 << VuoTime_make(2015, 6,  1, 20,  0,  0)
																	 << VuoTime_make(2015, 6,  1, 21,  0,  0);
		QTest::newRow("HalfHour")    << t << VuoTimeUnit_HalfHour    << VuoTime_make(2015, 6,  1, 20,  0,  0)
																	 << VuoTime_make(2015, 6,  1, 20, 30,  0)
																	 << VuoTime_make(2015, 6,  1, 20, 30,  0);
		QTest::newRow("QuarterHour") << t << VuoTimeUnit_QuarterHour << VuoTime_make(2015, 6,  1, 20, 15,  0)
																	 << VuoTime_make(2015, 6,  1, 20, 15,  0)
																	 << VuoTime_make(2015, 6,  1, 20, 30,  0);
		QTest::newRow("Minute")      << t << VuoTimeUnit_Minute      << VuoTime_make(2015, 6,  1, 20, 19,  0)
																	 << VuoTime_make(2015, 6,  1, 20, 20,  0)
																	 << VuoTime_make(2015, 6,  1, 20, 20,  0);
		QTest::newRow("Second")      << t << VuoTimeUnit_Second      << VuoTime_make(2015, 6,  1, 20, 19, 32)
																	 << VuoTime_make(2015, 6,  1, 20, 19, 33)
																	 << VuoTime_make(2015, 6,  1, 20, 19, 33);
	}
	void testRound()
	{
		QFETCH(VuoTime,     time);
		QFETCH(VuoTimeUnit, unit);
		QFETCH(VuoTime,     expectedRoundedDownTime);
		QFETCH(VuoTime,     expectedRoundedNearestTime);
		QFETCH(VuoTime,     expectedRoundedUpTime);

		VuoTime actualRoundedDownTime    = VuoTime_round(time, unit, VuoRoundingMethod_Down);
		VuoTime actualRoundedNearestTime = VuoTime_round(time, unit, VuoRoundingMethod_Nearest);
		VuoTime actualRoundedUpTime      = VuoTime_round(time, unit, VuoRoundingMethod_Up);

		QCOMPARE(actualRoundedDownTime,    expectedRoundedDownTime);
		QCOMPARE(actualRoundedNearestTime, expectedRoundedNearestTime);
		QCOMPARE(actualRoundedUpTime,      expectedRoundedUpTime);
	}

	void testFormat_data()
	{
		QTest::addColumn<VuoTime>("time");
		QTest::addColumn<VuoTimeFormat>("format");
		QTest::addColumn<QString>("expectedFormattedTimeUS");
		QTest::addColumn<QString>("expectedFormattedTimeUK");
		QTest::addColumn<QString>("expectedFormattedTimeDE");
		QTest::addColumn<QString>("expectedFormattedTimeJP");

		VuoTime t = VuoTime_make(2015, 6, 1, 20, 19, 32.6);

		//                                                                            en_US                                  en_GB                                  de_DE                                  ja_JP
		QTest::newRow("datetime-sortable")  << t << VuoTimeFormat_DateTimeSortable << "2015-06-02 00:19:32Z"              << "2015-06-02 00:19:32Z"              << "2015-06-02 00:19:32Z"              << "2015-06-02 00:19:32Z"				;
		QTest::newRow("datetime-short-12")  << t << VuoTimeFormat_DateTimeShort12  << "06/01/2015 08:19:32 PM"            << "01/06/2015 08:19:32 pm"            << "01.06.2015 08:19:32 pm"            << "2015/06/01 08:19:32 PM"				;
		QTest::newRow("datetime-short-24")  << t << VuoTimeFormat_DateTimeShort24  << "06/01/2015 20:19:32"               << "01/06/2015 20:19:32"               << "01.06.2015 20:19:32"               << "2015/06/01 20時19分32秒"				;
		QTest::newRow("datetime-medium-12") << t << VuoTimeFormat_DateTimeMedium12 << "Jun  1, 2015 08:19:32 PM"          << "Jun  1, 2015 08:19:32 pm"          << "Jun  1, 2015 08:19:32 pm"          << " 6  1, 2015 08:19:32 PM"			;
		QTest::newRow("datetime-medium-24") << t << VuoTimeFormat_DateTimeMedium24 << "Jun  1, 2015 20:19:32"             << "Jun  1, 2015 20:19:32"             << "Jun  1, 2015 20:19:32"             << " 6  1, 2015 20時19分32秒"			;
		QTest::newRow("datetime-long-12")   << t << VuoTimeFormat_DateTimeLong12   << "Monday, June  1, 2015 08:19:32 PM" << "Monday, June  1, 2015 08:19:32 pm" << "Montag, Juni  1, 2015 08:19:32 pm" << "月曜日, 6月  1, 2015 08:19:32 PM"	;
		QTest::newRow("datetime-long-24")   << t << VuoTimeFormat_DateTimeLong24   << "Monday, June  1, 2015 20:19:32"    << "Monday, June  1, 2015 20:19:32"    << "Montag, Juni  1, 2015 20:19:32"    << "月曜日, 6月  1, 2015 20時19分32秒"	;
		QTest::newRow("date-short")         << t << VuoTimeFormat_DateShort        << "06/01/2015"                        << "01/06/2015"                        << "01.06.2015"                        << "2015/06/01"							;
		QTest::newRow("date-medium")        << t << VuoTimeFormat_DateMedium       << "Jun  1, 2015"                      << "Jun  1, 2015"                      << "Jun  1, 2015"                      << " 6  1, 2015"						;
		QTest::newRow("date-long")          << t << VuoTimeFormat_DateLong         << "Monday, June  1, 2015"             << "Monday, June  1, 2015"             << "Montag, Juni  1, 2015"             << "月曜日, 6月  1, 2015"				;
		QTest::newRow("time-12")            << t << VuoTimeFormat_Time12           << "08:19:32 PM"                       << "08:19:32 pm"                       << "08:19:32 pm"                       << "08:19:32 PM"						;
		QTest::newRow("time-24")            << t << VuoTimeFormat_Time24           << "20:19:32"                          << "20:19:32"                          << "20:19:32"                          << "20時19分32秒"						;
	}
	void testFormat()
	{
		QFETCH(VuoTime,       time);
		QFETCH(VuoTimeFormat, format);
		QFETCH(QString,       expectedFormattedTimeUS);
		QFETCH(QString,       expectedFormattedTimeUK);
		QFETCH(QString,       expectedFormattedTimeDE);
		QFETCH(QString,       expectedFormattedTimeJP);

		VuoText actualFormattedTime;

		actualFormattedTime = VuoTime_formatWithLocale(time, format, "en_US");
		VuoRetain(actualFormattedTime);
		QCOMPARE(QString(actualFormattedTime), expectedFormattedTimeUS);
		VuoRelease(actualFormattedTime);

		actualFormattedTime = VuoTime_formatWithLocale(time, format, "en_GB");
		VuoRetain(actualFormattedTime);
		QCOMPARE(QString(actualFormattedTime), expectedFormattedTimeUK);
		VuoRelease(actualFormattedTime);

		actualFormattedTime = VuoTime_formatWithLocale(time, format, "de_DE");
		VuoRetain(actualFormattedTime);
		QCOMPARE(QString(actualFormattedTime), expectedFormattedTimeDE);
		VuoRelease(actualFormattedTime);

		actualFormattedTime = VuoTime_formatWithLocale(time, format, "ja_JP");
		VuoRetain(actualFormattedTime);
		QCOMPARE(QString(actualFormattedTime), expectedFormattedTimeJP);
		VuoRelease(actualFormattedTime);
	}
};

QTEST_APPLESS_MAIN(TestVuoTime)

#include "TestVuoTime.moc"

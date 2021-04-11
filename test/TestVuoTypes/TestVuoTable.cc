/**
 * @file
 * TestVuoTable implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"

#include "VuoTable.h"
#include "VuoTime.h"
}

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(VuoListPosition);
Q_DECLARE_METATYPE(VuoTableFormat);
Q_DECLARE_METATYPE(VuoTextComparison);
Q_DECLARE_METATYPE(VuoTextSort);
Q_DECLARE_METATYPE(VuoSortOrder);

/**
 * Tests the VuoTable type.
 */
class TestVuoTable : public QObject
{
	Q_OBJECT

private slots:

	void testParseAndSerialize_data()
	{
		QTest::addColumn<QString>("input");
		QTest::addColumn<QString>("output");
		QTest::addColumn<VuoTableFormat>("format");

		{
			QTest::newRow("empty string") << "" << "" << VuoTableFormat_Csv;
		}
		{
			QTest::newRow("single row, CSV") << "a,b,c" << QUOTE("a","b","c") << VuoTableFormat_Csv;
		}
		{
			QTest::newRow("single row, TSV") << "a\tb\tc" << QUOTE("a"\t"b"\t"c") << VuoTableFormat_Tsv;
		}
		{
			QTest::newRow("single column") << "h1\nh2\nh3" << QUOTE("h1"\n"h2"\n"h3") << VuoTableFormat_Csv;
		}
		{
			QTest::newRow("multiple rows and columns, Unix-style line endings") << "a,b\nc,d\n" << QUOTE("a","b"\n"c","d") << VuoTableFormat_Csv;
		}
		{
			QTest::newRow("multiple rows and columns, Windows-style line endings") << "a,b\r\nc,d\r\n" << QUOTE("a","b"\n"c","d") << VuoTableFormat_Csv;
		}
		{
			QTest::newRow("column and row headers") << ",h1,h2,h3\nh4,a,b,c\nh5,d,e,f" << QUOTE(,"h1","h2","h3"\n"h4","a","b","c"\n"h5","d","e","f") << VuoTableFormat_Csv;
		}
		{
			QTest::newRow("different-sized rows") << "a\nb,c,d\ne,f" << QUOTE("a",,\n"b","c","d"\n"e","f",) << VuoTableFormat_Csv;
		}
		{
			QTest::newRow("different-sized columns") << ",h1\nh2,a\nh3" << QUOTE(,"h1"\n"h2","a"\n"h3",) << VuoTableFormat_Csv;
		}
		{
			QTest::newRow("empty rows") << "\na\n\nb\nc\n\n" << QUOTE("a"\n"b"\n"c") << VuoTableFormat_Csv;
		}
		{
			QTest::newRow("empty items, CSV") << ",a,,b,c,," << QUOTE(,"a",,"b","c",,) << VuoTableFormat_Csv;
		}
		{
			QTest::newRow("empty items, TSV") << "\ta\t\tb\tc\t\t" << QUOTE(\t"a"\t\t"b"\t"c"\t\t) << VuoTableFormat_Tsv;
		}
		{
			QTest::newRow("quoted items") << QUOTE("a","b""c","d,e") << QUOTE("a","b""c","d,e") << VuoTableFormat_Csv;
		}
		{
			QTest::newRow("UTF-8 items") << "⓪,\"①②\",③④⑤" << "\"⓪\",\"①②\",\"③④⑤\"" << VuoTableFormat_Csv;
		}
		{
			// "Leading and trailing spaces that are part of non-quoted fields are ignored as this is by far the
			// most common behavior and expected by many applications." (libcsv documentation)
			QTest::newRow("leading/trailing spaces") << QUOTE( , a,b , c ," d ", "e" ,) << QUOTE(,"a","b","c"," d ","e",) << VuoTableFormat_Csv;
		}
		{
			// "By default, however, libcsv will also attempt to parse malformed CSV data such as data containing
			// unescaped quotes or quotes within non-quoted fields. (libcsv documentation)
			QTest::newRow("quotes within a non-quoted field") << "a\"b,c" << QUOTE("a""b","c") << VuoTableFormat_Csv;
			QTest::newRow("unescaped quotes") << "\"a\"b\",c" << QUOTE("a""b","c") << VuoTableFormat_Csv;
		}
	}
	void testParseAndSerialize()
	{
		QFETCH(QString, input);
		QFETCH(QString, output);
		QFETCH(VuoTableFormat, format);

		VuoTable table = VuoTable_makeFromText(input.toUtf8().constData(), format);
		VuoText actualOutput = VuoTable_serialize(table, format);
		QCOMPARE(QString::fromUtf8(actualOutput), output);

		VuoTable_retain(table);
		VuoTable_release(table);
		VuoRetain(actualOutput);
		VuoRelease(actualOutput);
	}

	void testMakeFromJson_data()
	{
		QTest::addColumn<QString>("input");
		QTest::addColumn<QString>("output");

		{
			QTest::newRow("empty string") << "" << "";
		}
		{
			const char *csv = QUOTE("a","b"\n"c","d");
			VuoTable table = VuoTable_makeFromText(csv, VuoTableFormat_Csv);

			char *json = VuoTable_getString(table);
			QTest::newRow("no headers, in-process") << QString(json) << QString(csv);

			char *jsonInterprocess = VuoTable_getInterprocessString(table);
			QTest::newRow("no headers, interprocess") << QString(jsonInterprocess) << QString(csv);

			free(json);
			free(jsonInterprocess);
		}
		{
			const char *csv = QUOTE(,"h1","h2"\n"h3","a","b"\n"h4","c","d");
			VuoTable table = VuoTable_makeFromText(csv, VuoTableFormat_Csv);

			char *json = VuoTable_getString(table);
			QTest::newRow("headers, in-process") << QString(json) << QString(csv);

			char *jsonInterprocess = VuoTable_getInterprocessString(table);
			QTest::newRow("headers, interprocess") << QString(jsonInterprocess) << QString(csv);

			free(json);
			free(jsonInterprocess);
		}
	}
	void testMakeFromJson()
	{
		QFETCH(QString, input);
		QFETCH(QString, output);

		VuoTable table = VuoTable_makeFromString(input.toUtf8().constData());
		VuoText actualOutput = VuoTable_serialize(table, VuoTableFormat_Csv);
		QCOMPARE(QString::fromUtf8(actualOutput), output);

		VuoTable_retain(table);
		VuoTable_release(table);
		VuoRetain(actualOutput);
		VuoRelease(actualOutput);
	}

	void testSortInteger_data()
	{
		QTest::addColumn<QString>("input");
		QTest::addColumn<QString>("output");
		QTest::addColumn<int>("columnIndex");
		QTest::addColumn<VuoTextSort>("sortType");
		QTest::addColumn<VuoSortOrder>("sortOrder");
		QTest::addColumn<bool>("firstRowIsHeader");

		{
			QTest::newRow("empty table") << "" << "" << 1 << VuoTextSort_Text << VuoSortOrder_Ascending << false;
		}
		{
			QTest::newRow("single column") << QUOTE(cherry\napple\nbanana)
										   << QUOTE(apple\nbanana\ncherry)
										   << 1 << VuoTextSort_Text << VuoSortOrder_Ascending << false;
		}
		{
			QTest::newRow("no headers") << QUOTE(cherry,beans\napple,chipotle\nbanana,avocado)
										<< QUOTE(apple,chipotle\nbanana,avocado\ncherry,beans)
										<< 1 << VuoTextSort_Text << VuoSortOrder_Ascending << false;
		}
		{
			QTest::newRow("column headers") << QUOTE(date,amount,credit card\n2017.01.01,$2.00,xxxx-4444\n2017.01.03,$1.00,xxxx-6666\n2017.01.02,$3.00,xxxx-5555)
											<< QUOTE(date,amount,credit card\n2017.01.03,$1.00,xxxx-6666\n2017.01.01,$2.00,xxxx-4444\n2017.01.02,$3.00,xxxx-5555)
											<< 2 << VuoTextSort_Text << VuoSortOrder_Ascending << true;
		}
		{
			QTest::newRow("duplicate items") << QUOTE(Athens,OH\nLogan,OH\nVienna,WV\nMarietta,OH\nParkersburg,WV\nBelpre,OH)
											 << QUOTE(Athens,OH\nLogan,OH\nMarietta,OH\nBelpre,OH\nVienna,WV\nParkersburg,WV)
											 << 2 << VuoTextSort_Text << VuoSortOrder_Ascending << false;
		}
		{
			const char *orig = QUOTE(,eye color,dignified demeanor\nStrange,green,rarely\nOtto,yellow,very);
			const char *sorted = QUOTE(,eye color,dignified demeanor\nOtto,yellow,very\nStrange,green,rarely);
			QTest::newRow("zero index") << orig << sorted << 0 << VuoTextSort_Text << VuoSortOrder_Ascending << true;
			QTest::newRow("negative index") << orig << sorted << -1 << VuoTextSort_Text << VuoSortOrder_Ascending << true;
		}
		{
			QTest::newRow("index > column count") << QUOTE(,wears a bandana,rescues Timmy\nSimon,sometimes,never\nLassie,unknown,always)
												  << QUOTE(,wears a bandana,rescues Timmy\nLassie,unknown,always\nSimon,sometimes,never)
												  << 4 << VuoTextSort_Text << VuoSortOrder_Ascending << true;
		}
		{
			QTest::newRow("empty items") << QUOTE(,cups of coffee,donuts\n,5,\nBoss,4,\nEmployee,,6)
										 << QUOTE(,cups of coffee,donuts\nBoss,4,\n,5,\nEmployee,,6)
										 << 2 << VuoTextSort_Text << VuoSortOrder_Ascending << true;
		}
		{
			QTest::newRow("descending") << QUOTE(cherry,beans\napple,chipotle\nbanana,avocado)
										<< QUOTE(cherry,beans\nbanana,avocado\napple,chipotle)
										<< 1 << VuoTextSort_Text << VuoSortOrder_Descending << false;
		}
		{
			QTest::newRow("case-insensitive") << QUOTE(Hobby\nbinge watching\nSCUBA diving\nAmerican football\nzoology)
											  << QUOTE(Hobby\nAmerican football\nbinge watching\nSCUBA diving\nzoology)
											  << 1 << VuoTextSort_Text << VuoSortOrder_Ascending << true;
		}
		{
			QTest::newRow("case-sensitive") << QUOTE(Hobby\nbinge watching\nSCUBA diving\nAmerican football\nzoology)
											<< QUOTE(Hobby\nAmerican football\nSCUBA diving\nbinge watching\nzoology)
											<< 1 << VuoTextSort_TextCaseSensitive << VuoSortOrder_Ascending << true;
		}
		{
			QTest::newRow("integers") << QUOTE(Cat,Number of toes\nJake,28\nFluffy,18\nBitey,20\nDr. Claw,26\nAnticat,-19\nNegacat,-20\nSandy Claws,2)
									  << QUOTE(Cat,Number of toes\nJake,28\nDr. Claw,26\nBitey,20\nFluffy,18\nSandy Claws,2\nAnticat,-19\nNegacat,-20)
									  << 2 << VuoTextSort_Number << VuoSortOrder_Descending << true;
		}
		{
			QTest::newRow("decimals") << QUOTE(Odometer,Miles per gallon\n10000,44.5\n20000,62.4\n30000,102.8\n40000,39.10)
									  << QUOTE(Odometer,Miles per gallon\n40000,39.10\n10000,44.5\n20000,62.4\n30000,102.8)
									  << 2 << VuoTextSort_Number << VuoSortOrder_Ascending << true;
		}
		{
			vector<VuoTime> times;
			times.push_back( VuoTime_make(1911, 01, 01, 13, 0, 0) );
			times.push_back( VuoTime_make(1910, 12, 31, 12, 0, 0) );
			times.push_back( VuoTime_make(1913, 01, 01, 0, 0, 0) );
			times.push_back( VuoTime_make(1912, 01, 01, 0, 0, 0) );
			times.push_back( VuoTime_make(1910, 12, 31, 0, 0, 0) );

			vector<VuoTimeFormat> formats;
			formats.push_back(VuoTimeFormat_DateTimeSortable);
			formats.push_back(VuoTimeFormat_DateTimeShort24);
			formats.push_back(VuoTimeFormat_DateShort);
			formats.push_back(VuoTimeFormat_DateLong);
			formats.push_back(VuoTimeFormat_Time12);

			vector<const char *> descriptions;
			descriptions.push_back("Date and time, sortable");
			descriptions.push_back("Date and time, short 24-hour");
			descriptions.push_back("Date, short");
			descriptions.push_back("Date, long");
			descriptions.push_back("Time, 12-hour");

			vector<string> summaries(formats.size());
			for (int i = 0; i < formats.size(); ++i)
			{
				char *summary = VuoTimeFormat_getSummary(formats[i]);
				summaries[i] = summary;
				free(summary);
			}

			vector< vector<string> > formattedTimes(formats.size());
			for (int i = 0; i < formats.size(); ++i)
			{
				formattedTimes[i].resize(times.size());
				for (int j = 0; j < times.size(); ++j)
				{
					VuoText t = VuoTime_format(times[j], formats[i]);
					formattedTimes[i][j] = t;
					VuoRetain(t);
					VuoRelease(t);
				}
			}

			vector<string> orig(formats.size());
			for (int i = 0; i < formats.size(); ++i)
			{
				orig[i] += "\"" + summaries[i] + "\"\n";
				for (int j = 0; j < times.size(); ++j)
					orig[i] += "\"" + formattedTimes[i][j] + "\"\n";
			}

			vector<string> sorted;
			sorted.push_back(summaries[0] + "\n" + formattedTimes[0][4] + "\n" + formattedTimes[0][1] + "\n" + formattedTimes[0][0] + "\n" + formattedTimes[0][3] + "\n" + formattedTimes[0][2]);
			sorted.push_back(summaries[1] + "\n" + formattedTimes[1][4] + "\n" + formattedTimes[1][1] + "\n" + formattedTimes[1][0] + "\n" + formattedTimes[1][3] + "\n" + formattedTimes[1][2]);
			sorted.push_back(summaries[2] + "\n" + formattedTimes[2][1] + "\n" + formattedTimes[2][4] + "\n" + formattedTimes[2][0] + "\n" + formattedTimes[2][3] + "\n" + formattedTimes[2][2]);
			sorted.push_back(summaries[3] + "\n" + formattedTimes[3][1] + "\n" + formattedTimes[3][4] + "\n" + formattedTimes[3][0] + "\n" + formattedTimes[3][3] + "\n" + formattedTimes[3][2]);
			sorted.push_back(summaries[4] + "\n" + formattedTimes[4][2] + "\n" + formattedTimes[4][3] + "\n" + formattedTimes[4][4] + "\n" + formattedTimes[4][1] + "\n" + formattedTimes[4][0]);

			for (int i = 0; i < formats.size(); ++i)
			{
				QTest::newRow(descriptions[i]) << orig[i].c_str() << sorted[i].c_str()
											   << 1 << VuoTextSort_Date << VuoSortOrder_Ascending << true;
			}
		}
		{
			QTest::newRow("Date, Excel format")
				<< QUOTE(Alaska,-62.2,1/23/71\nArgentina,-32.8,6/1/07\nAustralia,-23,6/29/94\nEngland,-26.1,1/10/82\nMalaysia,7.8,4/11/78\nPanama,2,2/20/95\nSamoa,11.1,9/29/71\nSiberia,-67.8,2/6/33\nSouth Africa,-20.1,8/23/13\nSouth Pole,-82.8,6/23/82)
				<< QUOTE(Alaska,-62.2,1/23/71\nSamoa,11.1,9/29/71\nMalaysia,7.8,4/11/78\nEngland,-26.1,1/10/82\nSouth Pole,-82.8,6/23/82\nAustralia,-23,6/29/94\nPanama,2,2/20/95\nArgentina,-32.8,6/1/07\nSouth Africa,-20.1,8/23/13\nSiberia,-67.8,2/6/33)
				<< 3 << VuoTextSort_Date << VuoSortOrder_Ascending << false;
		}
		{
			const char *orig = QUOTE(A,,B\nC,,D);
			QTest::newRow("null items as text") << orig << orig << 2 << VuoTextSort_Text << VuoSortOrder_Ascending << false;
			QTest::newRow("null items as case-sensitive text") << orig << orig << 2 << VuoTextSort_TextCaseSensitive << VuoSortOrder_Ascending << false;
			QTest::newRow("null items as numbers") << orig << orig << 2 << VuoTextSort_Number << VuoSortOrder_Ascending << false;
			QTest::newRow("null items as dates") << orig << orig << 2 << VuoTextSort_Date << VuoSortOrder_Ascending << false;
		}
	}
	void testSortInteger()
	{
		QFETCH(QString, input);
		QFETCH(QString, output);
		QFETCH(int, columnIndex);
		QFETCH(VuoTextSort, sortType);
		QFETCH(VuoSortOrder, sortOrder);
		QFETCH(bool, firstRowIsHeader);

		VuoTable table = VuoTable_makeFromText(input.toUtf8().constData(), VuoTableFormat_Csv);
		VuoTable sortedTable = VuoTable_sort_VuoInteger(table, columnIndex, sortType, sortOrder, firstRowIsHeader);
		VuoText actualOutputQuoted = VuoTable_serialize(sortedTable, VuoTableFormat_Csv);
		VuoText actualOutput = VuoText_replace(actualOutputQuoted, "\"", "");
		QCOMPARE(QString::fromUtf8(actualOutput), output);

		VuoTable_retain(table);
		VuoTable_retain(sortedTable);
		VuoRetain(actualOutputQuoted);
		VuoRetain(actualOutput);

		VuoTable_release(table);
		VuoTable_release(sortedTable);
		VuoRelease(actualOutputQuoted);
		VuoRelease(actualOutput);
	}

	void testSortText_data()
	{
		QTest::addColumn<QString>("input");
		QTest::addColumn<QString>("output");
		QTest::addColumn<QString>("columnHeader");
		QTest::addColumn<bool>("firstRowIsHeader");

		{
			QTest::newRow("empty table") << "" << "" << "test" << false;
		}
		{
			QTest::newRow("no headers") << QUOTE(cherry,beans\napple,chipotle\nbanana,avocado)
										<< QUOTE(cherry,beans\napple,chipotle\nbanana,avocado)
										<< "test" << false;
		}
		{
			const char *orig   = QUOTE(rate,time,distance\n10,3,30\n20,1,20\n40,2,80);
			const char *sorted = QUOTE(rate,time,distance\n20,1,20\n40,2,80\n10,3,30);

			QTest::newRow("exact match") << orig << sorted << "time" << true;

			QTest::newRow("case-insensitive match") << orig << sorted << "TIME" << true;

			QTest::newRow("substring match") << orig << sorted << "im" << true;

			QTest::newRow("multiple matches") << orig << sorted << "i" << true;

			QTest::newRow("no match") << orig << orig << "times" << true;

			QTest::newRow("empty target header") << orig << orig << "" << true;
		}
	}
	void testSortText()
	{
		QFETCH(QString, input);
		QFETCH(QString, output);
		QFETCH(QString, columnHeader);
		QFETCH(bool, firstRowIsHeader);

		VuoTable table = VuoTable_makeFromText(input.toUtf8().constData(), VuoTableFormat_Csv);
		VuoTable sortedTable = VuoTable_sort_VuoText(table, columnHeader.toUtf8().constData(), VuoTextSort_TextCaseSensitive, VuoSortOrder_Ascending, firstRowIsHeader);
		VuoText actualOutputQuoted = VuoTable_serialize(sortedTable, VuoTableFormat_Csv);
		VuoText actualOutput = VuoText_replace(actualOutputQuoted, "\"", "");
		QCOMPARE(QString::fromUtf8(actualOutput), output);

		VuoTable_retain(table);
		VuoTable_retain(sortedTable);
		VuoRetain(actualOutputQuoted);
		VuoRetain(actualOutput);

		VuoTable_release(table);
		VuoTable_release(sortedTable);
		VuoRelease(actualOutputQuoted);
		VuoRelease(actualOutput);
	}

	void testTranspose_data()
	{
		QTest::addColumn<QString>("input");
		QTest::addColumn<QString>("output");

		{
			QTest::newRow("empty table") << "" << "";
		}
		{
			QTest::newRow("no headers") << QUOTE(Tokyo,37832892\nShanghai,34000000\nJakarta,31689592\nSeoul,25514000)
										<< QUOTE(Tokyo,Shanghai,Jakarta,Seoul\n37832892,34000000,31689592,25514000);
		}
		{
			QTest::newRow("column and row headers") << ",Toothbrush,Pajamas,Helmet\nFavorite Kid,✓,,✓\nAnnoying Kid,✓,,"
													<< ",Favorite Kid,Annoying Kid\nToothbrush,✓,✓\nPajamas,,\nHelmet,✓,";
		}
		{
			QTest::newRow("empty cells") << QUOTE(Station,,Line\nGreenbelt,,Green\nFort Totten,,Green,Red\nGallery Place/Chinatown,,Green,Red,Yellow)
										 << QUOTE(Station,Greenbelt,Fort Totten,Gallery Place/Chinatown\n,,,\nLine,Green,Green,Green\n,,Red,Red\n,,,Yellow);
		}
	}
	void testTranspose()
	{
		QFETCH(QString, input);
		QFETCH(QString, output);

		VuoTable table = VuoTable_makeFromText(input.toUtf8().constData(), VuoTableFormat_Csv);
		VuoTable transposedTable = VuoTable_transpose(table);
		VuoText actualOutputQuoted = VuoTable_serialize(transposedTable, VuoTableFormat_Csv);
		VuoText actualOutput = VuoText_replace(actualOutputQuoted, "\"", "");
		QCOMPARE(QString::fromUtf8(actualOutput), output);

		VuoTable_retain(table);
		VuoTable_retain(transposedTable);
		VuoRetain(actualOutputQuoted);
		VuoRetain(actualOutput);

		VuoTable_release(table);
		VuoTable_release(transposedTable);
		VuoRelease(actualOutputQuoted);
		VuoRelease(actualOutput);
	}

	void testAddRow_data()
	{
		QTest::addColumn<QString>("input");
		QTest::addColumn<QString>("row");
		QTest::addColumn<VuoListPosition>("position");
		QTest::addColumn<QString>("output");

		{
			const char *row = QUOTE(1.0,2.0,3.0,4.0);

			QTest::newRow("empty table, beginning") << "" << row << VuoListPosition_Beginning << row;
			QTest::newRow("empty table, end") << "" << row << VuoListPosition_End << row;
		}
		{
			const char *table = QUOTE(a,b,c\nd,e,f);
			const char *row = QUOTE(g,h,i);

			QTest::newRow("multiple rows and columns, beginning") << table << row << VuoListPosition_Beginning << QString(row).append("\n").append(table);
			QTest::newRow("multiple rows and columns, end") << table << row << VuoListPosition_End << QString(table).append("\n").append(row);
		}
		{
			QTest::newRow("empty cells") << QUOTE(a,b,c\nd,e\nf) << QUOTE(g,h,i) << VuoListPosition_End
										 << QUOTE(a,b,c\nd,e,\nf,,\ng,h,i);
		}
		{
			QTest::newRow("row shorter than table") << QUOTE(a,b,c) << QUOTE(d) << VuoListPosition_End
													<< QUOTE(a,b,c\nd,,);
		}
		{
			QTest::newRow("row longer than table") << QUOTE(a,b,c) << QUOTE(d,e,f,g,h) << VuoListPosition_End
												   << QUOTE(a,b,c,,\nd,e,f,g,h);
		}
	}
	void testAddRow()
	{
		QFETCH(QString, input);
		QFETCH(QString, row);
		QFETCH(VuoListPosition, position);
		QFETCH(QString, output);

		VuoTable rowTable = VuoTable_makeFromText(row.toUtf8().constData(), VuoTableFormat_Csv);
		VuoList_VuoText rowList = VuoTable_getRow_VuoInteger(rowTable, 1, true);

		VuoTable table = VuoTable_makeFromText(input.toUtf8().constData(), VuoTableFormat_Csv);
		VuoTable modifiedTable = VuoTable_addRow(table, position, rowList);
		VuoText actualOutputQuoted = VuoTable_serialize(modifiedTable, VuoTableFormat_Csv);
		VuoText actualOutput = VuoText_replace(actualOutputQuoted, "\"", "");
		QCOMPARE(QString::fromUtf8(actualOutput), output);

		VuoTable_retain(rowTable);
		VuoRetain(rowList);
		VuoTable_retain(table);
		VuoTable_retain(modifiedTable);
		VuoRetain(actualOutputQuoted);
		VuoRetain(actualOutput);

		VuoTable_release(rowTable);
		VuoRelease(rowList);
		VuoTable_release(table);
		VuoTable_release(modifiedTable);
		VuoRelease(actualOutputQuoted);
		VuoRelease(actualOutput);
	}

	void testAddColumn_data()
	{
		QTest::addColumn<QString>("input");
		QTest::addColumn<QString>("column");
		QTest::addColumn<VuoListPosition>("position");
		QTest::addColumn<QString>("output");

		{
			const char *column = QUOTE(1.0\n2.0\n3.0\n4.0);

			QTest::newRow("empty table, beginning") << "" << column << VuoListPosition_Beginning << column;
			QTest::newRow("empty table, end") << "" << column << VuoListPosition_End << column;
		}
		{
			const char *table = QUOTE(a,b\nc,d\ne,f);
			const char *column = QUOTE(g\nh\ni);

			QTest::newRow("multiple rows and columns, beginning") << table << column << VuoListPosition_Beginning
																  << QUOTE(g,a,b\nh,c,d\ni,e,f);

			QTest::newRow("multiple rows and columns, end") << table << column << VuoListPosition_End
															<< QUOTE(a,b,g\nc,d,h\ne,f,i);
		}
		{
			QTest::newRow("empty cells") << QUOTE(a,b,c\nd,e\nf) << QUOTE(g\nh\ni) << VuoListPosition_End
										 << QUOTE(a,b,c,g\nd,e,,h\nf,,,i);
		}
		{
			QTest::newRow("column shorter than table") << QUOTE(a\nb\nc) << QUOTE(d) << VuoListPosition_End
													   << QUOTE(a,d\nb,\nc,);
		}
		{
			QTest::newRow("column longer than table") << QUOTE(a\nb\nc) << QUOTE(d\ne\nf\ng\nh) << VuoListPosition_End
													  << QUOTE(a,d\nb,e\nc,f\n,g\n,h);
		}
	}
	void testAddColumn()
	{
		QFETCH(QString, input);
		QFETCH(QString, column);
		QFETCH(VuoListPosition, position);
		QFETCH(QString, output);

		VuoTable columnTable = VuoTable_makeFromText(column.toUtf8().constData(), VuoTableFormat_Csv);
		VuoList_VuoText columnList = VuoTable_getColumn_VuoInteger(columnTable, 1, true);

		VuoTable table = VuoTable_makeFromText(input.toUtf8().constData(), VuoTableFormat_Csv);
		VuoTable modifiedTable = VuoTable_addColumn(table, position, columnList);
		VuoText actualOutputQuoted = VuoTable_serialize(modifiedTable, VuoTableFormat_Csv);
		VuoText actualOutput = VuoText_replace(actualOutputQuoted, "\"", "");
		QCOMPARE(QString::fromUtf8(actualOutput), output);

		VuoTable_retain(columnTable);
		VuoRetain(columnList);
		VuoTable_retain(table);
		VuoTable_retain(modifiedTable);
		VuoRetain(actualOutputQuoted);
		VuoRetain(actualOutput);

		VuoTable_release(columnTable);
		VuoRelease(columnList);
		VuoTable_release(table);
		VuoTable_release(modifiedTable);
		VuoRelease(actualOutputQuoted);
		VuoRelease(actualOutput);
	}

	void testChangeRow_data()
	{
		QTest::addColumn<QString>("input");
		QTest::addColumn<QString>("newValues");
		QTest::addColumn<int>("rowIndex");
		QTest::addColumn<bool>("preserveHeader");
		QTest::addColumn<QString>("output");

		{
			const char *row = QUOTE(a,b,c,d);

			QTest::newRow("empty table") << "" << row << 1 << false << "";
			QTest::newRow("empty table, header") << "" << row << 1 << true << "";
		}
		{
			const char *table = QUOTE(a,b,c\nd,e,f);

			QTest::newRow("multiple rows and columns") << table << QUOTE(g,h,i) << 2 << false
													   << QUOTE(a,b,c\ng,h,i);

			QTest::newRow("multiple rows and columns, header") << table << QUOTE(h,i) << 2 << true
															   << QUOTE(a,b,c\nd,h,i);
		}
		{
			const char *table = QUOTE(a,b,c\nd,e\nf);

			QTest::newRow("empty cells") << table << QUOTE(g,h,i) << 2 << false
										 << QUOTE(a,b,c\ng,h,i\nf,,);

			QTest::newRow("empty cells, header") << table << QUOTE(h,i) << 2 << true
												 << QUOTE(a,b,c\nd,h,i\nf,,);
		}
		{
			const char *table = QUOTE(a,b,c,d\ne,f,g);

			QTest::newRow("row shorter than table") << table << QUOTE(h,i) << 1 << false
													<< QUOTE(h,i,,\ne,f,g,);

			QTest::newRow("row shorter than table, header") << table << QUOTE(i) << 1 << true
															<< QUOTE(a,i,,\ne,f,g,);
		}
		{
			const char *table = QUOTE(a,b,c\nd,e,f);

			QTest::newRow("row longer than table") << table << QUOTE(g,h,i,j) << 1 << false
												   << QUOTE(g,h,i,j\nd,e,f,);

			QTest::newRow("row longer than table, header") << table << QUOTE(h,i,j) << 1 << true
														   << QUOTE(a,h,i,j\nd,e,f,);
		}
	}
	void testChangeRow()
	{
		QFETCH(QString, input);
		QFETCH(QString, newValues);
		QFETCH(int, rowIndex);
		QFETCH(bool, preserveHeader);
		QFETCH(QString, output);

		VuoTable rowTable = VuoTable_makeFromText(newValues.toUtf8().constData(), VuoTableFormat_Csv);
		VuoTable_retain(rowTable);

		VuoList_VuoText rowList = VuoTable_getRow_VuoInteger(rowTable, 1, true);
		VuoRetain(rowList);
		VuoTable_release(rowTable);

		VuoTable table = VuoTable_makeFromText(input.toUtf8().constData(), VuoTableFormat_Csv);
		VuoTable_retain(table);

		VuoTable modifiedTable = VuoTable_changeRow_VuoInteger(table, rowIndex, rowList, preserveHeader);
		VuoTable_retain(modifiedTable);
		VuoTable_release(table);
		VuoRelease(rowList);

		VuoText actualOutputQuoted = VuoTable_serialize(modifiedTable, VuoTableFormat_Csv);
		VuoRetain(actualOutputQuoted);
		VuoTable_release(modifiedTable);

		VuoText actualOutput = VuoText_replace(actualOutputQuoted, "\"", "");
		VuoRetain(actualOutput);
		VuoRelease(actualOutputQuoted);

		QCOMPARE(QString::fromUtf8(actualOutput), output);
		VuoRelease(actualOutput);
	}

	void testChangeColumn_data()
	{
		QTest::addColumn<QString>("input");
		QTest::addColumn<QString>("newValues");
		QTest::addColumn<int>("columnIndex");
		QTest::addColumn<bool>("preserveHeader");
		QTest::addColumn<QString>("output");

		{
			const char *row = QUOTE(a\nb\nc\nd);

			QTest::newRow("empty table") << "" << row << 1 << false << "";
			QTest::newRow("empty table, header") << "" << row << 1 << true << "";
		}
		{
			const char *table = QUOTE(a,b\nc,d\ne,f);

			QTest::newRow("multiple rows and columns") << table << QUOTE(g\nh\ni) << 2 << false
													   << QUOTE(a,g\nc,h\ne,i);

			QTest::newRow("multiple rows and columns, header") << table << QUOTE(h\ni) << 2 << true
															   << QUOTE(a,b\nc,h\ne,i);
		}
		{
			const char *table = QUOTE(a,b,c\nd,e\nf);

			QTest::newRow("empty cells") << table << QUOTE(g\nh\ni) << 2 << false
										 << QUOTE(a,g,c\nd,h,\nf,i,);

			QTest::newRow("empty cells, header") << table << QUOTE(h\ni) << 2 << true
												 << QUOTE(a,b,c\nd,h,\nf,i,);
		}
		{
			QTest::newRow("empty header cell, header") << QUOTE(a\nb,c) << QUOTE(d) << 2 << true
													   << QUOTE(a,\nb,d);
		}
		{
			const char *table = QUOTE(a,b\nc,d\ne,f\ng);

			QTest::newRow("column shorter than table") << table << QUOTE(h\ni) << 1 << false
													   << QUOTE(h,b\ni,d\n,f\n,);

			QTest::newRow("column shorter than table, header") << table << QUOTE(i) << 1 << true
															   << QUOTE(a,b\ni,d\n,f\n,);
		}
		{
			const char *table = QUOTE(a,b\nc,d\ne,f);

			QTest::newRow("column longer than table") << table << QUOTE(g\nh\ni\nj) << 1 << false
													  << QUOTE(g,b\nh,d\ni,f\nj,);

			QTest::newRow("column longer than table, header") << table << QUOTE(h\ni\nj) << 1 << true
															  << QUOTE(a,b\nh,d\ni,f\nj,);
		}
	}
	void testChangeColumn()
	{
		QFETCH(QString, input);
		QFETCH(QString, newValues);
		QFETCH(int, columnIndex);
		QFETCH(bool, preserveHeader);
		QFETCH(QString, output);

		VuoTable columnTable = VuoTable_makeFromText(newValues.toUtf8().constData(), VuoTableFormat_Csv);
		VuoTable_retain(columnTable);

		VuoList_VuoText columnList = VuoTable_getColumn_VuoInteger(columnTable, 1, true);
		VuoRetain(columnList);
		VuoTable_release(columnTable);

		VuoTable table = VuoTable_makeFromText(input.toUtf8().constData(), VuoTableFormat_Csv);
		VuoTable_retain(table);

		VuoTable modifiedTable = VuoTable_changeColumn_VuoInteger(table, columnIndex, columnList, preserveHeader);
		VuoTable_retain(modifiedTable);
		VuoTable_release(table);
		VuoRelease(columnList);

		VuoText actualOutputQuoted = VuoTable_serialize(modifiedTable, VuoTableFormat_Csv);
		VuoRetain(actualOutputQuoted);
		VuoTable_release(modifiedTable);

		VuoText actualOutput = VuoText_replace(actualOutputQuoted, "\"", "");
		VuoRetain(actualOutput);
		VuoRelease(actualOutputQuoted);

		QCOMPARE(QString::fromUtf8(actualOutput), output);
		VuoRelease(actualOutput);
	}

	void testRemoveRow_data()
	{
		QTest::addColumn<QString>("input");
		QTest::addColumn<VuoListPosition>("position");
		QTest::addColumn<QString>("output");

		{
			QTest::newRow("empty table") << "" << VuoListPosition_Beginning << "";
		}
		{
			const char *table = QUOTE(a,b,c\nd,e,f\ng,h,i);

			QTest::newRow("multiple rows and columns, beginning") << table << VuoListPosition_Beginning << QUOTE(d,e,f\ng,h,i);
			QTest::newRow("multiple rows and columns, end") << table << VuoListPosition_End << QUOTE(a,b,c\nd,e,f);
		}
		{
			const char *table = QUOTE(a,b,c\nd,e\nf);

			QTest::newRow("empty cells") << table << VuoListPosition_End << QUOTE(a,b,c\nd,e,);
			QTest::newRow("longest row") << table << VuoListPosition_Beginning << QUOTE(d,e,\nf,,);
		}
	}
	void testRemoveRow()
	{
		QFETCH(QString, input);
		QFETCH(VuoListPosition, position);
		QFETCH(QString, output);

		VuoTable table = VuoTable_makeFromText(input.toUtf8().constData(), VuoTableFormat_Csv);
		VuoTable_retain(table);

		VuoTable modifiedTable = VuoTable_removeRow(table, position);
		VuoTable_retain(modifiedTable);
		VuoTable_release(table);

		VuoText actualOutputQuoted = VuoTable_serialize(modifiedTable, VuoTableFormat_Csv);
		VuoRetain(actualOutputQuoted);
		VuoTable_release(modifiedTable);

		VuoText actualOutput = VuoText_replace(actualOutputQuoted, "\"", "");
		VuoRetain(actualOutput);
		VuoRelease(actualOutputQuoted);

		QCOMPARE(QString::fromUtf8(actualOutput), output);
		VuoRelease(actualOutput);
	}

	void testRemoveColumn_data()
	{
		QTest::addColumn<QString>("input");
		QTest::addColumn<VuoListPosition>("position");
		QTest::addColumn<QString>("output");

		{
			QTest::newRow("empty table") << "" << VuoListPosition_Beginning << "";
		}
		{
			const char *table = QUOTE(a,b,c\nd,e,f\ng,h,i);

			QTest::newRow("multiple rows and columns, beginning") << table << VuoListPosition_Beginning << QUOTE(b,c\ne,f\nh,i);
			QTest::newRow("multiple rows and columns, end") << table << VuoListPosition_End << QUOTE(a,b\nd,e\ng,h);
		}
		{
			const char *table = QUOTE(a,b,c\nd,e\nf);

			QTest::newRow("empty cells") << table << VuoListPosition_End << QUOTE(a,b\nd,e\nf,);
			QTest::newRow("longest column") << table << VuoListPosition_Beginning << QUOTE(b,c\ne,\n,);
		}
	}
	void testRemoveColumn()
	{
		QFETCH(QString, input);
		QFETCH(VuoListPosition, position);
		QFETCH(QString, output);

		VuoTable table = VuoTable_makeFromText(input.toUtf8().constData(), VuoTableFormat_Csv);
		VuoTable_retain(table);

		VuoTable modifiedTable = VuoTable_removeColumn(table, position);
		VuoTable_retain(modifiedTable);
		VuoTable_release(table);

		VuoText actualOutputQuoted = VuoTable_serialize(modifiedTable, VuoTableFormat_Csv);
		VuoRetain(actualOutputQuoted);
		VuoTable_release(modifiedTable);

		VuoText actualOutput = VuoText_replace(actualOutputQuoted, "\"", "");
		VuoRetain(actualOutput);
		VuoRelease(actualOutputQuoted);

		QCOMPARE(QString::fromUtf8(actualOutput), output);
		VuoRelease(actualOutput);
	}

	void testFindFirstMatchingRow_data()
	{
		QTest::addColumn<QString>("input");
		QTest::addColumn<int>("columnIndex");
		QTest::addColumn<QString>("columnHeader");
		QTest::addColumn<QString>("valueToFind");
		QTest::addColumn<VuoTextComparison>("valueComparison");
		QTest::addColumn<bool>("includeHeader");
		QTest::addColumn<QString>("expectedRow");

		QString table = QUOTE(a,b,c\nd,e,f\ng,h,i\nalpha,beta,gamma);

		QTest::newRow("empty table")                              << ""    << 0 << ""  << ""   << (VuoTextComparison){VuoTextComparison_Equals,   false} << false << QUOTE(null);

		// Column 0 (out of range) should be clamped to 1.
		QTest::newRow("table, column 0 equals ''")                << table << 0 << ""  << ""   << (VuoTextComparison){VuoTextComparison_Equals,   false} << false << QUOTE(null);
		QTest::newRow("table, column 0 equals 'g', header")       << table << 0 << "a" << "g"  << (VuoTextComparison){VuoTextComparison_Equals,   false} << true  << QUOTE(["g","h","i"]);
		QTest::newRow("table, column 0 equals 'g', no header")    << table << 0 << "a" << "g"  << (VuoTextComparison){VuoTextComparison_Equals,   false} << false << QUOTE(["h","i"]);
		QTest::newRow("table, column 0 contains '', header")      << table << 0 << "a" << ""   << (VuoTextComparison){VuoTextComparison_Contains, false} << true  << QUOTE(["a","b","c"]);
		QTest::newRow("table, column 0 contains '', no header")   << table << 0 << "a" << ""   << (VuoTextComparison){VuoTextComparison_Contains, false} << false << QUOTE(["b","c"]);
		QTest::newRow("table, column 0 contains 'g', header")     << table << 0 << "a" << "g"  << (VuoTextComparison){VuoTextComparison_Contains, false} << true  << QUOTE(["g","h","i"]);
		QTest::newRow("table, column 0 contains 'g', no header")  << table << 0 << "a" << "g"  << (VuoTextComparison){VuoTextComparison_Contains, false} << false << QUOTE(["h","i"]);

		QTest::newRow("table, column 1 equals 'g', header")       << table << 1 << "a" << "g"  << (VuoTextComparison){VuoTextComparison_Equals,   false} << true  << QUOTE(["g","h","i"]);
		QTest::newRow("table, column 1 equals 'g', no header")    << table << 1 << "a" << "g"  << (VuoTextComparison){VuoTextComparison_Equals,   false} << false << QUOTE(["h","i"]);

		QTest::newRow("table, column 2 equals 'z'")               << table << 2 << "b" << "z"  << (VuoTextComparison){VuoTextComparison_Equals,   false} << true  << QUOTE(null);
		QTest::newRow("table, column 2 equals 'e', header")       << table << 2 << "b" << "e"  << (VuoTextComparison){VuoTextComparison_Equals,   false} << true  << QUOTE(["d","e","f"]);
		QTest::newRow("table, column 2 equals 'e', no header")    << table << 2 << "b" << "e"  << (VuoTextComparison){VuoTextComparison_Equals,   false} << false << QUOTE(["e","f"]);

		QTest::newRow("table, column 2 contains 'zz'")            << table << 2 << "b" << "zz" << (VuoTextComparison){VuoTextComparison_Contains, false} << true  << QUOTE(null);
		QTest::newRow("table, column 2 contains 'et', header")    << table << 2 << "b" << "et" << (VuoTextComparison){VuoTextComparison_Contains, false} << true  << QUOTE(["alpha","beta","gamma"]);
		QTest::newRow("table, column 2 contains 'et', no header") << table << 2 << "b" << "et" << (VuoTextComparison){VuoTextComparison_Contains, false} << false << QUOTE(["beta","gamma"]);

		// Column 4 (out of range) should be clamped to 3.
		QTest::newRow("table, column 4 equals ''")                << table << 4 << "c" << ""   << (VuoTextComparison){VuoTextComparison_Equals,   false} << false << QUOTE(null);
		QTest::newRow("table, column 4 equals 'i', header")       << table << 4 << "c" << "i"  << (VuoTextComparison){VuoTextComparison_Equals,   false} << true  << QUOTE(["g","h","i"]);
		QTest::newRow("table, column 4 equals 'i', no header")    << table << 4 << "c" << "i"  << (VuoTextComparison){VuoTextComparison_Equals,   false} << false << QUOTE(["h","i"]);
	}
	void testFindFirstMatchingRow()
	{
		QFETCH(QString, input);
		QFETCH(int, columnIndex);
		QFETCH(QString, columnHeader);
		QFETCH(QString, valueToFind);
		QFETCH(VuoTextComparison, valueComparison);
		QFETCH(bool, includeHeader);
		QFETCH(QString, expectedRow);

		VuoTable table = VuoTable_makeFromText(input.toUtf8().constData(), VuoTableFormat_Csv);
		VuoTable_retain(table);

		VuoList_VuoText foundRowByColumnIndex  = VuoTable_findFirstMatchingRow_VuoInteger(table, columnIndex,                  valueToFind.toUtf8().data(), valueComparison, includeHeader);
		VuoList_VuoText foundRowByColumnHeader = VuoTable_findFirstMatchingRow_VuoText   (table, columnHeader.toUtf8().data(), valueToFind.toUtf8().data(), valueComparison, includeHeader);

		VuoLocal(foundRowByColumnIndex);
		VuoLocal(foundRowByColumnHeader);

		VuoTable_release(table);

		QCOMPARE(QString::fromUtf8(VuoList_VuoText_getString(foundRowByColumnIndex)),  expectedRow);
		QCOMPARE(QString::fromUtf8(VuoList_VuoText_getString(foundRowByColumnHeader)), expectedRow);
	}

};

QTEST_APPLESS_MAIN(TestVuoTable)

#include "TestVuoTable.moc"

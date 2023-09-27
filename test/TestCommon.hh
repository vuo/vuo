/**
 * @file
 * TestCommon interface + implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include <string>
#include <vector>
using namespace std;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wunreachable-code"
#pragma clang diagnostic ignored "-Winvalid-constexpr"
#include <QtTest/QtTest>
#pragma clang diagnostic pop

#include "VuoStringUtilities.hh"

/**
 * Static methods shared between multiple test modules.
 */
class TestCommon
{
public:
	/**
	 * Executes `command` and provides its output, split into lines, in `linesFromCommand`.
	 *
	 * If the command exits with nonzero status, marks the test as failed.
	 */
	static void executeCommand(string command, vector<string> &linesFromCommand)
	{
		linesFromCommand.clear();

		do
		{
			FILE *outputFromCommand = popen(command.c_str(), "r");
			QVERIFY(outputFromCommand);

			char *lineWithoutNullTerminator = nullptr;
			size_t len = 0;
			while ((lineWithoutNullTerminator = fgetln(outputFromCommand, &len)))
			{
				string line(lineWithoutNullTerminator, len);
				if (line.find("No such file or directory") != string::npos)
					QFAIL(qPrintable(QString("Command \"%1\" failed: %2")
						.arg(QString::fromStdString(command))
						.arg(QString::fromStdString(line))));
				linesFromCommand.push_back(line);
			}

			if (lineWithoutNullTerminator)
				free(lineWithoutNullTerminator);

			int exitStatus = pclose(outputFromCommand);
			if (exitStatus)
			{
				for (string line : linesFromCommand)
					fprintf(stderr, "%s\n", line.c_str());
				QFAIL(qPrintable(QString("Command \"%1\" exited with status %2")
					.arg(QString::fromStdString(command))
					.arg(exitStatus)));
			}
		} while (linesFromCommand.empty());
	}

	/**
	 * Runs `otool -L` on the binary to find its dylib dependencies,
	 * then invokes a callback on each dependency.
	 *
	 * If the binary contains multiple architectures,
	 * the callback is invoked for each architecture's dylib dependencies.
	 */
	static void doForEachDylibDependency(string binaryPath, std::function<void(const string &dependencyPath)> doForDependency)
	{
		string command{"otool -L \"" + binaryPath + "\""};
		vector<string> linesFromCommand;
		TestCommon::executeCommand(command, linesFromCommand);

		QVERIFY2(linesFromCommand.size() >= 1,
			qPrintable(QString("The output of `%1` should be a dependency list, but instead it output \"%2\"")
				.arg(QString::fromStdString(command))
				.arg(QString::fromStdString(VuoStringUtilities::join(linesFromCommand, '\n')))));

		QVERIFY2(VuoStringUtilities::beginsWith(linesFromCommand[0], binaryPath),
			qPrintable(QString("The output of `%1` should begin with the path to the binary we're checking, but instead it output \"%2\"")
				.arg(QString::fromStdString(command))
				.arg(QString::fromStdString(VuoStringUtilities::join(linesFromCommand, '\n')))));

		for (auto line : linesFromCommand)
			if (line[0] == '\t')
			{
				vector<string> parts = VuoStringUtilities::split(VuoStringUtilities::substrAfter(line, "\t"), ' ');
				QVERIFY2(parts.size() > 1,
					qPrintable(QString("The output of `%1` should be a dependency list where each dependency's loader path is followed by version numbers, but instead it output \"%2\"")
						.arg(QString::fromStdString(command))
						.arg(line.c_str())));

				doForDependency(parts[0]);
			}
	}
};

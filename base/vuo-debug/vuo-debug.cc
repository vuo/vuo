/**
 * @file
 * vuo-debug implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <getopt.h>
#include <Vuo/Vuo.h>


class TelemetryLogger : public VuoRunnerDelegate
{
	void receivedTelemetryStats(unsigned long utime, unsigned long stime)
	{
		printf("stats: %lu %lu\n", utime, stime);
	}
	void receivedTelemetryNodeExecutionStarted(string nodeIdentifier)
	{
		printf("nodeExecutionStarted: %s\n", nodeIdentifier.c_str());
	}
	void receivedTelemetryNodeExecutionFinished(string nodeIdentifier)
	{
		printf("nodeExecutionFinished: %s\n", nodeIdentifier.c_str());
	}
	void receivedTelemetryInputPortUpdated(string portIdentifier, bool receivedEvent, bool receivedData, string dataSummary)
	{
		printf("inputPortUpdated: %s %d %d %s\n", portIdentifier.c_str(), receivedEvent, receivedData, dataSummary.c_str());
	}
	void receivedTelemetryOutputPortUpdated(string portIdentifier, bool sentData, string dataSummary)
	{
		printf("outputPortUpdated: %s %d %s\n", portIdentifier.c_str(), sentData, dataSummary.c_str());
	}
	void receivedTelemetryPublishedOutputPortUpdated(VuoRunner::Port *port, bool sentData, string dataSummary)
	{
		printf("publishedOutputPortUpdated: %s %d %s\n", port->getName().c_str(), sentData, dataSummary.c_str());
	}
	void receivedTelemetryEventDropped(string portIdentifier)
	{
		printf("eventDropped: %s\n", portIdentifier.c_str());
	}
	void receivedTelemetryError(string message)
	{
		printf("error: %s\n", message.c_str());
	}
	void lostContactWithComposition(void)
	{
		printf("lostContactWithComposition\n");
	}
};

void printHelp(char *argv0)
{
	printf("Tool for debugging compositions. Runs the given composition executable file and logs telemetry data to the console.\n\n"
		   "Nodes in the composition use the directory containing the composition executable file to resolve relative paths.\n\n"
		   "Usage: %s [options] file\n\n"
		   "Options:\n"
		   "  --help                       Display this information.\n",
		   argv0);
}

int main (int argc, char * const argv[])
{
	bool hasInputFile = false;
	string executablePath;

	try
	{
		bool doPrintHelp = false;

		static struct option options[] = {
			{"help", no_argument, NULL, 0},
			{NULL, no_argument, NULL, 0}
		};
		int optionIndex=-1;
		while((getopt_long(argc, argv, "", options, &optionIndex)) != -1)
		{
			switch(optionIndex)
			{
				case 0:  // --help
					doPrintHelp = true;
					break;
			}
		}

		hasInputFile = (optind < argc) && ! doPrintHelp;

		if (doPrintHelp)
			printHelp(argv[0]);
		else
		{
			if (! hasInputFile)
				throw std::runtime_error("no input file");
			executablePath = argv[optind];

			string file, executableDir, ext;
			VuoFileUtilities::splitPath(executablePath, executableDir, file, ext);

			VuoRunner * runner = VuoRunner::newSeparateProcessRunnerFromExecutable(executablePath, executableDir);

			TelemetryLogger *logger = new TelemetryLogger();
			runner->setDelegate(logger);

			runner->startPaused();
			runner->subscribeToAllTelemetry();
			runner->unpause();
			runner->waitUntilStopped();
		}

		return 0;
	}
	catch (std::exception &e)
	{
		fprintf(stderr, "%s: error: %s\n", hasInputFile ? executablePath.c_str() : argv[0], e.what());
		if (!hasInputFile)
		{
			fprintf(stderr, "\n");
			printHelp(argv[0]);
		}
		return 1;
	}
}

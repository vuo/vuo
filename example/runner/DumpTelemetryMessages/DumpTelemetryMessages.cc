/**
 * @file
 * DumpTelemetryMessages implementation.
 *
 * This example demonstrates receiving telemetry messages from a running Vuo composition.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <Vuo/Vuo.h>

class TelemetryLogger : public VuoRunnerDelegate
{
	void receivedTelemetryStats(unsigned long utime, unsigned long stime)
	{
		printf("heartbeat\n");
	}
	void receivedTelemetryNodeExecutionStarted(string compositionIdentifier, string nodeIdentifier)
	{
		printf("nodeExecutionStarted: %s\n", nodeIdentifier.c_str());
	}
	void receivedTelemetryNodeExecutionFinished(string compositionIdentifier, string nodeIdentifier)
	{
		printf("nodeExecutionFinished: %s\n", nodeIdentifier.c_str());
	}
	void receivedTelemetryInputPortUpdated(string compositionIdentifier, string portIdentifier, bool receivedEvent, bool receivedData, string dataSummary)
	{
		printf("inputPortUpdated: %s %d %d %s\n", portIdentifier.c_str(), receivedEvent, receivedData, dataSummary.c_str());
	}
	void receivedTelemetryOutputPortUpdated(string compositionIdentifier, string portIdentifier, bool sentEvent, bool sentData, string dataSummary)
	{
		printf("outputPortUpdated: %s %d %d %s\n", portIdentifier.c_str(), sentEvent, sentData, dataSummary.c_str());
	}
	void receivedTelemetryPublishedOutputPortUpdated(VuoRunner::Port *port, bool sentData, string dataSummary)
	{
		printf("publishedOutputPortUpdated: %s %d %s\n", port->getName().c_str(), sentData, dataSummary.c_str());
	}
	void receivedTelemetryEventDropped(string compositionIdentifier, string portIdentifier)
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

int main (int argc, char * const argv[])
{
	VuoCompilerIssues issues;
	VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile(SOURCE_DIR "/Count.vuo", &issues);
	if (! runner)
	{
		fprintf(stderr, "%s\n", issues.getLongDescription(false).c_str());
		return 1;
	}

	TelemetryLogger *logger = new TelemetryLogger();
	runner->setDelegate(logger);

	runner->startPaused();
	runner->subscribeToAllTelemetry("");
	runner->unpause();

	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC * 5), dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
	                   runner->stop();
	               });

	runner->waitUntilStopped();

	delete runner;

	return 0;
}

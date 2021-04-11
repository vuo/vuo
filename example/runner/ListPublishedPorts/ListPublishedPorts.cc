/**
 * @file
 * ListPublishedPorts implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <Vuo/Vuo.h>

int main(void)
{
	// Compile, link, and run the composition
	VuoCompilerIssues issues;
	VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile(SOURCE_DIR "/CountWithPublishedPorts.vuo", &issues);
	if (! runner)
	{
		fprintf(stderr, "%s\n", issues.getLongDescription(false).c_str());
		return 1;
	}
	runner->startPaused();

	try
	{
		// Print a list of the published input and output ports
		vector<VuoRunner::Port *> inputs = runner->getPublishedInputPorts();
		vector<VuoRunner::Port *> outputs = runner->getPublishedOutputPorts();

		printf("Published input ports:\n");
		for (vector<VuoRunner::Port *>::iterator i = inputs.begin(); i != inputs.end(); ++i)
			printf("\t%s (%s)\n", (*i)->getName().c_str(), (*i)->getType().c_str());

		printf("Published output ports:\n");
		for (vector<VuoRunner::Port *>::iterator i = outputs.begin(); i != outputs.end(); ++i)
			printf("\t%s (%s)\n", (*i)->getName().c_str(), (*i)->getType().c_str());
	}
	catch (exception &e)
	{
		fprintf(stderr, "Error: %s", e.what());
	}

	runner->stop();

	return 0;
}

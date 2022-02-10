/**
 * @file
 * CompileAndRunInNewProcess implementation.
 *
 * This example demonstrates compiling and linking a composition (.vuo file) and then running it in a separate process.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <Vuo/Vuo.h>

int main(void)
{
	// Compile, link, and run the composition in a new process
	VuoCompilerIssues issues;
	VuoRunner *runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile(SOURCE_DIR "/Count.vuo", &issues);
	if (! runner)
	{
		fprintf(stderr, "%s\n", issues.getLongDescription(false).c_str());
		return 1;
	}
	runner->start();

	// Stop it after 5 seconds
	sleep(5);
	runner->stop();

	return 0;
}

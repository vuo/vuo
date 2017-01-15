/**
 * @file
 * CompileAndRunInNewProcess implementation.
 *
 * This example demonstrates compiling and linking a composition (.vuo file) and then running it in a separate process.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <Vuo/Vuo.h>

int main(void)
{
	// Compile, link, and run the composition in a new process
	VuoRunner * runner = VuoCompiler::newSeparateProcessRunnerFromCompositionFile("Count.vuo");
	runner->start();

	// Stop it after 5 seconds
	sleep(5);
	runner->stop();

	return 0;
}

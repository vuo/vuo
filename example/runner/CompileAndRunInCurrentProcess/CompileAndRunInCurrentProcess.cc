/**
 * @file
 * CompileAndRunInCurrentProcess implementation.
 *
 * This example demonstrates compiling and linking a composition (.vuo file) and then running it in the current process.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <dispatch/dispatch.h>
#include <Vuo/Vuo.h>

int main(void)
{
	// Compile, link, and run the composition in the current process
	VuoRunner * runner = VuoCompiler::newCurrentProcessRunnerFromCompositionFile("Count.vuo");
	runner->start();

	// On a separate thread, wait 5 seconds, then stop the composition
	dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
	dispatch_async(queue, ^{
		sleep(5);
		runner->stop();
	});

	// Perform the work the composition needs on the main thread, and wait for the compostion to stop
	runner->runOnMainThread();

	delete runner;

	return 0;
}

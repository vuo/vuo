/**
 * @file
 * VuoRunner32 implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoRunner32.hh"
#include "VuoFileUtilities.hh"

/**
 * Creates a runner object that can run the composition in file @a compositionPath in a new process.
 */
VuoRunner32 * VuoRunner32::newSeparateProcessRunnerFromCompositionFile(string compositionPath)
{
	string compositionDir, compositionFile, compositionExt;
	VuoFileUtilities::splitPath(compositionPath, compositionDir, compositionFile, compositionExt);
	string compiledCompositionPath = VuoFileUtilities::makeTmpFile(compositionFile, "bc");

	string compiledCompositionDir, compiledCompositionFile, compiledCompositionExt;
	VuoFileUtilities::splitPath(compiledCompositionPath, compiledCompositionDir, compiledCompositionFile, compiledCompositionExt);
	string linkedCompositionPath = compiledCompositionDir + compiledCompositionFile;

	string vuoFrameworkDir = VuoFileUtilities::getVuoFrameworkPath();
	string vuoCompilePath = vuoFrameworkDir + "/MacOS/vuo-compile";
	string vuoLinkPath = vuoFrameworkDir + "/MacOS/vuo-link";

	// vuo-compile --output /tmp/composition.bc composition.vuo
	pid_t compilePid = fork();
	if (compilePid == 0)
	{
		execl(vuoCompilePath.c_str(), "vuo-compile", "--output", compiledCompositionPath.c_str(), compositionPath.c_str(), NULL);
		fprintf(stderr, "Couldn't run %s\n", vuoCompilePath.c_str());
		return NULL;
	}
	else if (compilePid > 0)
	{
		int status;
		waitpid(compilePid, &status, 0);
	}
	else
	{
		fprintf(stderr, "Couldn't fork %s\n", vuoCompilePath.c_str());
		return NULL;
	}

	// vuo-link --output /tmp/composition composition.bc
	pid_t linkPid = fork();
	if (linkPid == 0)
	{
		execl(vuoLinkPath.c_str(), "vuo-link", "--output", linkedCompositionPath.c_str(), compiledCompositionPath.c_str(), NULL);
		fprintf(stderr, "Couldn't run %s\n", vuoLinkPath.c_str());
		return NULL;
	}
	else if (linkPid > 0)
	{
		int status;
		waitpid(linkPid, &status, 0);
	}
	else
	{
		fprintf(stderr, "Couldn't fork %s\n", vuoLinkPath.c_str());
		return NULL;
	}

	remove(compiledCompositionPath.c_str());

	return newSeparateProcessRunnerFromExecutable(linkedCompositionPath, compositionDir, true);
}

/**
 * @see VuoRunner::newSeparateProcessRunnerFromExecutable
 */
VuoRunner32 * VuoRunner32::newSeparateProcessRunnerFromExecutable(string executablePath, string sourceDir,
																  bool deleteExecutableWhenFinished)
{
	VuoRunner32 * vr = new VuoRunner32();
	vr->executablePath = executablePath;
	vr->shouldDeleteBinariesWhenFinished = deleteExecutableWhenFinished;
	vr->sourceDir = sourceDir;
	return vr;
}

/**
 * @see VuoRunner::start
 */
void VuoRunner32::start(void)
{
	VuoRunner::start();
}

/**
 * @see VuoRunner::startPaused
 */
void VuoRunner32::startPaused(void)
{
	VuoRunner::startPaused();
}

/**
 * @see VuoRunner::pause
 */
void VuoRunner32::pause(void)
{
	VuoRunner::pause();
}

/**
 * @see VuoRunner::unpause
 */
void VuoRunner32::unpause(void)
{
	VuoRunner::unpause();
}

/**
 * @see VuoRunner::stop
 */
void VuoRunner32::stop(void)
{
	VuoRunner::stop();
}

/**
 * @see VuoRunner::waitUntilStopped
 */
void VuoRunner32::waitUntilStopped(void)
{
	VuoRunner::waitUntilStopped();
}

/**
 * @see VuoRunner::setPublishedInputPortValue
 */
void VuoRunner32::setPublishedInputPortValue(Port *port, json_object *value)
{
	VuoRunner::setPublishedInputPortValue(port, value);
}

/**
 * @see VuoRunner::firePublishedInputPortEvent
 */
void VuoRunner32::firePublishedInputPortEvent(Port *port)
{
	VuoRunner::firePublishedInputPortEvent(port);
}

/**
 * @see VuoRunner::firePublishedInputPortEvent
 */
void VuoRunner32::firePublishedInputPortEvent(void)
{
	VuoRunner::firePublishedInputPortEvent();
}

/**
 * @see VuoRunner::waitForAnyPublishedOutputPortEvent
 */
void VuoRunner32::waitForAnyPublishedOutputPortEvent(void)
{
	VuoRunner::waitForAnyPublishedOutputPortEvent();
}

/**
 * @see VuoRunner::getPublishedOutputPortValue
 */
json_object * VuoRunner32::getPublishedOutputPortValue(Port *port)
{
	return VuoRunner::getPublishedOutputPortValue(port);
}

/**
 * @see VuoRunner::getPublishedInputPorts
 */
vector<VuoRunner::Port *> VuoRunner32::getPublishedInputPorts(void)
{
	return VuoRunner::getPublishedInputPorts();
}

/**
 * @see VuoRunner::getPublishedOutputPorts
 */
vector<VuoRunner::Port *> VuoRunner32::getPublishedOutputPorts(void)
{
	return VuoRunner::getPublishedOutputPorts();
}

/**
 * @see VuoRunner::getPublishedInputPortWithName
 */
VuoRunner::Port * VuoRunner32::getPublishedInputPortWithName(string name)
{
	return VuoRunner::getPublishedInputPortWithName(name);
}

/**
 * @see VuoRunner::getPublishedOutputPortWithName(
 */
VuoRunner::Port * VuoRunner32::getPublishedOutputPortWithName(string name)
{
	return VuoRunner::getPublishedOutputPortWithName(name);
}

/**
 * @see VuoRunner::setInputPortValue
 */
void VuoRunner32::setInputPortValue(string portIdentifier, json_object *value)
{
	VuoRunner::setInputPortValue(portIdentifier, value);
}

/**
 * @see VuoRunner::fireTriggerPortEvent
 */
void VuoRunner32::fireTriggerPortEvent(string portIdentifier)
{
	VuoRunner::fireTriggerPortEvent(portIdentifier);
}

/**
 * @see VuoRunner::getInputPortValue
 */
json_object * VuoRunner32::getInputPortValue(string portIdentifier)
{
	return VuoRunner::getInputPortValue(portIdentifier);
}

/**
 * @see VuoRunner::getOutputPortValue
 */
json_object * VuoRunner32::getOutputPortValue(string portIdentifier)
{
	return VuoRunner::getOutputPortValue(portIdentifier);
}

/**
 * @see VuoRunner::getInputPortSummary
 */
string VuoRunner32::getInputPortSummary(string portIdentifier)
{
	return VuoRunner::getInputPortSummary(portIdentifier);
}

/**
 * @see VuoRunner::getOutputPortSummary
 */
string VuoRunner32::getOutputPortSummary(string portIdentifier)
{
	return VuoRunner::getOutputPortSummary(portIdentifier);
}

/**
 * @see VuoRunner::isStopped
 */
bool VuoRunner32::isStopped(void)
{
	return VuoRunner::isStopped();
}

/**
 * @see VuoRunner32::setDelegate
 */
void VuoRunner32::setDelegate(VuoRunnerDelegate *delegate)
{
	VuoRunner::setDelegate(delegate);
}

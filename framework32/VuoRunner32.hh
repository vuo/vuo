/**
 * @file
 * VuoRunner32 interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUORUNNER32_H
#define VUORUNNER32_H

#include "VuoRunner.hh"

/**
 * 32-bit version of VuoRunner.
 *
 * This class allows running and controlling a composition from a 32-bit process.
 * The composition runs in a separate 64-bit process.
 */
class VuoRunner32 : private VuoRunner
{
public:
	static VuoRunner32 * newSeparateProcessRunnerFromCompositionFile(string compositionPath);
	static VuoRunner32 * newSeparateProcessRunnerFromExecutable(string executablePath, string sourceDir, bool deleteExecutableWhenFinished = false);
	void start(void);
	void startPaused(void);
	void pause(void);
	void unpause(void);
	void stop(void);
	void waitUntilStopped(void);
	void setPublishedInputPortValue(Port *port, json_object *value);
	void firePublishedInputPortEvent(Port *port);
	void firePublishedInputPortEvent(void);
	void waitForAnyPublishedOutputPortEvent(void);
	json_object * getPublishedOutputPortValue(Port *port);
	vector<Port *> getPublishedInputPorts(void);
	vector<Port *> getPublishedOutputPorts(void);
	Port * getPublishedInputPortWithName(string name);
	Port * getPublishedOutputPortWithName(string name);
	void setInputPortValue(string portIdentifier, json_object *value);
	void fireTriggerPortEvent(string portIdentifier);
	json_object * getInputPortValue(string portIdentifier);
	json_object * getOutputPortValue(string portIdentifier);
	string getInputPortSummary(string portIdentifier);
	string getOutputPortSummary(string portIdentifier);
	bool isStopped(void);
	void setDelegate(VuoRunnerDelegate *delegate);
};

#endif

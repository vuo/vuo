/**
 * @file
 * VuoException implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoException.hh"

#include <iostream>
#include <sstream>

using namespace std;

/**
 * Creates an exception with the specified description,
 * collects the current stack trace,
 * and optionally logs both.
 */
VuoException::VuoException(const string &description, bool log)
{
	this->description = description;
	this->backtrace = VuoLog_getBacktrace();

	if (log)
	{
		if (backtrace.size() < 3)
		{
			VUserLog("%s", description.c_str());
			return;
		}

		ostringstream oss;
		oss << description << endl;
		oss << "Exception occurred at:" << endl;

		// Skip the VuoException constructor.
		backtrace.erase(backtrace.begin());

		int i = 1;
		for (auto line : backtrace)
		{
			char *lineNumber;
			asprintf(&lineNumber, "%3d ", i++);
			oss << lineNumber << line << endl;
			free(lineNumber);
		}

		string dispatchQueue = dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL);
		if (dispatchQueue != "com.apple.root.default-qos")
			oss << "Queue: " << dispatchQueue << endl;

		char threadName[256];
		bzero(threadName, 256);
		int ret = pthread_getname_np(pthread_self(), threadName, 256);
		if (ret == 0 && strlen(threadName))
			oss << "Thread: " << threadName << endl;

		// Use VLog so it appears in crash reports.
		VUserLog("%s", oss.str().c_str());
	}
}

/**
 * Destructor.
 */
VuoException::~VuoException(void) throw()
{
}

/**
 * Returns the description.
 */
const char *VuoException::what() const throw()
{
	return description.c_str();
}

/**
 * Returns the stack trace when creating this exception.
 */
const vector<string> &VuoException::getBacktrace()
{
	return backtrace;
}

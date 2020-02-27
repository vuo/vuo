/**
 * @file
 * VuoException interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * An exception that keeps track of the stack when it was created.
 */
class VuoException : public exception
{
public:
	VuoException(const string &description, bool log = true);
	~VuoException(void) throw();  ///< Needed to prevent a build error.
	const char *what() const throw();
	const vector<string> &getBacktrace();

private:
	string description;
	vector<string> backtrace;
};

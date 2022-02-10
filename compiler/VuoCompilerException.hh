/**
 * @file
 * VuoCompilerException interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoCompilerIssue;
class VuoCompilerIssues;

#include "VuoException.hh"

/**
 * An exception containing a list of issues that occurred while compiling a composition.
 *
 * At least one of the issues should be an error (otherwise there's no reason to throw an exception).
 * The issues may also include warnings.
 *
 * Currently, compiler functions handle `VuoCompilerIssue`s in two different ways. The older way is to create
 * a new VuoCompilerIssue if there's an error and throw a VuoCompilerException for it. The newer way is for the
 * function to have a VuoCompilerIssues parameter, for it to append errors and warnings to those VuoCompilerIssues,
 * and for it to throw a VuoCompilerException containing those VuoCompilerIssues if there's an error.
 * Unless you know that the function and its callees are all using the older way or all using the newer way,
 * the recommended pattern for callers to handle both possibilities is:
 *
 * \eg{
 * VuoCompilerIssues *issues = new VuoCompilerIssues();
 * try {
 *     fun(issues);
 * } catch (VuoCompilerException &e) {
 *     if (e.getIssues() != issues)
 *         issues->append(e.getIssues());
 * }
 * cout << issues->getLongDescription();  // or whatever you want to do with issues
 * }
 */
class VuoCompilerException : public VuoException
{
public:
	VuoCompilerException(const VuoCompilerIssue &issue);
	VuoCompilerException(VuoCompilerIssues *issues, bool ownsIssues);
	VuoCompilerException(const VuoCompilerException &other);
//	VuoCompilerException(void) throw() { }  ///< Needed for this class to be the value type in an STL map.
	~VuoCompilerException(void) throw();  ///< Needed to prevent a build error.
	VuoCompilerIssues * getIssues(void) const;
	const char * what() const throw();

private:
	VuoCompilerIssues *issues;
	bool ownsIssues;
	char *shortDescription;
};

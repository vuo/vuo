/**
 * @file
 * VuoCompilerException implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompilerException.hh"
#include "VuoCompilerIssue.hh"

/**
 * Constructor.
 */
VuoCompilerException::VuoCompilerException(const VuoCompilerIssue &issue)
	: VuoException(issue.getShortDescription(false), false)
{
	issues = new VuoCompilerIssues(issue);
	ownsIssues = true;
	shortDescription = issues ? strdup(issues->getShortDescription(false).c_str()) : nullptr;
}

/**
 * Constructor.
 *
 * If @a ownsIssues is true, the created VuoCompilerException is responsible for deallocating @a issues.
 * Otherwise, the caller is responsible.
 */
VuoCompilerException::VuoCompilerException(VuoCompilerIssues *issues, bool ownsIssues)
	: VuoException(issues ? issues->getShortDescription(false) : "", false)
{
	this->issues = issues;
	this->ownsIssues = ownsIssues;
	this->shortDescription = issues ? strdup(issues->getShortDescription(false).c_str()) : nullptr;
}

/**
 * Copy constructor.
 */
VuoCompilerException::VuoCompilerException(const VuoCompilerException &other)
	: VuoException(other)
{
	VUserLog("This shouldn't be called. You may need to replace `throw e` with `throw`.");
	this->issues = nullptr;
	this->ownsIssues = false;
	this->shortDescription = nullptr;
}

/**
 * Destructor.
 */
VuoCompilerException::~VuoCompilerException(void) throw()
{
	if (ownsIssues)
		delete issues;
	free(shortDescription);
}

/**
 * Returns the list of issues.
 */
VuoCompilerIssues * VuoCompilerException::getIssues(void) const
{
	return issues;
}

/**
 * Returns an HTML-formatted string containing short descriptions of all issues.
 */
const char * VuoCompilerException::what() const throw()
{
	return shortDescription;
}

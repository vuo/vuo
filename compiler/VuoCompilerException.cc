/**
 * @file
 * VuoCompilerException implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerException.hh"


/**
 * Constructor.
 */
VuoCompilerError::VuoCompilerError(const string &summary, const string &details,
								   const set<VuoNode *> &problemNodes, const set<VuoCable *> &problemCables)
{
	this->summary = summary;
	this->details = details;
	this->problemNodes = problemNodes;
	this->problemCables = problemCables;
}

/**
 * Returns a summary of the error.
 */
string VuoCompilerError::getSummary(void) const
{
	return summary;
}

/**
 * Returns a detailed description of the error.
 */
string VuoCompilerError::getDetails(void) const
{
	return details;
}

/**
 * Returns the nodes involved in the error.
 */
set<VuoNode *> VuoCompilerError::getProblemNodes(void) const
{
	return problemNodes;
}

/**
 * Returns the cables involved in the error.
 */
set<VuoCable *> VuoCompilerError::getProblemCables(void) const
{
	return problemCables;
}


/**
 * Constructor.
 */
VuoCompilerException::VuoCompilerException(const vector<VuoCompilerError> &errors)
{
	this->errors = errors;

	for (vector<VuoCompilerError>::iterator i = this->errors.begin(); i != this->errors.end(); ++i)
		description += (*i).getSummary() + " — " + (*i).getDetails() + "\n";
}

/**
 * Returns the list of errors.
 */
vector<VuoCompilerError> VuoCompilerException::getErrors(void) const
{
	return errors;
}

/**
 * Returns a string containing the list of errors.
 */
const char * VuoCompilerException::what() const throw()
{
	return description.c_str();
}

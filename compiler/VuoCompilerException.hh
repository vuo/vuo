/**
 * @file
 * VuoCompilerException interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILEREXCEPTION_HH
#define VUOCOMPILEREXCEPTION_HH

class VuoCable;
class VuoNode;

/**
 * A description of an error that occurred while compiling a composition.
 */
class VuoCompilerError
{
public:
	VuoCompilerError(const string &summary, const string &details,
					 const set<VuoNode *> &problemNodes, const set<VuoCable *> &problemCables);
	string getSummary(void) const;
	string getDetails(void) const;
	set<VuoNode *> getProblemNodes(void) const;
	set<VuoCable *> getProblemCables(void) const;

private:
	string summary;
	string details;
	set<VuoNode *> problemNodes;
	set<VuoCable *> problemCables;
};


/**
 * An exception containing a list of errors that occurred while compiling a composition.
 */
class VuoCompilerException : public exception
{
public:
	VuoCompilerException(const vector<VuoCompilerError> &errors);
	VuoCompilerException(void) throw() { }  ///< Needed for this class to be the value type in an STL map.
	~VuoCompilerException(void) throw() { }  ///< Needed to prevent a build error.
	vector<VuoCompilerError> getErrors(void) const;
	const char * what() const throw();

private:
	vector<VuoCompilerError> errors;
	string description;
};

#endif

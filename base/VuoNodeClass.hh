/**
 * @file
 * VuoNodeClass interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUONODECLASS_HH
#define VUONODECLASS_HH

#include "VuoBase.hh"
#include "VuoModule.hh"

class VuoCompilerNodeClass;
class VuoPortClass;
class VuoNode;

/**
 * This base class represents the metadata and implementation of one node class — i.e., for each node implementation file, there is one instance of @c VuoNodeClass.
 *
 * To instantiate a node implemented by this node class, create an instance of @c VuoNode using @c VuoNodeClass::newNode or @c VuoCompilerNodeClass::newNode.
 *
 * @see VuoCompilerNodeClass
 */
class VuoNodeClass : public VuoBase<VuoCompilerNodeClass,void>, public VuoModule
{
public:
	VuoNodeClass(string className, vector<string> inputPortClassNames, vector<string> outputPortClassNames);
	VuoNodeClass(string className, VuoPortClass * refreshPortClass, VuoPortClass * donePortClass, vector<VuoPortClass *> inputPortClasses, vector<VuoPortClass *> outputPortClasses);

	static const int unreservedInputPortStartIndex;
	static const int unreservedOutputPortStartIndex;
	static const string publishedInputNodeClassName;
	static const string publishedOutputNodeClassName;
	static const string publishedInputNodeIdentifier;
	static const string publishedOutputNodeIdentifier;
	static const string publishedInputNodeSimultaneousTriggerName;

	VuoNode * newNode(string title="", double x=0, double y=0);
	VuoNode * newNode(VuoNode *nodeToCopyMetadataFrom);

	string getClassName(void);
	bool isTypecastNodeClass(void);

	bool isInterface(void);
	void setInterface(bool isInterface);

	vector<string> getExampleCompositionFileNames(void);
	void setExampleCompositionFileNames(vector<string> exampleCompositionFileNames);

	VuoPortClass * getRefreshPortClass(void);
	void setRefreshPortClass(VuoPortClass * refreshPortClass);
	VuoPortClass * getDonePortClass(void);
	void setDonePortClass(VuoPortClass * donePortClass);

	vector<VuoPortClass *> getInputPortClasses(void);
	void setInputPortClasses(vector<VuoPortClass *> inputPortClasses);

	vector<VuoPortClass *> getOutputPortClasses(void);
	void setOutputPortClasses(vector<VuoPortClass *> outputPortClasses);

	void print(void);

private:
	bool interface;
	vector<string> exampleCompositionFileNames;
	VuoPortClass * refreshPortClass;
	VuoPortClass * donePortClass;
	vector<VuoPortClass *> inputPortClasses; ///< Includes refresh port.
	vector<VuoPortClass *> outputPortClasses; ///< Includes done port.
};

#endif // VUONODECLASS_HH

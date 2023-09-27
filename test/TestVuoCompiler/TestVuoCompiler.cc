/**
 * @file
 * TestVuoCompiler implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "TestVuoCompiler.hh"

/**
 * Initializes @c compiler and loads node classes.
 */
void TestVuoCompiler::initCompiler()
{
	compiler = new VuoCompiler(QDir::current().canonicalPath().toStdString());
	compiler->addModuleSearchPath(BINARY_DIR "/test/TestVuoCompiler");
}

/**
 * Cleans up @c compiler.
 */
void TestVuoCompiler::cleanupCompiler()
{
	delete compiler;
	compiler = NULL;
}

/**
 * Returns the path of the test composition (which should include the file extension).
 */
string TestVuoCompiler::getCompositionPath(string compositionFileName)
{
	QDir compositionDir = QDir::current();
	compositionDir.cd("composition");
	return compositionDir.filePath(QString(compositionFileName.c_str())).toStdString();
}

/**
 * Creates and returns a map allowing a node to be looked up based on its title.
 */
map<string, VuoNode *> TestVuoCompiler::makeNodeForTitle(vector<VuoNode *> nodes)
{
	map<string, VuoNode *> nodeForTitle;
	for (vector<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoNode *node = *i;
		nodeForTitle[node->getTitle()] = node;
	}
	return nodeForTitle;
}

/**
 * Creates and returns a map allowing a node to be looked up based on its title.
 */
map<string, VuoNode *> TestVuoCompiler::makeNodeForTitle(set<VuoNode *> nodes)
{
	map<string, VuoNode *> nodeForTitle;
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoNode *node = *i;
		nodeForTitle[node->getTitle()] = node;
	}
	return nodeForTitle;
}

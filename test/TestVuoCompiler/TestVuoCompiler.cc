/**
 * @file
 * TestVuoCompiler implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "TestVuoCompiler.hh"
#include "VuoCompilerCodeGenUtilities.hh"

/**
 * Uses the LLVM JIT to execute the specified function in the specified module.
 *
 * @see llvm/tools/lli/lli.cpp
 */
bool TestVuoCompiler::executeFunction(Module *mod, string functionName, vector<GenericValue> &args, GenericValue &ret)
{
	ForceJITLinking::ForceJITLinking();

	string errMsg;
	bool isErr = mod->MaterializeAllPermanently(&errMsg);
	if (isErr) {
		fprintf(stderr, "Couldn't materialize: %s\n", errMsg.c_str());
		return false;
	}

	EngineBuilder builder(mod);
	builder.setErrorStr(&errMsg);
	ExecutionEngine *ee = builder.create();
	if (! ee) {
		fprintf(stderr, "Couldn't create execution engine: %s\n", errMsg.c_str());
		return false;
	}

	Function *function = mod->getFunction(functionName);
	if (! function) {
		fprintf(stderr, "Couldn't find function %s\n", functionName.c_str());
		return false;
	}

	//		int ret = ee->runFunctionAsMain(EntryFn, InputArgv, envp);
	ret = ee->runFunction(function, args);
	return true;
}

/**
 * Initializes @c compiler and loads node classes.
 */
void TestVuoCompiler::initCompiler()
{
	compiler = new VuoCompiler();
	compiler->addModuleSearchPath("../TestVuoCompiler/node-TestVuoCompiler");
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

/**
 * @file
 * TestCompositionExecution implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "TestCompositionExecution.hh"

#include <fcntl.h>
#include <libgen.h>

/**
 * Sets up a compiler, adding both regular and for-testing-only node classes to the search path.
 */
VuoCompiler * TestCompositionExecution::initCompiler(void)
{
	VuoCompiler *c = new VuoCompiler();
	c->addModuleSearchPath("node-" + QDir::current().dirName().toStdString());
	return c;
}

/**
 * Returns the directory containing the test compositions.
 */
QDir TestCompositionExecution::getCompositionDir(void)
{
	QDir compositionDir = QDir::current();
	compositionDir.cd("composition");
	return compositionDir;
}

/**
 * Returns the path of the test composition (which should include the file extension).
 */
string TestCompositionExecution::getCompositionPath(string compositionFileName)
{
	return getCompositionDir().filePath(QString(compositionFileName.c_str())).toStdString();
}

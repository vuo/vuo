/**
 * @file
 * TestVuoCompiler interface.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#include <QtCore/QString>
#include <QtTest/QtTest>

#include <libgen.h>
#include <fcntl.h>
#include "VuoCompiler.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerNodeClass.hh"

/**
 * Tests for basic compiler functionality.
 */
class TestVuoCompiler : public QObject
{
	Q_OBJECT

protected:
	VuoCompiler *compiler; ///< A compiler instance for testing.
	bool executeFunction(Module *mod, string functionName, vector<GenericValue> &args, GenericValue &ret);
	void initCompiler();
	void cleanupCompiler();
	string getCompositionPath(string compositionFileName);
	map<string, VuoNode *> makeNodeForTitle(vector<VuoNode *> nodes);
	map<string, VuoNode *> makeNodeForTitle(set<VuoNode *> nodes);
};

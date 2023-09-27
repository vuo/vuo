/**
 * @file
 * TestVuoCompiler interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include <fcntl.h>
#include <libgen.h>

#include <QtCore/QString>
#include <QtTest/QtTest>

#include <Vuo/Vuo.h>

/**
 * Tests for basic compiler functionality.
 */
class TestVuoCompiler : public QObject
{
	Q_OBJECT

protected:
	VuoCompiler *compiler; ///< A compiler instance for testing.
	void initCompiler();
	void cleanupCompiler();
	string getCompositionPath(string compositionFileName);
	map<string, VuoNode *> makeNodeForTitle(vector<VuoNode *> nodes);
	map<string, VuoNode *> makeNodeForTitle(set<VuoNode *> nodes);
};

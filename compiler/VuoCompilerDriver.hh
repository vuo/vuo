/**
 * @file
 * VuoCompilerDriver interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoCompiler;
class VuoCompilerComposition;
class VuoCompilerGraphvizParser;
class VuoProtocol;

/**
 * This class represents a driver for a protocol.
 */
class VuoCompilerDriver
{
public:
	static VuoCompilerDriver *driverForProtocol(VuoCompiler *compiler, string protocolId);

	bool isValidDriverForProtocol(VuoProtocol *protocol);
	void applyToComposition(VuoCompilerComposition *composition, VuoCompiler *compiler, bool canPublishedInputsBeEdited = true);

private:
	VuoCompilerDriver(VuoCompiler *compiler, const string &driverAsCompositionString);

	VuoCompiler *compiler;
	VuoCompilerGraphvizParser *parser;
	string compositionString;
};

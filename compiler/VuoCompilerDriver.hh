/**
 * @file
 * VuoCompilerDriver interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERDRIVER_HH
#define VUOCOMPILERDRIVER_HH

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
	VuoCompilerDriver(VuoCompiler *compiler, const string &driverAsCompositionString);

	bool isValidDriverForProtocol(VuoProtocol *protocol);
	void applyToComposition(VuoCompilerComposition *composition);

private:
	VuoCompilerGraphvizParser *parser;
};

#endif // VUOCOMPILERDRIVER_HH

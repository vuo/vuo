/**
 * @file
 * VuoPreprocessAction implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoPreprocessAction.hh"
#include "VuoPreprocessorCallbacks.hh"

using namespace clang;

/**
 * Constructs a Clang front-end action that is ready to be executed.
 *
 * @param output The stream to which the transformed source code will be written.
 * @param preprocessorCallbacks The set of callbacks for Clang to call while preprocessing the source code.
 */
VuoPreprocessAction::VuoPreprocessAction(raw_ostream &output, unique_ptr<VuoPreprocessorCallbacks> preprocessorCallbacks) :
	PrintPreprocessedAction(),
	output(output),
	preprocessorCallbacks(std::move(preprocessorCallbacks))
{
}

/**
 * Transfers `preprocessorCallbacks` to Clang's preprocessor.
 */
bool VuoPreprocessAction::BeginSourceFileAction(CompilerInstance &CI)
{
	CI.getPreprocessor().addPPCallbacks(std::move(preprocessorCallbacks));
	return true;
}

/**
 * Preprocesses the source code and writes the result to the output stream.
 */
void VuoPreprocessAction::ExecuteAction()
{
	CompilerInstance &compilerInstance = getCompilerInstance();
	compilerInstance.getPreprocessorOutputOpts().ShowCPP = 1;
	compilerInstance.getPreprocessorOutputOpts().ShowComments = 1;
	compilerInstance.getPreprocessorOutputOpts().ShowIncludeDirectives = 1;
	compilerInstance.getPreprocessorOutputOpts().ShowMacros = 1;
	compilerInstance.getPreprocessorOutputOpts().UseLineDirectives = 1;
	DoPrintPreprocessedInput(compilerInstance.getPreprocessor(), &output, compilerInstance.getPreprocessorOutputOpts());
}

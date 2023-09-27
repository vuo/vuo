/**
 * @file
 * VuoEmitLLVMOnlyAction implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoEmitLLVMOnlyAction.hh"
#include "VuoPreprocessorCallbacks.hh"

using namespace clang;

/**
 * Constructs a Clang front-end action that is ready to be executed.
 */
VuoEmitLLVMOnlyAction::VuoEmitLLVMOnlyAction(llvm::LLVMContext *context, unique_ptr<VuoPreprocessorCallbacks> preprocessorCallbacks) :
	EmitLLVMOnlyAction(context),
	preprocessorCallbacks(std::move(preprocessorCallbacks))
{
}

/**
 * Transfers `preprocessorCallbacks` to Clang's preprocessor.
 */
bool VuoEmitLLVMOnlyAction::BeginSourceFileAction(CompilerInstance &CI)
{
	CI.getPreprocessor().addPPCallbacks(std::move(preprocessorCallbacks));
	return true;
}

/**
 * @file
 * VuoEmitLLVMOnlyAction interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "clang/CodeGen/CodeGenAction.h"
class VuoPreprocessorCallbacks;

/**
 * A wrapper for Clang's EmitLLVMOnlyAction that adds VuoPreprocessorCallbacks.
 */
class VuoEmitLLVMOnlyAction : public clang::EmitLLVMOnlyAction
{
public:
	VuoEmitLLVMOnlyAction(llvm::LLVMContext *context, unique_ptr<VuoPreprocessorCallbacks> preprocessorCallbacks);
	bool BeginSourceFileAction(clang::CompilerInstance &CI);

private:
	unique_ptr<VuoPreprocessorCallbacks> preprocessorCallbacks;
};

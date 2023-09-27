/**
 * @file
 * VuoPreprocessAction interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "clang/Frontend/FrontendActions.h"
class VuoPreprocessorCallbacks;

/**
 * A Clang front-end action that runs source code through Clang's preprocessor.
 *
 * Unlike PrintPreprocessedAction, this class writes its output to a stream provided by the caller.
 */
class VuoPreprocessAction : public clang::PrintPreprocessedAction
{
public:
	VuoPreprocessAction(raw_ostream &output, unique_ptr<VuoPreprocessorCallbacks> preprocessorCallbacks);
	bool BeginSourceFileAction(clang::CompilerInstance &CI);
	void ExecuteAction();

private:
	raw_ostream &output;
	unique_ptr<VuoPreprocessorCallbacks> preprocessorCallbacks;
};

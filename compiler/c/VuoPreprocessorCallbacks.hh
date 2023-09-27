/**
 * @file
 * VuoPreprocessorCallbacks interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "clang/Lex/PPCallbacks.h"
class VuoCModuleCompiler;

/**
 * While a source file is being preprocessed, handles include directives for actual and placeholder header files.
 */
class VuoPreprocessorCallbacks : public clang::PPCallbacks
{
public:
	VuoPreprocessorCallbacks(VuoCModuleCompiler *moduleCompiler, clang::vfs::InMemoryFileSystem *inMemoryFileSystem, bool shouldDefineGenericTypes);
	bool FileNotFound(StringRef fileName, SmallVectorImpl<char> &recoveryPath);
	void InclusionDirective(clang::SourceLocation hashLoc, const clang::Token &includeTok, StringRef fileName, bool isAngled,
							clang::CharSourceRange fileNameRange, const clang::FileEntry *file, StringRef searchPath, StringRef relativePath,
							const clang::Module *imported);

private:
	VuoCModuleCompiler *moduleCompiler;
	clang::vfs::InMemoryFileSystem *inMemoryFileSystem;
	bool shouldDefineGenericTypes;
};

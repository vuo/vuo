/**
 * @file
 * VuoPreprocessorCallbacks implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoPreprocessorCallbacks.hh"
#include "VuoCModuleCompiler.hh"

using namespace clang;

/**
 * Constructs a callbacks object that is ready to be added to a Clang preprocessor.
 */
VuoPreprocessorCallbacks::VuoPreprocessorCallbacks(VuoCModuleCompiler *moduleCompiler, vfs::InMemoryFileSystem *inMemoryFileSystem, bool shouldDefineGenericTypes) :
	PPCallbacks(),
	moduleCompiler(moduleCompiler),
	inMemoryFileSystem(inMemoryFileSystem),
	shouldDefineGenericTypes(shouldDefineGenericTypes)
{
}

/**
 * If @a fileName is a placeholder for a specialized generic header, generates an in-memory header file
 * and tells the preprocessor where to find it.
 */
bool VuoPreprocessorCallbacks::FileNotFound(StringRef fileName, SmallVectorImpl<char> &recoveryPath)
{
	string recoveryPathStr = moduleCompiler->generateVirtualFileForSpecializedHeader(inMemoryFileSystem, fileName.str(), shouldDefineGenericTypes);
	if (! recoveryPathStr.empty())
	{
		recoveryPath.assign(recoveryPathStr.begin(), recoveryPathStr.end());
		return true;
	}

	return false;
}

/**
 * Records the name of each file included with `#include`, `#include_next`, or `#import`.
 */
void VuoPreprocessorCallbacks::InclusionDirective(SourceLocation hashLoc, const Token &includeTok, StringRef fileName, bool isAngled,
												  CharSourceRange fileNameRange, const FileEntry *file, StringRef searchPath, StringRef relativePath,
												  const clang::Module *imported)
{
	moduleCompiler->includedHeaders.insert(fileName.str());
}

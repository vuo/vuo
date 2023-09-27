/**
 * @file
 * VuoCompilerDiagnosticConsumer implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompilerDiagnosticConsumer.hh"

#include <sstream>

#include "VuoClangIssues.hh"

#ifndef DOXYGEN
// Workaround for "Undefined symbols… typeinfo for clang::DiagnosticConsumer".
clang::DiagnosticConsumer::~DiagnosticConsumer() {}
#endif

/**
 * Creates a diagnostic consumer that logs compilation errors in `issues`.
 *
 * @param issues The list to which compilation errors/warnings should be added.
 * @param virtualToRealFilePaths Key = virtual file path+name (see VuoCModuleCompiler::getVirtualSourcePath); value = real (file-on-disk) file path+name.
 */
VuoCompilerDiagnosticConsumer::VuoCompilerDiagnosticConsumer(shared_ptr<VuoClangIssues> issues, map<string, string> virtualToRealFilePaths)
	: issues(issues), virtualToRealFilePaths(virtualToRealFilePaths)
{
}

/**
 * Invoked by Clang to log compilation errors.
 *
 * Based on https://github.com/llvm-mirror/clang-tools-extra/blob/master/clangd/Compiler.cpp
 */
void VuoCompilerDiagnosticConsumer::HandleDiagnostic(clang::DiagnosticsEngine::Level level, const clang::Diagnostic &diagnostic)
{
	ostringstream formattedLocation;
	if (diagnostic.hasSourceManager() && diagnostic.getLocation().isValid())
	{
		auto &sourceManager = diagnostic.getSourceManager();
		auto loc = sourceManager.getFileLoc(diagnostic.getLocation());

		auto filename = sourceManager.getFilename(loc);

		// Ignore warnings in other people's code.
		if ((level == clang::DiagnosticsEngine::Note || level == clang::DiagnosticsEngine::Warning)
		 && (filename.find("/Vuo.framework/Frameworks/llvm.framework/") != llvm::StringRef::npos
		  || filename.find("/Vuo.framework/Headers/macos/") != llvm::StringRef::npos
		  || filename.find("/.conan/data/") != llvm::StringRef::npos
		  || filename.find("/Applications/Xcode.app/") != llvm::StringRef::npos))
			return;

		auto realFilePath = virtualToRealFilePaths.find(filename.str());
		if (realFilePath != virtualToRealFilePaths.end())
			formattedLocation << realFilePath->second;
		else
			formattedLocation << filename.str();

		formattedLocation << ':' << sourceManager.getPresumedLineNumber(loc);
		formattedLocation << ':' << sourceManager.getPresumedColumnNumber(loc);
	}

	llvm::SmallString<64> message;
	diagnostic.FormatDiagnostic(message);

	issues->addIssue(formattedLocation.str(), level, message.str());
}

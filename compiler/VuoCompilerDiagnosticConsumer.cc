/**
 * @file
 * VuoCompilerDiagnosticConsumer implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompilerDiagnosticConsumer.hh"

#include "VuoCompilerIssue.hh"

#ifndef DOXYGEN
// Workaround for "Undefined symbols… typeinfo for clang::DiagnosticConsumer".
clang::DiagnosticConsumer::~DiagnosticConsumer() {}
#endif

/**
 * Creates a diagnostic consumer that logs compilation errors in `issues`.
 */
VuoCompilerDiagnosticConsumer::VuoCompilerDiagnosticConsumer(VuoCompilerIssues *issues)
    : issues(issues)
{
}

/**
 * Invoked by Clang to log compilation errors.
 *
 * Based on https://github.com/llvm-mirror/clang-tools-extra/blob/master/clangd/Compiler.cpp
 */
void VuoCompilerDiagnosticConsumer::HandleDiagnostic(clang::DiagnosticsEngine::Level level, const clang::Diagnostic &diagnostic)
{
	VuoCompilerIssue::IssueType issueType = VuoCompilerIssue::Warning;
	const char *levelName;
	if (level == clang::DiagnosticsEngine::Note)
		levelName = "note";
	else if (level == clang::DiagnosticsEngine::Warning)
		levelName = "warning";
	else if (level == clang::DiagnosticsEngine::Error
		  || level == clang::DiagnosticsEngine::Fatal)
	{
		issueType = VuoCompilerIssue::Error;
		levelName = "error";
	}
	else
		return;

	llvm::SmallString<64> location;
	if (diagnostic.hasSourceManager() && diagnostic.getLocation().isValid())
	{
		auto &sourceManager = diagnostic.getSourceManager();
		auto loc = sourceManager.getFileLoc(diagnostic.getLocation());

		auto filename = sourceManager.getFilename(loc);

		// Not our code.
		if (filename.find("/Vuo.framework/Frameworks/llvm.framework/") != llvm::StringRef::npos
		 || filename.find("/Vuo.framework/Headers/macos/") != llvm::StringRef::npos
		 || filename.find("/.conan/data/") != llvm::StringRef::npos
		 || filename.find("/Applications/Xcode.app/") != llvm::StringRef::npos)
			if (issueType == VuoCompilerIssue::Warning)
				return;

		llvm::raw_svector_ostream oss(location);
		loc.print(oss, sourceManager);
	}

	llvm::SmallString<64> message;
	diagnostic.FormatDiagnostic(message);

	VUserLog("%s: %s: %s", levelName, location.c_str(), message.c_str());
	issues->append(VuoCompilerIssue(issueType, "compiling module", location.str(), "", message.str()));
}

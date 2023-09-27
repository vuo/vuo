/**
 * @file
 * VuoCompilerDiagnosticConsumer interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoClangIssues;

/**
 * Logs compilation errors to @ref VuoCompilerIssues.
 */
class VuoCompilerDiagnosticConsumer : public clang::DiagnosticConsumer
{
	shared_ptr<VuoClangIssues> issues;
	map<string, string> virtualToRealFilePaths;

public:
	explicit VuoCompilerDiagnosticConsumer(shared_ptr<VuoClangIssues> issues, map<string, string> virtualToRealFilePaths = {});
	void HandleDiagnostic(clang::DiagnosticsEngine::Level level, const clang::Diagnostic &diagnostic) override;
};

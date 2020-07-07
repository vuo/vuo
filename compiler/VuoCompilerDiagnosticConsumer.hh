/**
 * @file
 * VuoCompilerDiagnosticConsumer interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoCompilerIssues;

/**
 * Logs compilation errors to @ref VuoCompilerIssues.
 */
class VuoCompilerDiagnosticConsumer : public clang::DiagnosticConsumer
{
    VuoCompilerIssues *issues;

public:
	explicit VuoCompilerDiagnosticConsumer(VuoCompilerIssues *issues);
	void HandleDiagnostic(clang::DiagnosticsEngine::Level level, const clang::Diagnostic &diagnostic) override;
};

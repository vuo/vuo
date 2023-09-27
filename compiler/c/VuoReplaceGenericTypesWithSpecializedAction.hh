/**
 * @file
 * VuoReplaceGenericTypesWithSpecializedAction interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Rewrite/Core/Rewriter.h"
class VuoPreprocessorCallbacks;

/**
 * A Clang front-end action that performs a source-to-source transformation, replacing generic type names within
 * function names and Vuo type annotations with the corresponding specialized type names.
 */
class VuoReplaceGenericTypesWithSpecializedAction : public clang::ASTFrontendAction
{
public:
	VuoReplaceGenericTypesWithSpecializedAction(raw_ostream &output, const map<string, string> &replacements, bool shouldReplaceGenericTypesInVuoTypeAnnotations, json_object *specializedModuleDetails, unique_ptr<VuoPreprocessorCallbacks> preprocessorCallbacks);
	unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &compilerInstance, llvm::StringRef inFile);
	bool BeginSourceFileAction(clang::CompilerInstance &CI);
	void EndSourceFileAction();

private:
	raw_ostream &output;
	map<string, string> replacements;
	bool shouldReplaceGenericTypesInVuoTypeAnnotations;
	json_object *specializedModuleDetails;
	unique_ptr<VuoPreprocessorCallbacks> preprocessorCallbacks;
	clang::Rewriter rewriter;

	/**
	 * The class that actually does the work of replacing the types.
	 */
	class Visitor : public clang::RecursiveASTVisitor<Visitor>
	{
	public:
		Visitor(const map<string, string> &replacements, bool shouldReplaceGenericTypesInVuoTypeAnnotations, json_object *specializedModuleDetails, clang::Rewriter &rewriter);
		bool VisitFunctionDecl(clang::FunctionDecl *decl);
		bool VisitDeclRefExpr(clang::DeclRefExpr *expr);
		bool VisitAnnotateAttr(clang::AnnotateAttr *attr);
		bool VisitVarDecl(clang::VarDecl *decl);

	private:
		bool isInModuleSourceFile(clang::SourceRange sourceRange);
		void replaceGenericTypeNamesInTypeSpecifier(clang::QualType qualType, clang::SourceRange sourceRange);
		bool replaceGenericTypeNamesInIdentifier(string &identifier);
		vector<string> orderedGenericTypeNames;
		const map<string, string> replacements;
		bool shouldReplaceGenericTypesInVuoTypeAnnotations;
		json_object *specializedModuleDetails;
		clang::Rewriter &rewriter;
	};

	/**
	 * An intermediate class that just sets up the Visitor.
	 */
	class Consumer : public clang::ASTConsumer
	{
	public:
		Consumer(const map<string, string> &replacements, bool shouldReplaceGenericTypesInVuoTypeAnnotations, json_object *specializedModuleDetails, clang::Rewriter &rewriter);
		void HandleTranslationUnit(clang::ASTContext &context);

	private:
		Visitor visitor;
	};
};

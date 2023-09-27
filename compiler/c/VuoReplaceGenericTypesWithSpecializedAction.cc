/**
 * @file
 * VuoReplaceGenericTypesWithSpecializedAction implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoReplaceGenericTypesWithSpecializedAction.hh"
#include "VuoPreprocessorCallbacks.hh"
#include "VuoGenericType.hh"
#include "VuoStringUtilities.hh"

using namespace clang;

/**
 * Constructs a Clang front-end action that is ready to be executed.
 *
 * @param output The stream to which the transformed source code will be written.
 * @param replacements For each generic type name, the specialized type name that should be substituted in its place.
 * @param shouldReplaceGenericTypesInVuoTypeAnnotations If false, Vuo type annotations will not be modified.
 * @param specializedModuleDetails Module metadata that should be added to the metadata already in the source code.
 * @param preprocessorCallbacks Callbacks for handling include directives.
 */
VuoReplaceGenericTypesWithSpecializedAction::VuoReplaceGenericTypesWithSpecializedAction(raw_ostream &output,
																						 const map<string, string> &replacements,
																						 bool shouldReplaceGenericTypesInVuoTypeAnnotations,
																						 json_object *specializedModuleDetails,
																						 unique_ptr<VuoPreprocessorCallbacks> preprocessorCallbacks) :
	ASTFrontendAction(),
	output(output),
	replacements(replacements),
	shouldReplaceGenericTypesInVuoTypeAnnotations(shouldReplaceGenericTypesInVuoTypeAnnotations),
	specializedModuleDetails(specializedModuleDetails),
	preprocessorCallbacks(std::move(preprocessorCallbacks))
{
}

/**
 * Boilerplate needed by Clang.
 */
unique_ptr<ASTConsumer> VuoReplaceGenericTypesWithSpecializedAction::CreateASTConsumer(CompilerInstance &compilerInstance, llvm::StringRef inFile)
{
	rewriter.setSourceMgr(compilerInstance.getSourceManager(), compilerInstance.getLangOpts());
	return unique_ptr<Consumer>(new Consumer(replacements, shouldReplaceGenericTypesInVuoTypeAnnotations, specializedModuleDetails, rewriter));
}

/**
 * Transfers `preprocessorCallbacks` to Clang's preprocessor.
 */
bool VuoReplaceGenericTypesWithSpecializedAction::BeginSourceFileAction(CompilerInstance &CI)
{
	CI.getPreprocessor().addPPCallbacks(std::move(preprocessorCallbacks));
	return true;
}

/**
 * Writes the transformed source code to `output`.
 */
void VuoReplaceGenericTypesWithSpecializedAction::EndSourceFileAction()
{
	rewriter.getEditBuffer(rewriter.getSourceMgr().getMainFileID()).write(output);
}

/**
 * Constructor.
 */
VuoReplaceGenericTypesWithSpecializedAction::Consumer::Consumer(const map<string, string> &replacements,
																bool shouldReplaceGenericTypesInVuoTypeAnnotations,
																json_object *specializedModuleDetails,
																clang::Rewriter &rewriter) :
	visitor(replacements, shouldReplaceGenericTypesInVuoTypeAnnotations, specializedModuleDetails, rewriter)
{
}

/**
 * Boilerplate needed by Clang.
 */
void VuoReplaceGenericTypesWithSpecializedAction::Consumer::HandleTranslationUnit(clang::ASTContext &context)
{
	visitor.TraverseDecl(context.getTranslationUnitDecl());
}

/**
 * Constructor.
 */
VuoReplaceGenericTypesWithSpecializedAction::Visitor::Visitor(const map<string, string> &replacements,
															  bool shouldReplaceGenericTypesInVuoTypeAnnotations,
															  json_object *specializedModuleDetails,
															  clang::Rewriter &rewriter) :
	replacements(replacements),
	shouldReplaceGenericTypesInVuoTypeAnnotations(shouldReplaceGenericTypesInVuoTypeAnnotations),
	specializedModuleDetails(specializedModuleDetails),
	rewriter(rewriter)
{
}

/**
 * Replaces any generic type names found within the function name.
 */
bool VuoReplaceGenericTypesWithSpecializedAction::Visitor::VisitFunctionDecl(FunctionDecl *decl)
{
	DeclarationNameInfo nameInfo = decl->getNameInfo();
	string name = nameInfo.getAsString();

	if (replaceGenericTypeNamesInIdentifier(name))
		rewriter.ReplaceText(nameInfo.getSourceRange(), name);

	return true;
}

/**
 * Replaces any generic type names found within @a expr, a reference to an entity that has been
 * declared previously, such as a variable, function, or enum.
 */
bool VuoReplaceGenericTypesWithSpecializedAction::Visitor::VisitDeclRefExpr(DeclRefExpr *expr)
{
	DeclarationNameInfo nameInfo = expr->getNameInfo();
	string name = nameInfo.getAsString();
	string originalName = name;

	if (replaceGenericTypeNamesInIdentifier(name))
	{
		SourceLocation begin = expr->getLocStart();
		SourceLocation end = begin.getLocWithOffset(originalName.length()-1);
		SourceRange sourceRange(begin, end);
		rewriter.ReplaceText(sourceRange, name);
	}

	return true;
}

/**
 * Replaces any generic type names found within `__attribute__((annotate("vuoType:" …))`
 * (the annotations on function parameters in node classes).
 */
bool VuoReplaceGenericTypesWithSpecializedAction::Visitor::VisitAnnotateAttr(AnnotateAttr *attr)
{
	if (! shouldReplaceGenericTypesInVuoTypeAnnotations)
		return true;

	string annotation = attr->getAnnotation().str();
	if (VuoStringUtilities::beginsWith(annotation, "vuoType:"))
	{
		string attribute = rewriter.getRewrittenText(attr->getRange());
		if (replaceGenericTypeNamesInIdentifier(attribute))
			rewriter.ReplaceText(attr->getRange(), attribute);
	}

	return true;
}

/**
 * Adds `specializedModuleDetails` to the module's metadata.
 */
bool VuoReplaceGenericTypesWithSpecializedAction::Visitor::VisitVarDecl(VarDecl *decl)
{
	if (! specializedModuleDetails)
		return true;

	if (decl->getNameAsString() == "moduleDetails" && decl->hasInit())
	{
		Expr *expr = decl->getInit();
		while (isa<CastExpr>(expr))
			expr = cast<CastExpr>(expr)->getSubExpr();

		clang::StringLiteral *metadataExpr = dyn_cast<clang::StringLiteral>(expr);
		if (metadataExpr)
		{
			json_object *metadataJson = json_tokener_parse(metadataExpr->getString().str().c_str());
			json_object_object_add(metadataJson, "specializedModule", specializedModuleDetails);
			string modifiedMetadata = string("VUO_STRINGIFY(") + json_object_to_json_string_ext(metadataJson, JSON_C_TO_STRING_PLAIN) + ")";

			SourceLocation begin = metadataExpr->getLocStart();
			SourceLocation end = metadataExpr->getLocEnd();
			SourceRange sourceRange(begin, end);
			rewriter.ReplaceText(sourceRange, modifiedMetadata);
		}
	}

	return true;
}

/**
 * Finds each generic type name from `replacements` in @a identifier and replaces it with the corresponding
 * specialized type name from `replacements`.
 */
bool VuoReplaceGenericTypesWithSpecializedAction::Visitor::replaceGenericTypeNamesInIdentifier(string &identifier)
{
	return VuoGenericType::replaceGenericTypeNamesInString(identifier, replacements, orderedGenericTypeNames);
}

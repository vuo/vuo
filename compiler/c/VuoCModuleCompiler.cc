/**
 * @file
 * VuoCModuleCompiler implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCModuleCompiler.hh"
#include "VuoEmitLLVMOnlyAction.hh"
#include "VuoReplaceGenericTypesWithSpecializedAction.hh"
#include "VuoPreprocessAction.hh"
#include "VuoPreprocessorCallbacks.hh"
#include "VuoClangIssues.hh"
#include "VuoCompiler.hh"
#include "VuoCompilerCompoundType.hh"
#include "VuoCompilerDiagnosticConsumer.hh"
#include "VuoCompilerException.hh"
#include "VuoCompilerGenericType.hh"
#include "VuoCompilerSpecializedNodeClass.hh"
#include "VuoException.hh"
#include "VuoGenericType.hh"
#include "VuoMakeDependencies.hh"
#include "VuoNodeClass.hh"
#include "VuoNodeSet.hh"
#include "VuoStringUtilities.hh"
#include "VuoType.hh"
#include "clang/Frontend/TextDiagnosticBuffer.h"
#include "clang/Lex/PreprocessorOptions.h"
#include <sstream>

using namespace clang;

/**
 * Registers this compiler.
 */
void __attribute__((constructor)) VuoCModuleCompiler::init()
{
	VuoModuleCompiler::registerModuleCompiler("c", &VuoCModuleCompiler::newModuleCompiler);
}

/**
 * Constructs a module compiler, without yet checking if the provided source is valid or attempting to compile it.
 */
VuoCModuleCompiler::VuoCModuleCompiler(const string &moduleKey, const string &sourcePath, const VuoModuleCompilerSettings &settings) :
	VuoModuleCompiler(moduleKey, sourcePath, settings),
	specializedModuleDetails(nullptr),
	compileIssues(nullptr)
{
}

/**
 * Returns a new VuoCModuleCompiler instance if @a sourcePath has a C/C++/Objective-C file extension, otherwise null.
 */
VuoModuleCompiler * VuoCModuleCompiler::newModuleCompiler(const string &moduleKey, const string &sourcePath,
														  const VuoModuleCompilerSettings &settings)
{
	string dir, file, ext;
	VuoFileUtilities::splitPath(sourcePath, dir, file, ext);
	if (VuoFileUtilities::isCFamilySourceExtension(ext))
		return new VuoCModuleCompiler(moduleKey, sourcePath, settings);

	return nullptr;
}

/**
 * Destructor.
 */
VuoCModuleCompiler::~VuoCModuleCompiler(void)
{
	delete compileIssues;
}

/**
 * Overrides the source code, using @a sourceCode instead of the file contents.
 */
void VuoCModuleCompiler::overrideSourceCode(const string &sourceCode, const string &sourcePath)
{
	this->sourceCode = sourceCode;
}

/**
 * Compiles the source code to LLVM bitcode that can be loaded as a Vuo node class, type, or library.
 */
VuoModuleCompilerResults VuoCModuleCompiler::compile(dispatch_queue_t llvmQueue, VuoCompilerIssues *issues)
{
	VuoModuleCompilerResults results;
	compileIssues = new VuoCompilerIssues;
	clangIssues = std::make_shared<VuoClangIssues>();
	headerSearchPaths = {};

	// Add the module PCH files' sysroot as the first header search path.
	sysroot = settings.vuoFrameworkPath + "/Headers";
	headerSearchPaths.push_back(sysroot);

	// Add the directory containing the generic header and source files for compound types as the next header search path.
	headerSearchPaths.push_back(settings.vuoFrameworkPath + "/Modules");

	// Since we'll be feeding the module source file to Clang through a virtual file path, add the original source directory
	// back in as the next header search path.
	string sourceDir, file, ext;
	VuoFileUtilities::splitPath(sourcePath, sourceDir, file, ext);
	headerSearchPaths.push_back(sourceDir);

	headerSearchPaths.insert(headerSearchPaths.end(), settings.headerSearchPaths.begin(), settings.headerSearchPaths.end());

	for (string &headerSearchPath : headerSearchPaths)
		VuoFileUtilities::canonicalizePath(headerSearchPath);

	try
	{
		// Cases:
		// 1. The module is a fully-specialized generic module.
		//    a. The caller passed settings.typeNameReplacements. We should replace generic types using that mapping.
		//    b. The source path is a placeholder that refers to a specialization of a generic type in the same directory.
		//       We should figure out the mapping of generic to specialized types from the file name.
		// 2. The module is an unspecialized generic module.
		//    a. The module is a node class. We should replace each generic type with its default backing type.
		//    b. The module is a type. We should substitute an abridged version of the source code.
		// 3. The module does not refer to any generic types (except perhaps in the paths of included headers).

		string transformedSourceCode;
		unique_ptr<VuoFileUtilities::File> genericSourceFile;
		unique_ptr<VuoFileUtilities::File> genericHeaderFile;
		string genericPrefix;

		if (! sourceCode.empty())
		{
			transformedSourceCode = sourceCode;
		}
		else if (VuoFileUtilities::fileExists(sourcePath))
		{
			transformedSourceCode = VuoFileUtilities::readFileToString(sourcePath);
		}
		else
		{
			genericSourceFile = findGenericSourceFile(sourcePath, genericPrefix, genericHeaderFile);
			if (genericSourceFile)
			{
				transformedSourceCode = genericSourceFile->getContentsAsString();
				settings.typeNameReplacements = VuoCompilerCompoundType::parseSpecializedTypes(moduleKey, genericPrefix);
			}
			else
			{
				throw VuoException("File does not exist: '" + sourcePath + "'");
			}
		}

		typeNameReplacements = (! settings.typeNameReplacements.empty() ?
									settings.typeNameReplacements :
									parseBackingTypesForGenericTypes(transformedSourceCode));

		precompiledHeaderPath = findPrecompiledHeaderFile(transformedSourceCode, genericPrefix);

		bool isUnspecializedGenericType = settings.typeNameReplacements.empty() && ! typeNameReplacements.empty() &&
										  isGenericType(transformedSourceCode, moduleKey, typeNameReplacements.size());
		if (isUnspecializedGenericType)
		{
			transformedSourceCode = abridgeUnspecializedGenericType(transformedSourceCode);
		}
		else
		{
			for (auto i : typeNameReplacements)
				lookUpVuoType(i.second);

			bool isFullySpecializedGenericModule = ! settings.typeNameReplacements.empty();
			if (isFullySpecializedGenericModule)
			{
				string genericModuleKey = ! genericPrefix.empty() ? genericPrefix : moduleKey;

				if (VuoNodeClass::isNodeClassName(moduleKey))
				{
					specializedModuleDetails = VuoCompilerSpecializedNodeClass::buildSpecializedModuleDetails(typeNameReplacements, genericModuleKey);
				}
				else
				{
					if (isType(transformedSourceCode, genericModuleKey, typeNameReplacements.size()))
					{
						VuoCompilerType *genericType = lookUpVuoType(genericModuleKey);
						specializedModuleDetails = VuoCompilerCompoundType::buildSpecializedModuleDetails(genericType, typeNameReplacements, vuoTypes);
					}
				}
			}

			transformedSourceCode = preprocess(transformedSourceCode);

			if (! typeNameReplacements.empty())
				transformedSourceCode = replaceGenericTypes(transformedSourceCode);
		}

		results.module = compileTransformedSourceCode(transformedSourceCode, llvmQueue, results.makeDependencies);
	}
	catch (VuoCompilerException &e)
	{
		compileIssues->append(e.getIssues());
	}
	catch (VuoException &e)
	{
		VuoCompilerIssue issue(VuoCompilerIssue::Error, "compiling module", sourcePath, "", e.what());
		compileIssues->append(issue);
	}

	if (! compileIssues->isEmpty() || ! clangIssues->isEmpty())
	{
		VuoCompilerIssue::IssueType issueType = compileIssues->hasErrors() || clangIssues->hasErrors() ?
													VuoCompilerIssue::Error :
													VuoCompilerIssue::Warning;

		vector<string> replacementsList;
		for (auto i : typeNameReplacements)
			replacementsList.push_back(i.first + " -> " + i.second);
		string replacementsStr = VuoStringUtilities::join(replacementsList, ", ");

		string details = "Failed to compile %moduleKey";
		if (! typeNameReplacements.empty())
			details += " [" + replacementsStr + "]";

		VuoCompilerIssue issue(issueType, "compiling module", sourcePath, "", details);
		issue.setModuleKey(moduleKey);
		issue.setClangIssues(clangIssues);
		issues->append(issue);
		issues->append(compileIssues);
	}

	return results;
}

/**
 * Returns header file contents for the specialized-generic module described by `sourcePath`, the path of the
 * generic module's implementation file with the specialized type(s) appended to the file name.
 *
 * Example: If `sourcePath` is `vuo/type/compound/VuoList_VuoInteger.cc`, this function locates the gemeric header
 * file at `vuo/type/compound/VuoList.h` and specializes it by replacing `VuoGenericType1` with `VuoInteger`.
 */
string VuoCModuleCompiler::generateHeader(VuoCompilerIssues *issues)
{
	string genericPrefix;
	unique_ptr<VuoFileUtilities::File> genericHeaderFile;
	unique_ptr<VuoFileUtilities::File> genericSourceFile = findGenericSourceFile(sourcePath, genericPrefix, genericHeaderFile);
	if (! genericSourceFile)
	{
		VuoCompilerIssue issue(VuoCompilerIssue::Error, "generating header", sourcePath, "", "Generic module implementation not found: '" + sourcePath + "'");
		issue.setModuleKey(moduleKey);
		issues->append(issue);
	}
	else if (! genericHeaderFile)
	{
		VuoCompilerIssue issue(VuoCompilerIssue::Error, "generating header", sourcePath, "", "Generic module interface not found: '" + sourcePath + "'");
		issue.setModuleKey(moduleKey);
		issues->append(issue);
	}

	return generateContentsForSpecializedHeader(moduleKey, genericPrefix, genericHeaderFile.get());
}

/**
 * Returns a virtual directory path to represent the locations of source files generated by this class.
 */
string VuoCModuleCompiler::getVirtualSourceDirectory(void)
{
	return "/VuoCModuleCompiler/";
}

/**
 * Returns a virtual file path to represent the location of the module's in-memory source code.
 */
string VuoCModuleCompiler::getVirtualSourcePath(void)
{
	string dir, file, ext;
	VuoFileUtilities::splitPath(sourcePath, dir, file, ext);
	return getVirtualSourceDirectory() + file + "." + ext;
}

/**
 * Constructs a compiler instance that is ready to execute an action with @a inputSourceCode as the input.
 *
 * @throw VuoCompilerException Diagnostics could not be set up for the compiler.
 */
unique_ptr<CompilerInstance> VuoCModuleCompiler::createCompilerInstance(const string &inputSourceCode,
																		bool shouldInsertExternDeclarations, bool shouldEnableAllWarnings,
																		bool shouldGenerateDependencyFile, string &dependencyFilePath,
																		IntrusiveRefCntPtr<vfs::InMemoryFileSystem> &inMemoryFileSystem)
{
	vector<const char *> args;

	// Add a virtual input file from which the source code string will be read from memory.
	string placeholderSourcePath = getVirtualSourcePath();
	map<string, string> virtualToRealFilePaths{{placeholderSourcePath, sourcePath}};
	args.push_back(placeholderSourcePath.c_str());

	args.push_back("-DVUO_COMPILER");
	args.push_back("-fblocks");

	// Provide full backtraces, for easier debugging using Instruments and `VuoLog_backtrace()`.
	// The Clang driver translates `-fno-omit-frame-pointer` to `clang -cc1`'s `-mdisable-fp-elim`.
	// In Clang 10, this was renamed to `-mframe-pointer=all`.
	// https://b33p.net/kosada/vuo/vuo/-/issues/19064
	args.push_back("-mdisable-fp-elim");

	// Fix "NEON support not enabled" compile error when the module uses the Neon extension on ARM targets
	// (e.g. via Accelerate.framework).
	if (VuoCompiler::getTargetArch(settings.target) == "arm64")
	{
		args.push_back("-target-feature");
		args.push_back("+neon");
	}

	if (shouldEnableAllWarnings)
	{
		// Sync with /CMakeLists.txt's `commonFlags`.
		args.push_back("-Wall");
		args.push_back("-Wextra");
		args.push_back("-Wimplicit-fallthrough");
		args.push_back("-Wno-unused-parameter");
		args.push_back("-Wno-sign-compare");
		args.push_back("-Werror=implicit");
		args.push_back("-Wtautological-compare");
	}
	else
	{
		// When processing source code that still contains generic type names in function names, avoid reporting errors/warnings
		// that will be resolved once the specialized type names are substituted in. (This doesn't avoid all of them;
		// the remainder can only be avoided by modifying the source code, e.g. adding an 'extern' function declaration.)
		args.push_back("-Wno-implicit-function-declaration");
		args.push_back("-Wno-int-conversion");
	}

	string sourceDir, sourceFile, sourceExt;
	VuoFileUtilities::splitPath(sourcePath, sourceDir, sourceFile, sourceExt);

	if (VuoFileUtilities::isCPlusPlusSourceExtension(sourceExt) || VuoFileUtilities::isObjectiveCPlusPlusSourceExtension(sourceExt))
	{
		args.push_back("-xobjective-c++");
		args.push_back("-std=c++14");
		args.push_back("-stdlib=libc++");
		args.push_back("-fcxx-exceptions");
	}
	else
	{
		args.push_back("-xobjective-c");
		args.push_back("-std=c99");
	}

	args.push_back("-fexceptions");
	args.push_back("-fobjc-exceptions");

	// Since the PCH is relocatable, we pass the same sysroot and resource dir used when compiling the PCH
	// (see CMake function `VuoGenerateModulePCH`), and we provide a header and framework search paths for
	// system files that are not in the sysroot, without overriding the ones that are.

	args.push_back("-include-pch");
	args.push_back(precompiledHeaderPath.c_str());

	args.push_back("-isysroot");
	args.push_back(sysroot.c_str());

	string resourceDir = sysroot + "/clang";
	args.push_back("-resource-dir");
	args.push_back(resourceDir.c_str());

	string virtualSourceDir = getVirtualSourceDirectory();
	args.push_back("-I");
	args.push_back(virtualSourceDir.c_str());

	for (auto i = headerSearchPaths.begin(); i != headerSearchPaths.end(); ++i)
	{
		args.push_back("-I");
		args.push_back(i->c_str());
	}

	vector<string> systemHeaderSearchPaths = {
		sysroot + "/clang/include",
		sysroot + "/usr/include/c++/v1",
		sysroot + "/usr/include",
		sysroot + "/usr/include/pthread",
		settings.macOSSDKPath + "/usr/include"
	};
	for (auto i = systemHeaderSearchPaths.begin(); i != systemHeaderSearchPaths.end(); ++i)
	{
		args.push_back("-isystem");
		args.push_back(i->c_str());
	}

	string systemFrameworkSearchPath = settings.macOSSDKPath + "/System/Library/Frameworks";
	args.push_back("-iframework");
	args.push_back(systemFrameworkSearchPath.c_str());

	string compiledFilePathInDependencyFile;
	if (shouldGenerateDependencyFile)
	{
		dependencyFilePath = VuoFileUtilities::makeTmpFile(moduleKey, "d");
		compiledFilePathInDependencyFile = VuoMakeDependencies::getPlaceholderCompiledFilePath();

		// https://bugzilla.mozilla.org/show_bug.cgi?id=1340588#c4
		// https://lists.llvm.org/pipermail/cfe-users/2018-March/001268.html
		args.push_back("-MT");
		args.push_back(compiledFilePathInDependencyFile.c_str());
		args.push_back("-dependency-file");
		args.push_back(dependencyFilePath.c_str());
	}

	if (settings.isVerbose)
	{
		args.push_back("-v");

		ostringstream s;
		s << "clang -cc1 ";
		for (vector<const char *>::iterator i = args.begin(); i != args.end(); ++i)
			s << *i << " ";
		VUserLog("%s", s.str().c_str());
	}

	// Set up a temporary diagnostics engine to store errors encountered while parsing args.
	IntrusiveRefCntPtr<DiagnosticIDs> diagID(new DiagnosticIDs());
	IntrusiveRefCntPtr<DiagnosticOptions> diagOptions(new DiagnosticOptions());
	TextDiagnosticBuffer *diagBuffer = new TextDiagnosticBuffer;
	DiagnosticsEngine compilerInvocationDiags(diagID, &*diagOptions, diagBuffer);

	// This invokes `clang -cc1`.  See `clang -cc1 --help` for available options.
	shared_ptr<CompilerInvocation> compilerInvocation(new CompilerInvocation);
	bool argsOk = CompilerInvocation::CreateFromArgs(*compilerInvocation, &args[0], &args[0] + args.size(), compilerInvocationDiags);

	compilerInvocation->TargetOpts->Triple = settings.target;

	unique_ptr<MemoryBuffer> buffer = MemoryBuffer::getMemBufferCopy(inputSourceCode);
	compilerInvocation->getPreprocessorOpts().addRemappedFile(placeholderSourcePath, buffer.get());
	buffer.release();

	unique_ptr<CompilerInstance> compilerInstance(new CompilerInstance);
	compilerInstance->setInvocation(compilerInvocation);

	// Set up the actual diagnostics for reporting issues, respecting the args to enable/disable warnings.
	compilerInstance->createDiagnostics();
	if (! compilerInstance->hasDiagnostics())
	{
		VuoCompilerIssue issue(VuoCompilerIssue::Error, "compiling module", sourcePath, "", "Failed to set up Clang diagnostics.");
		throw VuoCompilerException(issue);
	}

	VuoCompilerDiagnosticConsumer *diagnosticConsumer = new VuoCompilerDiagnosticConsumer(clangIssues, virtualToRealFilePaths);
	compilerInstance->getDiagnostics().setClient(diagnosticConsumer);

	diagBuffer->FlushDiagnostics(compilerInstance->getDiagnostics());
	if (! argsOk)
	{
		VuoCompilerIssue issue(VuoCompilerIssue::Error, "compiling module", sourcePath, "", "Clang could not parse arguments.");
		throw VuoCompilerException(issue);
	}

	inMemoryFileSystem = setUpVirtualFileSystem(compilerInstance.get(), shouldInsertExternDeclarations);

	return compilerInstance;
}

/**
 * Sets up a virtual file system with in-memory file contents for:
 * - A header file that typedefs generic type names to specialized type names.
 * - Each header file that the module depends on that is embedded within a node set (zip file).
 *
 * The caller can add additional files to @a inMemoryFileSystem after this call.
 */
IntrusiveRefCntPtr<vfs::InMemoryFileSystem> VuoCModuleCompiler::setUpVirtualFileSystem(CompilerInstance *compilerInstance,
																					   bool shouldInsertExternDeclarations)
{
	vector< pair<string, string> > inMemoryIncludeFiles;

	if (! typeNameReplacements.empty())
	{
		string virtualSourceDirectory = getVirtualSourceDirectory();

		string preface = string() +
						 "#ifdef __cplusplus\n" +
						 "extern \"C\" {\n" +
						 "#endif\n";

		auto addExternDeclaration = [&preface] (const string &declarationTemplate, string replacementGenericType)
		{
			string templateGenericType = "VuoGenericType1";
			replacementGenericType = VuoType::extractInnermostTypeName(replacementGenericType);
			vector<string> unused;
			string declaration = declarationTemplate;
			VuoGenericType::replaceGenericTypeNamesInString(declaration, {{templateGenericType, replacementGenericType}}, unused);
			preface += declaration + "\n";
		};

		// For each specialized type, insert additional source code:
		// - an #include for the type's header file
		// - a typedef for the corresponding generic type
		// - extern declarations for functions known to generate "assigning to an incompatible type" errors when first parsing the AST

		for (auto i : typeNameReplacements)
		{
			preface += "#include \"" + i.second + ".h\"\n";
			preface += "typedef " + i.second + " " + i.first + ";\n";

			if (shouldInsertExternDeclarations)
				addExternDeclaration("extern VuoGenericType1 VuoGenericType1_makeFromJson(struct json_object *);", i.first);

			includedHeaders.insert(i.second + ".h");
		}

		// For each specialized header file encountered in an include directive during a previous Clang front-end action, insert:
		// - an #include for the specialized header file
		// - a typedef from the generic header file name (assumed to be a module key) to the specialized header file name
		// - extern declarations for functions known to generate "assigning to an incompatible type" errors when first parsing the AST

		for (auto i : typeNameReplacementsFromIncludedHeaders)
		{
			preface += "#include \"" + i.second + ".h\"\n";
			preface += "typedef " + i.second + " " + i.first + ";\n";

			if (shouldInsertExternDeclarations)
				addExternDeclaration("extern VuoGenericType1 VuoListGetValue_VuoGenericType1(const VuoList_VuoGenericType1, const unsigned long);", i.first);
		}

		preface += string() +
				   "#ifdef __cplusplus\n" +
				   "}\n" +
				   "#endif\n";

		auto isIdentifierAvailable = [this](const string &file){ return ! findHeaderFile(file + ".h"); };
		string prefaceFile = VuoStringUtilities::formUniqueIdentifier(isIdentifierAvailable, "preface");
		string prefacePath = virtualSourceDirectory + prefaceFile + ".h";

		inMemoryIncludeFiles.push_back({prefacePath, preface});
		compilerInstance->getPreprocessorOpts().Includes.push_back(prefacePath);

		// Add in-memory include files for header files embedded within the module's node set
		// or any other node set that this module depends on for its specialized types.

		vector<VuoNodeSet *> nodeSetsWithHeaderFiles;

		if (settings.nodeSet)
			nodeSetsWithHeaderFiles.push_back(settings.nodeSet);

		for (auto i : typeNameReplacements)
		{
			VuoCompilerType *type = vuoTypes[i.second];
			if (type)
			{
				VuoNodeSet *nodeSet = type->getBase()->getNodeSet();
				if (nodeSet)
					nodeSetsWithHeaderFiles.push_back(nodeSet);
			}
		}

		for (VuoNodeSet *nodeSet : nodeSetsWithHeaderFiles)
		{
			vector<string> headersInNodeSet = nodeSet->getHeaderPaths();
			vector<string> includedHeadersInNodeSet;
			std::set_intersection(headersInNodeSet.begin(), headersInNodeSet.end(),
								  includedHeaders.begin(), includedHeaders.end(),
								  std::back_inserter(includedHeadersInNodeSet));

			for (const string &headerPath : includedHeadersInNodeSet)
			{
				if (! findHeaderFile(headerPath))
				{
					string headerFileContents = nodeSet->getFileContents(headerPath);
					inMemoryIncludeFiles.push_back({virtualSourceDirectory + headerPath, headerFileContents});
				}
			}
		}
	}

	// Set up a file system that provides in-memory contents for the placeholder include paths
	// but otherwise behaves like the regular file system.

	compilerInstance->createFileManager();

	IntrusiveRefCntPtr<vfs::InMemoryFileSystem> inMemoryFileSystem(new vfs::InMemoryFileSystem);

	for (auto i : inMemoryIncludeFiles)
		addVirtualFileToFileSystem(inMemoryFileSystem.get(), i.first, i.second);

	IntrusiveRefCntPtr<vfs::OverlayFileSystem> overlayFileSystem(new vfs::OverlayFileSystem(compilerInstance->getFileManager().getVirtualFileSystem()));
	overlayFileSystem->pushOverlay(inMemoryFileSystem);

	compilerInstance->setVirtualFileSystem(overlayFileSystem);
	compilerInstance->setFileManager(new FileManager(FileSystemOptions{}, overlayFileSystem));
	compilerInstance->createSourceManager(compilerInstance->getFileManager());

	return inMemoryFileSystem;
}

/**
 * Adds a virtual file to @a inMemoryFileSystem.
 *
 * @throw VuoCompilerException Adding to the in-memory file system failed.
 */
void VuoCModuleCompiler::addVirtualFileToFileSystem(vfs::InMemoryFileSystem *inMemoryFileSystem, const string &virtualPath, const string &contents)
{
	unique_ptr<MemoryBuffer> buffer = MemoryBuffer::getMemBufferCopy(contents);
	time_t modificationTime = time(NULL);
	bool added = inMemoryFileSystem->addFile(virtualPath, modificationTime, std::move(buffer));

	if (! added)
	{
		VuoCompilerIssue issue(VuoCompilerIssue::Error, "compiling module", sourcePath, "", "Failed to add in-memory file \"" + virtualPath + "\"");
		throw VuoCompilerException(issue);
	}
}

/**
 * If @a path refers to a specialization of a generic header file (e.g. `VuoList_VuoInteger.h` or `VuoList_VuoGenericType1.h`
 * as specializations of `VuoList.h`), creates a virtual file containing the specialized header contents.
 *
 * @param inMemoryFileSystem The file system in which to create the virtual file.
 * @param path The path that appears in an include directive.
 * @param shouldDefineGenericTypes If true, this function will replace generic type names in the header file name and contents
 *    with specialized type names.
 * @return If a virtual file was created, returns the directory in which it was placed. Otherwise, returns an empty string.
 */
string VuoCModuleCompiler::generateVirtualFileForSpecializedHeader(vfs::InMemoryFileSystem *inMemoryFileSystem, const string &path,
																   bool shouldDefineGenericTypes)
{
	string dir, file, ext;
	VuoFileUtilities::splitPath(path, dir, file, ext);

	string genericPrefix;
	unique_ptr<VuoFileUtilities::File> genericHeaderFile = findGenericHeaderFile(file, genericPrefix);

	if (genericHeaderFile)
	{
		// Place the file in a unique directory for use by `VuoPreprocessorCallbacks::FileNotFound()`.
		auto isIdentifierAvailable = [inMemoryFileSystem](const string &dir){ return ! inMemoryFileSystem->exists(dir); };
		string virtualDirectory = VuoStringUtilities::formUniqueIdentifier(isIdentifierAvailable, "specialized");
		string virtualPath = virtualDirectory + "/" + file + "." + ext;

		string specializedModuleKey = file;
		vector<string> unused;
		VuoGenericType::replaceGenericTypeNamesInString(specializedModuleKey, typeNameReplacements, unused);

		string moduleKeyForContents = shouldDefineGenericTypes ? specializedModuleKey : file;
		string contents = generateContentsForSpecializedHeader(moduleKeyForContents, genericPrefix, genericHeaderFile.get());

		// Identify the file so that:
		// - Clang doesn't skip over it when preprocessing include directives, thinking it's already been included
		// - Clang's error reporting refers to the specialized file name instead of the generic one
		contents = "#line 1 \"" + virtualDirectory + "/" + moduleKeyForContents + "." + ext + "\"\n" +
				   contents;

		if (file != specializedModuleKey &&
				typeNameReplacementsFromIncludedHeaders.find(file) == typeNameReplacementsFromIncludedHeaders.end())
		{
			VuoFileUtilities::File genericSourceFile = genericHeaderFile->fileWithDifferentExtension("cc");
			if (genericSourceFile.exists())
			{
				size_t genericTypeCount = VuoStringUtilities::split(VuoStringUtilities::substrAfter(file, genericPrefix + "_"), '_').size();
				if (isGenericType(genericSourceFile.getContentsAsString(), genericPrefix, genericTypeCount))
					typeNameReplacementsFromIncludedHeaders[file] = specializedModuleKey;
			}
		}

		try
		{
			addVirtualFileToFileSystem(inMemoryFileSystem, virtualPath, contents);
			return virtualDirectory;
		}
		catch (VuoCompilerException &e)
		{
			compileIssues->append(e.getIssues());
		}
	}
	else
	{
		VuoCompilerIssue issue(VuoCompilerIssue::Warning, "compiling module", sourcePath, "", "Generic header not found for: '" + path + "'");
		compileIssues->append(issue);
	}

	return "";
}

/**
 * Returns a version of the generic header file (whose name corresponds to the first part of
 * @a specializedModuleKey) in which all generic types are replaced by specialized types
 * (found in the second part of @a specializedModuleKey).
 *
 * The first type name in the file name replaces `VuoGenericType1`, the second replaces
 * `VuoGenericType2`, etc.
 */
string VuoCModuleCompiler::generateContentsForSpecializedHeader(const string &specializedModuleKey, const string &genericPrefix,
																VuoFileUtilities::File *genericHeaderFile)
{
	map<string, string> headerTypeNameReplacements = VuoCompilerCompoundType::parseSpecializedTypes(specializedModuleKey, genericPrefix);

	string headerSourceCode = genericHeaderFile->getContentsAsString();

	// When a generic module includes a generic header, VuoGenericType1 in the module isn't necessarily
	// the same thing as VuoGenericType1 in the header. For example, if a module has `#include "VuoList_VuoGenericType2.h"`,
	// then each instance of `VuoGenericType1` in VuoList.h should be replaced with `VuoGenericType2`,
	// and will subsequently be replaced with whatever `VuoGenericType2` is specialized to.
	//
	// For the module, we use VuoReplaceGenericTypesWithSpecializedAction to replace generic type names that are
	// part of an identifier, and we use typedefs to replace generic type names that comprise the entire identifier.
	//
	// For a header included in the module, we can't use typedefs since they'll conflict with the module's typedefs.
	// If VuoReplaceGenericTypesWithSpecializedAction were able to replace all generic type names, making the typedefs
	// unnecessary, then we could use that, but that's a lot of language constructs to handle. So instead we use
	// a simple string replacement.
	vector<string> unused;
	VuoGenericType::replaceGenericTypeNamesInString(headerSourceCode, headerTypeNameReplacements, unused);

	return headerSourceCode;
}

/**
 * If Clang has written a dependency file, containing the list of source files required to compile the module,
 * replace or delete any virtual paths in the file.
 */
void VuoCModuleCompiler::replaceVirtualPathsInDependencyFile(shared_ptr<VuoMakeDependencies> makeDependencies, vfs::InMemoryFileSystem *inMemoryFileSystem)
{
	// Example replacements:
	//   - /VuoCModuleCompiler/preface.h             -> (nothing)
	//   - /VuoCModuleCompiler/VuoInteger.c          -> /…/type/VuoInteger.c
	//   - /VuoCModuleCompiler/VuoList_VuoInteger.cc -> /…/type/compound/VuoList.h /…/type/compound/VuoList.cc /…/type/VuoInteger.h
	//   - specialized/VuoList_VuoInteger.h          -> /…/type/compound/VuoList.h /…/type/VuoInteger.h

	vector<string> originalDependencies = makeDependencies->getDependencyPaths();
	vector<string> modifiedDependencies;

	for (const string &dependency : originalDependencies)
	{
		if (dependency == getVirtualSourcePath() || inMemoryFileSystem->exists(dependency))
		{
			if (dependency == getVirtualSourcePath() && VuoFileUtilities::fileExists(sourcePath))
			{
				modifiedDependencies.push_back(sourcePath);
			}
			else
			{
				string dir, file, ext;
				VuoFileUtilities::splitPath(dependency, dir, file, ext);

				string genericPrefix;
				unique_ptr<VuoFileUtilities::File> genericHeaderFile = findGenericHeaderFile(file, genericPrefix);

				if (genericHeaderFile)
				{
					modifiedDependencies.push_back(genericHeaderFile->path());

					if (ext == "cc")
					{
						VuoFileUtilities::File genericSourceFile = genericHeaderFile->fileWithDifferentExtension("cc");
						modifiedDependencies.push_back(genericSourceFile.path());
					}

					map<string, string> typeMapping = VuoCompilerCompoundType::parseSpecializedTypes(file, genericPrefix);
					for (auto i : typeMapping)
					{
						unique_ptr<VuoFileUtilities::File> typeHeaderFile = findHeaderFile(i.second + ".h");
						if (typeHeaderFile)
							modifiedDependencies.push_back(typeHeaderFile->path());
					}
				}
			}
		}
		else
		{
			modifiedDependencies.push_back(dependency);
		}
	}

	std::sort(modifiedDependencies.begin(), modifiedDependencies.end());
	auto endOfUnique = std::unique(modifiedDependencies.begin(), modifiedDependencies.end());
	modifiedDependencies.erase(endOfUnique, modifiedDependencies.end());

	makeDependencies->setDependencyPaths(modifiedDependencies);
}

/**
 * Expands macros in the source code (and does some other preprocessing).
 *
 * @throw VuoCompilerException The source code could not be parsed.
 */
string VuoCModuleCompiler::preprocess(const string &inputSourceCode)
{
	// Replace generic type names in #ifdefs. When the module being compiled is a specialized compound type
	// (e.g. VuoList_VuoAnchor), this allows the preprocessor to evaluate the #ifdef with the macro defined
	// in the specialized type (e.g. VuoAnchor_SUPPORTS_COMPARISON) rather than a placeholder macro that
	// is never defined (e.g. VuoGenericType1_SUPPORTS_COMPARISON).

	ostringstream macrosReplacedStream;

	VuoStringUtilities::doForEachLine(inputSourceCode, [this, &macrosReplacedStream](const char *lineArr)
	{
		string line = lineArr;

		if (VuoStringUtilities::beginsWith(line, "#ifdef "))
		{
			vector<string> unused;
			VuoGenericType::replaceGenericTypeNamesInString(line, typeNameReplacements, unused);
		}

		macrosReplacedStream << line << endl;
	});

	string outputSourceCode = macrosReplacedStream.str();

	// Run the module source code through Clang's preprocessor.
	// In the output source code, macros have been expanded, #line directives have been added,
	// and #included file contents have been inserted, while the #include directives have also been preserved.

	IntrusiveRefCntPtr<vfs::InMemoryFileSystem> inMemoryFileSystem;
	string unused;
	unique_ptr<CompilerInstance> compilerInstance = createCompilerInstance(outputSourceCode, false, false, false, unused, inMemoryFileSystem);

	outputSourceCode = "";
	raw_string_ostream output(outputSourceCode);

	unique_ptr<VuoPreprocessorCallbacks> preprocessorCallbacks(new VuoPreprocessorCallbacks(this, inMemoryFileSystem.get(), false));

	VuoPreprocessAction action(output, std::move(preprocessorCallbacks));

	bool ok = compilerInstance->ExecuteAction(action);
	if (! ok || compileIssues->hasErrors() || clangIssues->hasErrors())
	{
		VuoCompilerIssue issue(VuoCompilerIssue::Error, "compiling module", sourcePath, "", "Failed to preprocess the source code.");
		throw VuoCompilerException(issue);
	}

	output.flush();

	// Search the source code for any generic compound type names. For each one, generate a header file that will be
	// automatically #included in subsequent CompilerInstance passes (so the module developer doesn't have to add it).

	for (auto i : typeNameReplacements)
	{
		string compoundTypeName = "VuoList_" + i.first;
		if (outputSourceCode.find(compoundTypeName) != string::npos)
			generateVirtualFileForSpecializedHeader(inMemoryFileSystem.get(), compoundTypeName + ".h", true);
	}

	// Remove all lines inserted from #included files in order to:
	// - avoid a slew of compile errors that occur if you pass the raw preprocessed code to a subsequent Clang compilation
	// - make it feasible to debug the subsequent source code transformations

	ostringstream prunedSourceCodeStream;

	string virtualSourcePath = getVirtualSourcePath();
	string currentPath = virtualSourcePath;

	VuoStringUtilities::doForEachLine(outputSourceCode, [this, &prunedSourceCodeStream, &virtualSourcePath, &currentPath](const char *line)
	{
		string path = extractPath(line, "#line ");
		if (! path.empty())
			currentPath = path;

		if (currentPath == virtualSourcePath)
			prunedSourceCodeStream << line << endl;
	});

	return prunedSourceCodeStream.str();
}

/**
 * Replaces the generic types in @a inputSourceCode with their specializations and returns
 * the resulting source code.
 *
 * @throw VuoCompilerException The source code could not be parsed into an AST.
 */
string VuoCModuleCompiler::replaceGenericTypes(const string &inputSourceCode)
{
	// Replaces generic type names within function names and annotations.
	// Adds the specialized module's additional metadata to the metadata already in the generic module.

	IntrusiveRefCntPtr<vfs::InMemoryFileSystem> inMemoryFileSystem;
	string unused;
	unique_ptr<CompilerInstance> compilerInstance = createCompilerInstance(inputSourceCode, true, false, false, unused, inMemoryFileSystem);

	string outputSourceCode;
	raw_string_ostream output(outputSourceCode);

	unique_ptr<VuoPreprocessorCallbacks> preprocessorCallbacks(new VuoPreprocessorCallbacks(this, inMemoryFileSystem.get(), true));

	VuoReplaceGenericTypesWithSpecializedAction action(output, typeNameReplacements, ! settings.typeNameReplacements.empty(),
													   specializedModuleDetails, std::move(preprocessorCallbacks));
	bool ok = compilerInstance->ExecuteAction(action);

	if (! ok || compileIssues->hasErrors() || clangIssues->hasErrors())
	{
		VuoCompilerIssue issue(VuoCompilerIssue::Error, "compiling module", sourcePath, "", "Failed to replace generic types with specialized types.");
		throw VuoCompilerException(issue);
	}

	output.flush();
	return outputSourceCode;
}

/**
 * Returns a minimal type definition that gives just enough information for the Vuo compiler to load the type
 * and generate specialized types based on it.
 *
 * The minimal type definition avoids complications of compiling the full unspecialized definition:
 *
 * - It renames the Vuo type functions so they are prefixed with just the module key (e.g. `VuoList`)
 *   rather than the module key plus generic types (e.g. `VuoList_VuoGenericType1`). This enables
 *   `VuoCompilerType` to look up the Vuo type functions based on the module key just as it does for
 *   any other type, and it makes `VuoCompilerType` and `VuoCompilerNodeClass` consistent in how they
 *   handle unspecialized generic types.
 *
 * - It removes all code containing generic types. This avoids wasting time replacing generic types
 *   (and debugging problems with doing so) in module code that is never going to be used.
 */
string VuoCModuleCompiler::abridgeUnspecializedGenericType(const string &inputSourceCode)
{
	return string() +
			"#include \"module.h\"\n" +
			"#ifdef __cplusplus\n" +
			"extern \"C\" {\n" +
			"#endif\n" +
			"VuoModuleMetadata(" + findMetadata(inputSourceCode) + ");\n" +
			"typedef int * " + moduleKey + ";\n" +
			moduleKey + " " + moduleKey + "_makeFromJson(json_object *js) { return NULL; }\n" +
			"json_object * " + moduleKey + "_getJson(const " + moduleKey + " value) { return NULL; }\n" +
			"char * " + moduleKey + "_getSummary(const " + moduleKey + " value) { return NULL; }\n" +
			"#ifdef __cplusplus\n" +
			"}\n" +
			"#endif\n";
}

/**
 * Compiles the source code to LLVM bitcode.
 *
 * @throw VuoCompilerException The source code could not be parsed into an AST.
 */
llvm::Module * VuoCModuleCompiler::compileTransformedSourceCode(const string &inputSourceCode, dispatch_queue_t llvmQueue,
																shared_ptr<VuoMakeDependencies> &makeDependencies)
{
	__block llvm::Module *module = nullptr;

	dispatch_sync(llvmQueue, ^{
		IntrusiveRefCntPtr<vfs::InMemoryFileSystem> inMemoryFileSystem;
		string dependencyFilePath;
		unique_ptr<CompilerInstance> compilerInstance = createCompilerInstance(inputSourceCode, false, true, true, dependencyFilePath, inMemoryFileSystem);

		unique_ptr<VuoPreprocessorCallbacks> preprocessorCallbacks(new VuoPreprocessorCallbacks(this, inMemoryFileSystem.get(), true));

		// Pass in an LLVM context that will outlive the EmitLLVMOnlyAction so that the generated Module
		// can still be used after the EmitLLVMOnlyAction is destroyed.
		VuoEmitLLVMOnlyAction action(VuoCompiler::globalLLVMContext, std::move(preprocessorCallbacks));

		// If the action fails, VuoCompilerDiagnosticConsumer appends to `clangIssues`.
		bool ok = compilerInstance->ExecuteAction(action);

		if (ok)
		{
			unique_ptr<llvm::Module> modulePtr = action.takeModule();
			if (modulePtr)
			{
				module = modulePtr.release();

				try
				{
					makeDependencies = VuoMakeDependencies::createFromFile(dependencyFilePath);
					replaceVirtualPathsInDependencyFile(makeDependencies, inMemoryFileSystem.get());
				}
				catch (VuoException &e) {}

				VuoFileUtilities::deleteFile(dependencyFilePath);
			}
			else
			{
				VuoCompilerIssue issue(VuoCompilerIssue::Error, "compiling module", sourcePath, "", "Failed to generate an LLVM module.");
				compileIssues->append(issue);
			}
		}
	});

	return module;
}

/**
 * Returns the path name found in @a line after @a prefix.
 */
string VuoCModuleCompiler::extractPath(const char *line, const char *prefix)
{
	if (VuoStringUtilities::beginsWith(line, prefix))
	{
		int firstQuotePos = -1;
		int lastQuotePos = -1;

		for (int i = 0; i < strlen(line) && firstQuotePos == -1; ++i)
			if (line[i] == '"')
				firstQuotePos = i;

		for (int i = strlen(line) - 1; i >= 0 && lastQuotePos == -1; --i)
			if (line[i] == '"')
				lastQuotePos = i;

		if (firstQuotePos != -1 && lastQuotePos > firstQuotePos)
			return string(line, firstQuotePos + 1, lastQuotePos - firstQuotePos - 1);
	}

	return "";
}

/**
 * Returns the header file at the given absolute or relative @a path if one exists, otherwise null.
 *
 * If @a path is relative, looks for the header at each header search path (but not inside of node sets).
 */
unique_ptr<VuoFileUtilities::File> VuoCModuleCompiler::findHeaderFile(const string &path)
{
	string dir, file, ext;
	VuoFileUtilities::splitPath(path, dir, file, ext);

	if (VuoFileUtilities::isAbsolutePath(path))
	{
		if (VuoFileUtilities::fileExists(path))
		{
			VuoFileUtilities::canonicalizePath(dir);
			return unique_ptr<VuoFileUtilities::File>(new VuoFileUtilities::File(dir, file + "." + ext));
		}
	}
	else
	{
		for (string headerSearchPath : headerSearchPaths)
		{
			string headerPath = headerSearchPath + "/" + path;
			if (VuoFileUtilities::fileExists(headerPath))
				return unique_ptr<VuoFileUtilities::File>(new VuoFileUtilities::File(headerSearchPath, path));
		}
	}

	return nullptr;
}

/**
 * Returns the generic header file that corresponds to @a specializedModuleKey if one exists, otherwise null.
 */
unique_ptr<VuoFileUtilities::File> VuoCModuleCompiler::findGenericHeaderFile(const string &specializedModuleKey, string &genericPrefix)
{
	vector<string> parts = VuoStringUtilities::split(specializedModuleKey, '_');
	for (size_t i = 1; i < parts.size(); ++i)
	{
		vector<string> firstParts(parts.begin(), parts.begin() + i);
		string potentialGenericPrefix = VuoStringUtilities::join(firstParts, '_');

		unique_ptr<VuoFileUtilities::File> genericHeaderFile = findHeaderFile(potentialGenericPrefix + ".h");
		if (genericHeaderFile)
		{
			genericPrefix = potentialGenericPrefix;
			return genericHeaderFile;
		}
	}

	return nullptr;
}

/**
 * Returns the generic source file that corresponds to, and is located in the same directory as, @a specializedModuleKey,
 * if such a file exists, otherwise null.
 */
unique_ptr<VuoFileUtilities::File> VuoCModuleCompiler::findGenericSourceFile(const string &specializedPath, string &genericPrefix,
																			 unique_ptr<VuoFileUtilities::File> &genericHeaderFile)
{
	string dir, file, ext;
	VuoFileUtilities::splitPath(specializedPath, dir, file, ext);

	string potentialGenericPrefix;
	string potentialGenericSourceFileName;
	string potentialGenericHeaderFileName;
	bool foundGenericPrefix = false;

	auto checkGenericPrefix = [&foundGenericPrefix, dir, potentialGenericSourceFileName]()
	{
		foundGenericPrefix = VuoFileUtilities::fileExists(dir + potentialGenericSourceFileName);
		return foundGenericPrefix;
	};

	if (VuoNodeClass::isNodeClassName(file))
	{
		vector<string> parts = VuoStringUtilities::split(file, '.');
		for (int i = parts.size() - 1; i >= 1; --i)
		{
			vector<string> firstParts(parts.begin(), parts.begin() + i);
			potentialGenericPrefix = VuoStringUtilities::join(firstParts, '.');
			potentialGenericSourceFileName = potentialGenericPrefix + "." + ext;

			if (checkGenericPrefix())
				break;
		}
	}
	else
	{
		vector<string> parts = VuoStringUtilities::split(file, '_');
		for (size_t i = 1; i < parts.size(); ++i)
		{
			vector<string> firstParts(parts.begin(), parts.begin() + i);
			potentialGenericPrefix = VuoStringUtilities::join(firstParts, '_');
			potentialGenericSourceFileName = potentialGenericPrefix + ".cc";
			potentialGenericHeaderFileName = potentialGenericPrefix + ".h";

			if (checkGenericPrefix())
				break;
		}
	}

	if (foundGenericPrefix)
	{
		genericPrefix = potentialGenericPrefix;

		if (! potentialGenericHeaderFileName.empty() && VuoFileUtilities::fileExists(dir + potentialGenericHeaderFileName))
			genericHeaderFile = unique_ptr<VuoFileUtilities::File>(new VuoFileUtilities::File(dir, potentialGenericHeaderFileName));

		return unique_ptr<VuoFileUtilities::File>(new VuoFileUtilities::File(dir, potentialGenericSourceFileName));
	}

	return nullptr;
}

/**
 * Returns the path of the precompiled header needed to compile the module, based on the type of module,
 * language, and target architecture.
 *
 * @throw VuoCompilerException The precompiled header was not found at any of the header search paths.
 */
string VuoCModuleCompiler::findPrecompiledHeaderFile(const string &moduleSourceCode, const string &genericPrefix)
{
	string dir, file, ext;
	VuoFileUtilities::splitPath(sourcePath, dir, file, ext);

	string lang = "OBJC";
	if (VuoFileUtilities::isCPlusPlusSourceExtension(ext)
	 || VuoFileUtilities::isObjectiveCPlusPlusSourceExtension(ext))
		lang = "OBJCXX";

	string arch = VuoCompiler::getTargetArch(settings.target);

	string fileName = lang + "-" + arch + ".h.pch";
	unique_ptr<VuoFileUtilities::File> foundFile = findHeaderFile(fileName);

	if (! foundFile)
	{
		VuoCompilerIssue issue(VuoCompilerIssue::Error, "compiling module", sourcePath, "", "Precompiled header not found: '" + fileName + "'");
		throw VuoCompilerException(issue);
	}

	return foundFile->path();
}

/**
 * Returns a default actual type name for each generic type name found in @a moduleSourceCode.
 */
map<string, string> VuoCModuleCompiler::parseBackingTypesForGenericTypes(const string &moduleSourceCode)
{
	map<string, string> backingTypeForGeneric;

	// Scan the source code for any generic types. Assign them the default backing type.

	for (string genericTypeName : VuoGenericType::findGenericTypeNames(moduleSourceCode))
		backingTypeForGeneric[genericTypeName] = VuoCompilerGenericType::chooseBackingTypeName(genericTypeName, {});

	// Override the backing type for any generic types that have a default type or compatible types specified in the module metadata.

	string moduleMetadataString = findMetadata(moduleSourceCode);
	json_object *moduleMetadata = json_tokener_parse(moduleMetadataString.c_str());
	if (! moduleMetadata)
		return backingTypeForGeneric;

	vector<string> genericTypeNames;
	map<string, string> defaultTypeForGeneric;
	map<string, vector<string> > compatibleTypesForGeneric;
	VuoCompilerModule::parseGenericTypes(moduleMetadata, genericTypeNames, defaultTypeForGeneric, compatibleTypesForGeneric);

	json_object_put(moduleMetadata);

	for (string genericTypeName : genericTypeNames)
	{
		string defaultTypeName = defaultTypeForGeneric[genericTypeName];
		if (! defaultTypeName.empty())
		{
			backingTypeForGeneric[genericTypeName] = defaultTypeName;
		}
		else
		{
			vector<string> compatibleTypeNames = compatibleTypesForGeneric[genericTypeName];
			if (! compatibleTypeNames.empty())
				backingTypeForGeneric[genericTypeName] = VuoCompilerGenericType::chooseBackingTypeName(genericTypeName, compatibleTypeNames);
		}
	}

	return backingTypeForGeneric;
}

/**
 * Returns the JSON-formatted string within the `VuoModuleMetadata` macro in @a moduleSourceCode if found,
 * otherwise an empty string.
 */
string VuoCModuleCompiler::findMetadata(const string &moduleSourceCode)
{
	size_t metadataIdentifierPos = moduleSourceCode.find("VuoModuleMetadata");
	if (metadataIdentifierPos != string::npos)
	{
		size_t metadataStartPos = moduleSourceCode.find("(", metadataIdentifierPos) + 1;
		if (metadataStartPos != string::npos)
		{
			size_t metadataEndPos = moduleSourceCode.find(");\n", metadataStartPos);
			if (metadataEndPos != string::npos)
				return moduleSourceCode.substr(metadataStartPos, metadataEndPos - metadataStartPos);
		}
	}

	return "";
}

/**
 * Returns true if @a moduleSourceCode is probably the source code for a type — either a singleton type
 * called @a moduleKey or a compound type whose generic prefix is @a moduleKey.
 *
 * @see VuoCompilerType::isType()
 */
bool VuoCModuleCompiler::isType(const string &moduleSourceCode, const string &moduleKey, size_t genericTypeCount)
{
	string typeName = genericTypeCount == 0 ?
						  moduleKey :
						  VuoCompilerCompoundType::buildUnspecializedCompoundTypeName(moduleKey, genericTypeCount);

	// Searching the AST would be more robust, but a string search is good enough for all modules encountered so far.
	return moduleSourceCode.find(typeName + " " + typeName + "_makeFromJson(") != string::npos;
}

/**
 * Returns true if @a moduleSourceCode is probably the source code for a compound type whose name begins with
 * @a genericPrefix and whose definition refers to @a genericTypeCount generic types.
 */
bool VuoCModuleCompiler::isGenericType(const string &moduleSourceCode, const string &genericPrefix, size_t genericTypeCount)
{
	return genericTypeCount > 0 && isType(moduleSourceCode, genericPrefix, genericTypeCount);
}

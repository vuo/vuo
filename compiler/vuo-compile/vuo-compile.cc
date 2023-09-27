﻿/**
 * @file
 * vuo-compile implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <getopt.h>
#ifdef USING_VUO_FRAMEWORK
	#include <Vuo/Vuo.h>
#else
	#include "VuoCompiler.hh"
	#include "VuoCompilerComposition.hh"
	#include "VuoCompilerDelegate.hh"
	#include "VuoCompilerException.hh"
	#include "VuoCompilerIssue.hh"
	#include "VuoComposition.hh"
	#include "VuoException.hh"
	#include "VuoFileUtilities.hh"
	#include "VuoModuleCompiler.hh"
#endif

#include "VuoCompilerHash.h"

#include <iostream>

class VuoCompileCLIDelegate : public VuoCompilerDelegate
{
	VuoCompilerIssues *_issues;
public:
	VuoCompileCLIDelegate(VuoCompilerIssues *issues)
	{
		_issues = issues;
	}

	void loadedModules(const map<string, VuoCompilerModule *> &modulesAdded,
					   const map<string, pair<VuoCompilerModule *, VuoCompilerModule *> > &modulesModified,
					   const map<string, VuoCompilerModule *> &modulesRemoved, VuoCompilerIssues *issues)
	{
		_issues->append(issues);

		loadedModulesCompleted();
	}
};

void printHelp(char *argv0)
{
	printf("Compiles a Vuo composition file into LLVM bitcode,\n"
		   "or compiles C/C++/Objective-C/Objective-C++/GLSL/ISF source code into a Vuo module (node class, type, or library).\n"
		   "\n"
		   "Usage:\n"
		   "  %s [options] composition.vuo      Compiles composition.vuo into LLVM bitcode (suitable for use with vuo-link).\n"
		   "  %s [options] node.class.name.c    Compiles node.class.name.c into a Vuo node class (suitable for placing in '~/Library/Application Support/Vuo/Modules').\n"
		   "\n"
		   "Options:\n"
		   "     --help                       Display this information.\n"
		   " -I, --header-search-path <dir>   Add the specified directory to the search path for include files. This option may be specified more than once. Only affects compiling .c files.\n"
		   "     --list-node-classes[=dot]    Display a list of all loaded node classes, optionally with the declaration of each as it would appear in a .vuo file.\n"
		   " -o, --output <file>              Place the compiled code into <file>.\n"
		   "     --target                     Target the given architecture, vendor, and operating system (e.g. 'x86_64-apple-macosx10.10.0').\n"
		   " -v, --verbose                    Output diagnostic information.\n",
		   argv0, argv0);
}

int main (int argc, char * const argv[])
{
	bool hasInputFile = false;
	string inputPath;
	VuoCompilerIssues *issues = new VuoCompilerIssues();
	int ret = 0;

	try
	{
		string outputPath = "";
		bool doListNodeClasses = false;
		string listNodeClassesOption = "";
		string target = "";
		vector<char *> headerSearchPaths;
		string dependencyOutputPath;
		bool shouldGenerateHeader = false;
		bool doPrintHelp = false;
		bool isVerbose = false;
		string optimization = "off";

		static struct option options[] = {
			{"help", no_argument, NULL, 0},
			{"output", required_argument, NULL, 0},
			{"list-node-classes", optional_argument, NULL, 0},
			{"target", required_argument, NULL, 0},
			{"header-search-path", required_argument, NULL, 0},
			{"verbose", no_argument, NULL, 0},
			{"optimization", required_argument, NULL, 0},
			{"generate-builtin-module-caches", required_argument, NULL, 0},
			{"generate-builtin-modules", required_argument, NULL, 0},
			{"generate-header-file", no_argument, NULL, 0},
			{"dependency-output", required_argument, NULL, 0},
			{NULL, no_argument, NULL, 0}
		};
		int optionIndex=-1;
		int ret;
		while ((ret = getopt_long(argc, argv, "o:I:vcE", options, &optionIndex)) != -1)
		{
			if (ret == '?')
				continue;

			switch(optionIndex)
			{
				case 0:  // --help
					doPrintHelp = true;
					break;
				case 1:	 // --output
					outputPath = optarg;
					break;
				case 2:  // --list-node-classes
					doListNodeClasses = true;
					if (optarg)
						listNodeClassesOption = optarg;
					break;
				case 3:  // --target
					target = optarg;
					break;
				case 4:  // --header-search-path
					headerSearchPaths.push_back(optarg);
					break;
				case 5:  // --verbose
					isVerbose = true;
					break;
				case 6:  // --optimization
					optimization = optarg;
					break;
				case 7:  // --generate-builtin-module-caches
					VuoCompiler::generateBuiltInModuleCache(optarg, target, false);
					return 0;
				case 8:  // --generate-builtin-modules
					VuoCompiler::generateBuiltInModuleCache(optarg, target, true);
					return 0;
				case 9:  // --generate-header-file
					shouldGenerateHeader = true;
					break;
				case 10:  // --dependency-output
					dependencyOutputPath = optarg;
					break;

				case -1:  // Short option
					switch (ret)
					{
						case 'o':
							outputPath = optarg;
							break;
						case 'I':
							headerSearchPaths.push_back(optarg);
							break;
						case 'v':
							isVerbose = true;
							break;
						case 'c':
							// Silently ignore `-c`, to support clang-style `vuo-compile -c foo.c -o foo.o` invocations,
							// enabling `ccache` to do its thing.
							break;
						case 'E':
						{
							// `ccache` first tries to invoke the precompiler (`-E`);
							// pass the node/type/library source along to clang.

							printf("#pragma vuo-compile %s %s\n", VuoCompilerHash, target.c_str());
							fflush(stdout);

							char **argvForClang = (char **)malloc(sizeof(char *) * (argc + 6));
							int inArgIndex = 1;
							int outArgIndex = 0;
							argvForClang[outArgIndex++] = (char *)"clang";
							while (inArgIndex < argc)
							{
								if (strcmp(argv[inArgIndex], "--target") == 0)
									++inArgIndex;  // also skip `--target`'s argument
								else
									argvForClang[outArgIndex++] = argv[inArgIndex];
								++inArgIndex;
							}
							argvForClang[outArgIndex++] = (char *)"-I" VUO_SOURCE_DIR "/node";
							argvForClang[outArgIndex++] = (char *)"-I" VUO_SOURCE_DIR "/library";
							argvForClang[outArgIndex++] = (char *)"-I" VUO_SOURCE_DIR "/type";
							argvForClang[outArgIndex++] = (char *)"-I" MACOS_SDK_ROOT "/usr/include";
							argvForClang[outArgIndex++] = (char *)"-F" MACOS_SDK_ROOT "/System/Library/Frameworks";
							argvForClang[outArgIndex++] = nullptr;
							execvp(LLVM_ROOT "/bin/clang", argvForClang);
							break;
						}
						default:
							VUserLog("Error: Unknown option '%c'.", ret);
							break;
					}
					break;

				default:
					VUserLog("Error: Unknown option %d.", optionIndex);
					break;
			}

			optionIndex = -1;
		}

		hasInputFile = (optind < argc) && ! doPrintHelp && ! doListNodeClasses;

		// Declared in this order so destructors are called in reverse order.
		// https://b33p.net/kosada/vuo/vuo/-/issues/15271
		VuoCompileCLIDelegate delegate(issues);
		VuoCompiler compiler("", target);

		compiler.setVerbose(isVerbose);
		compiler.setDelegate(&delegate);
		if (!dependencyOutputPath.empty())
			compiler.setDependencyOutput(dependencyOutputPath);

#ifdef USING_VUO_FRAMEWORK
		compiler.addHeaderSearchPath(VuoFileUtilities::getVuoFrameworkPath() + "/Headers");
#endif

		for (vector<char *>::iterator i = headerSearchPaths.begin(); i != headerSearchPaths.end(); ++i)
			compiler.addHeaderSearchPath(*i);

		if (doPrintHelp)
			printHelp(argv[0]);
		else if (doListNodeClasses)
		{
			if (listNodeClassesOption == "" || listNodeClassesOption == "dot")
				compiler.listNodeClasses(listNodeClassesOption);
			else
				throw VuoException("unrecognized option '" + listNodeClassesOption + "' for --list-node-classes");
		}
		else if (isVerbose && ! hasInputFile)
		{
			compiler.print();
		}
		else
		{
			if (! hasInputFile)
				throw VuoException("no input file");
			inputPath = argv[optind];

			if (optimization == "on")
				compiler.setLoadAllModules(false);

			compiler.setShouldPotentiallyShowSplashWindow(true);

			if (inputPath == "-")
			{
				if (outputPath.empty())
					outputPath = "a.bc";

				// Read STDIN into inputString.
				std::cin >> std::noskipws;
				std::istream_iterator<char> it(std::cin), end;
				std::string inputString(it, end);

				// Assume it's a .vuo composition (not .c/.cc/.m/.mm).
				compiler.compileCompositionString(inputString, outputPath, true, issues);
			}
			else
			{
				string inputDir, inputFile, inputExtension;
				VuoFileUtilities::splitPath(inputPath, inputDir, inputFile, inputExtension);

				if (outputPath.empty())
					outputPath = inputDir + inputFile + ".bc";

				if (inputExtension == "vuo")
				{
					string compositionString = VuoFileUtilities::readFileToString(inputPath);
					auto composition         = VuoCompilerComposition::newCompositionFromGraphvizDeclaration(compositionString, &compiler);

#ifdef USING_VUO_FRAMEWORK
					for (auto protocol : VuoProtocol::getCompositionProtocols(compositionString))
					{
						auto driver = VuoCompilerDriver::driverForProtocol(&compiler, protocol->getId());
						driver->applyToComposition(composition, &compiler, false);
						delete driver;
					}
#endif

					compiler.setCompositionPath(inputPath);
					compiler.compileComposition(composition, outputPath, true, issues);

					VuoComposition *baseComposition = composition->getBase();
					delete composition;
					delete baseComposition;
				}
				else if (VuoFileUtilities::isCFamilySourceExtension(inputExtension) || inputExtension == "fs")
				{
					if (shouldGenerateHeader)
						compiler.generateHeaderForModule(inputPath, outputPath);
					else
						compiler.compileModule(inputPath, outputPath);
				}
				else
					throw VuoException("input file must have a file extension of .vuo, .c, .cc, .m, .mm, or .fs");
			}
		}
	}
	catch (VuoCompilerException &e)
	{
		if (issues != e.getIssues())
			issues->append(e.getIssues());

		ret = 1;
	}
	catch (VuoException &e)
	{
		fprintf(stderr, "%s: error: %s\n", hasInputFile ? (inputPath == "-" ? "stdin" : inputPath.c_str()) : argv[0], e.what());
		if (!hasInputFile)
		{
			fprintf(stderr, "\n");
			printHelp(argv[0]);
		}
		fprintf(stderr, "\n");

		ret = 1;
	}

	if (! issues->isEmpty())
	{
		if (hasInputFile && inputPath == "-")
			issues->setFilePathIfEmpty("stdin");
		fprintf(stderr, "%s\n", issues->getLongDescription(false).c_str());
	}

	delete issues;

	return ret;
}

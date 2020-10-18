/**
 * @file
 * vuo-compile implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <getopt.h>
#ifdef USING_VUO_FRAMEWORK
	#include <Vuo/Vuo.h>
#else
	#include "VuoCompiler.hh"
	#include "VuoCompilerException.hh"
	#include "VuoCompilerIssue.hh"
	#include "VuoException.hh"
	#include "VuoFileUtilities.hh"
	#include "VuoModuleCompiler.hh"
#endif

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
		   "or compiles C/C++/Objective-C/Objective-C++/GLSL/ISF source code into a Vuo node.\n"
		   "\n"
		   "Usage:\n"
		   "  %s [options] composition.vuo      Compiles composition.vuo into LLVM bitcode (suitable for use with vuo-link).\n"
		   "  %s [options] node.class.name.c    Compiles a node class into a Vuo node (suitable for placing in '~/Library/Application Support/Vuo/Modules').\n"
		   "\n"
		   "Options:\n"
		   "     --help                       Display this information.\n"
		   " -I, --header-search-path <dir>   Add the specified directory to the search path for include files. This option may be specified more than once. Only affects compiling .c files.\n"
		   "     --list-node-classes[=dot]    Display a list of all loaded node classes, optionally with the declaration of each as it would appear in a .vuo file.\n"
		   " -o, --output <file>              Place the compiled code into <file>.\n"
//		   "     --target                     Target the given architecture, vendor, and operating system (e.g. 'x86_64-apple-macosx10.10.0').\n"
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
			{NULL, no_argument, NULL, 0}
		};
		int optionIndex=-1;
		int ret;
		while ((ret = getopt_long(argc, argv, "o:I:v", options, &optionIndex)) != -1)
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
					VuoCompiler::generateBuiltInModuleCaches(optarg);
					return 0;

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

		VuoCompileCLIDelegate delegate(issues);  // Declared in this order so destructors are called in reverse order.
		VuoCompiler compiler;                    // https://b33p.net/kosada/node/15271
		compiler.setVerbose(isVerbose);
		compiler.setDelegate(&delegate);

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

				/// @todo https://b33p.net/kosada/node/12220
//				if (! target.empty())
//					compiler.setTarget(target);

				if (inputExtension == "vuo")
				{
					compiler.setCompositionPath(inputPath);
					compiler.compileComposition(inputPath, outputPath, true, issues);
				}
				else if (VuoFileUtilities::isCSourceExtension(inputExtension) || inputExtension == "fs")
					compiler.compileModule(inputPath, outputPath);
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

	vector<VuoCompilerIssue> issueList = issues->getList();
	for (vector<VuoCompilerIssue>::iterator i = issueList.begin(); i != issueList.end(); ++i)
		fprintf(stderr, "%s: %s: %s\n",
			!i->getFilePath().empty()
				? i->getFilePath().c_str()
				: (hasInputFile ? (inputPath == "-" ? "stdin" : inputPath.c_str()) : argv[0]),
			(*i).getIssueType() == VuoCompilerIssue::Error ? "error" : "warning",
			(*i).getShortDescription(false).c_str());

	delete issues;

	return ret;
}

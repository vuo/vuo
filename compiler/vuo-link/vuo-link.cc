/**
 * @file
 * vuo-link implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <getopt.h>
#include <Vuo/Vuo.h>

void printHelp(char *argv0)
{
	printf("Links a composition's LLVM bitcode (created by vuo-compile) into an executable or dynamic library.\n"
		   "\n"
		   "Usage: %s [options] composition.bc\n"
		   "\n"
		   "Options:\n"
		   "  --help                         Display this information.\n"
		   "  --list-node-classes[=dot]      Display a list of all loaded node classes, optionally with the declaration of each as it would appear in a .vuo file.\n"
		   "  --output <file>                Place the compiled code into <file>.\n"
		   "  --target <arg>                 Target the given architecture, vendor, and OS (e.g. 'x86_64-apple-macosx10.10.0').\n"
		   "  --format <arg>                 Output the given type of binary file. <arg> can be 'executable' or 'dylib'. The default is 'executable'.\n"
		   "  --library-search-path <dir>    Search for libraries in <dir>. This option may be specified more than once.\n"
		   "  --framework-search-path <dir>  Search for macOS frameworks in <dir>. This option may be specified more than once.\n"
		   "  --optimization <arg>           Whether to link to module cache dylibs (`module-caches`) or not (`no-module-caches`). The default is 'module-caches', which is usually faster.\n"
		   "  --verbose                      Output diagnostic information.\n",
		   argv0);
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
		string format = "executable";
		bool doPrintHelp = false;
		bool isVerbose = false;
		vector<char *> librarySearchPaths;
		vector<char *> frameworkSearchPaths;
		string optimizationOption;

		static struct option options[] = {
			{"help", no_argument, NULL, 0},
			{"output", required_argument, NULL, 0},
			{"list-node-classes", optional_argument, NULL, 0},
			{"target", required_argument, NULL, 0},
			{"format", required_argument, NULL, 0},
			{"library-search-path", required_argument, NULL, 0},
			{"framework-search-path", required_argument, NULL, 0},
			{"optimization", required_argument, NULL, 0},
			{"verbose", no_argument, NULL, 0},
			{NULL, no_argument, NULL, 0}
		};
		int optionIndex=-1;
		int ret;
		while ((ret = getopt_long(argc, argv, "", options, &optionIndex)) != -1)
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
				case 4:  // --format
					format = optarg;
					break;
				case 5:  // --library-search-path
					librarySearchPaths.push_back(optarg);
					break;
				case 6:  // --framework-search-path
					frameworkSearchPaths.push_back(optarg);
					break;
				case 7:  // --optimization
					optimizationOption = optarg;
					break;
				case 8:  // --verbose
					isVerbose = true;
					break;
				default:
					VUserLog("Error: Unknown option %d.", optionIndex);
					break;
			}
		}

		VuoCompiler compiler("", target);
		compiler.setVerbose(isVerbose);

		hasInputFile = (optind < argc) && ! doPrintHelp && ! doListNodeClasses;

		for (vector<char *>::iterator i = librarySearchPaths.begin(); i != librarySearchPaths.end(); ++i)
			compiler.addLibrarySearchPath(*i);

		for (vector<char *>::iterator i = frameworkSearchPaths.begin(); i != frameworkSearchPaths.end(); ++i)
			compiler.addFrameworkSearchPath(*i);

		if (doPrintHelp)
			printHelp(argv[0]);
		else if (doListNodeClasses)
		{
			if (listNodeClassesOption == "" || listNodeClassesOption == "path" || listNodeClassesOption == "dot")
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

			string inputDir, inputFile, inputExtension;
			VuoFileUtilities::splitPath(inputPath, inputDir, inputFile, inputExtension);

			if (outputPath.empty())
			{
				outputPath = inputDir + inputFile;
				if (format == "dylib")
					outputPath += ".dylib";
			}

			compiler.setCompositionPath(inputPath);

			VuoCompiler::Optimization optimization = VuoCompiler::Optimization_ModuleCaches;
			if (! optimizationOption.empty())
			{
				if (optimizationOption == "module-caches")
					optimization = VuoCompiler::Optimization_ModuleCaches;
				else if (optimizationOption == "existing-module-caches")
					optimization = VuoCompiler::Optimization_ExistingModuleCaches;
				else if (optimizationOption == "no-module-caches")
					optimization = VuoCompiler::Optimization_NoModuleCaches;
				else
					throw VuoException("unrecognized option '" + optimizationOption + "' for --optimization");
			}

			if (format == "executable")
				compiler.linkCompositionToCreateExecutable(inputPath, outputPath, optimization);
			else if (format == "dylib")
				compiler.linkCompositionToCreateDynamicLibrary(inputPath, outputPath, optimization);
			else
				throw VuoException("unrecognized option '" + format + "' for --format");
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
		fprintf(stderr, "%s: error: %s\n", hasInputFile ? inputPath.c_str() : argv[0], e.what());
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
		fprintf(stderr, "%s: %s: %s\n\n", hasInputFile ? inputPath.c_str() : argv[0],
		        (*i).getIssueType() == VuoCompilerIssue::Error ? "error" : "warning", (*i).getShortDescription(false).c_str());

	return ret;
}

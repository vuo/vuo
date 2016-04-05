/**
 * @file
 * vuo-link implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <getopt.h>
#include <Vuo/Vuo.h>

void printHelp(char *argv0)
{
	printf("Usage: %s [options] file\n"
		   "Options:\n"
		   "  --help                         Display this information.\n"
		   "  --list-node-classes[=<arg>]    Display a list of all loaded node classes. <arg> can be 'path' or 'dot'.\n"
		   "  --output <file>                Place the compiled code into <file>.\n"
		   "  --target                       Target the given architecture, vendor, and OS (e.g. 'x86_64-apple-macosx10.7.0').\n"
		   "  --library-search-path <dir>    Search for libraries in <dir>. This option may be specified more than once.\n"
		   "  --framework-search-path <dir>  Search for Mac OS X frameworks in <dir>. This option may be specified more than once.\n"
		   "  --optimization <arg>           Optimize for a faster build that links against a cache file ('fast-build') or a standalone binary that does not ('small-binary'). The default is 'fast-build'.\n"
		   "  --verbose                      Output diagnostic information.\n",
		   argv0);
}

int main (int argc, char * const argv[])
{
	bool hasInputFile = false;
	string inputPath;

	try
	{
		string outputPath = "";
		bool doListNodeClasses = false;
		string listNodeClassesOption = "";
		string target = "";
		bool doPrintHelp = false;
		bool isVerbose = false;
		vector<char *> librarySearchPaths;
		vector<char *> frameworkSearchPaths;
		string optimizationOption;
		VuoCompiler compiler;

		static struct option options[] = {
			{"help", no_argument, NULL, 0},
			{"output", required_argument, NULL, 0},
			{"list-node-classes", optional_argument, NULL, 0},
			{"target", required_argument, NULL, 0},
			{"library-search-path", required_argument, NULL, 0},
			{"framework-search-path", required_argument, NULL, 0},
			{"optimization", required_argument, NULL, 0},
			{"verbose", no_argument, NULL, 0},
			{NULL, no_argument, NULL, 0}
		};
		int optionIndex=-1;
		while((getopt_long(argc, argv, "", options, &optionIndex)) != -1)
		{
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
				case 4:  // --library-search-path
					librarySearchPaths.push_back(optarg);
					break;
				case 5:  // --framework-search-path
					frameworkSearchPaths.push_back(optarg);
					break;
				case 6:  // --optimization
					optimizationOption = optarg;
					break;
				case 7:  // --verbose
					isVerbose = true;
					compiler.setVerbose(true);
					break;
			}
		}

		hasInputFile = (optind < argc) && ! doPrintHelp && ! doListNodeClasses;

		for (vector<char *>::iterator i = librarySearchPaths.begin(); i != librarySearchPaths.end(); ++i)
			compiler.addLibrarySearchPath(*i);

		for (vector<char *>::iterator i = frameworkSearchPaths.begin(); i != frameworkSearchPaths.end(); ++i)
			compiler.addFrameworkSearchPath(*i);

		compiler.loadStoredLicense();

		if (doPrintHelp)
			printHelp(argv[0]);
		else if (doListNodeClasses)
		{
			if (listNodeClassesOption == "" || listNodeClassesOption == "path" || listNodeClassesOption == "dot")
				compiler.listNodeClasses(listNodeClassesOption);
			else
				throw std::runtime_error("unrecognized option '" + listNodeClassesOption + "' for --list-node-classes");
		}
		else if (isVerbose && ! hasInputFile)
		{
			compiler.print();
		}
		else
		{
			if (! hasInputFile)
				throw std::runtime_error("no input file");
			inputPath = argv[optind];

			string inputDir, inputFile, inputExtension;
			VuoFileUtilities::splitPath(inputPath, inputDir, inputFile, inputExtension);

			if (outputPath.empty())
				outputPath = inputDir + inputFile;

			if (! target.empty())
				compiler.setTarget(target);

			VuoCompiler::Optimization optimization = VuoCompiler::Optimization_FastBuild;
			if (! optimizationOption.empty())
			{
				if (optimizationOption == "fast-build")
					optimization = VuoCompiler::Optimization_FastBuild;
				else if (optimizationOption == "fast-build-existing-cache")
					optimization = VuoCompiler::Optimization_FastBuildExistingCache;
				else if (optimizationOption == "small-binary")
					optimization = VuoCompiler::Optimization_SmallBinary;
				else
					throw std::runtime_error("unrecognized option '" + optimizationOption + "' for --optimization");
			}

			compiler.linkCompositionToCreateExecutable(inputPath, outputPath, optimization);
		}

		return 0;
	}
	catch (std::exception &e)
	{
		fprintf(stderr, "%s: error: %s\n", hasInputFile ? inputPath.c_str() : argv[0], e.what());
		if (!hasInputFile)
		{
			fprintf(stderr, "\n");
			printHelp(argv[0]);
		}
		return 1;
	}
}

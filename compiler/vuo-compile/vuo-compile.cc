/**
 * @file
 * vuo-compile implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <getopt.h>
#ifdef USING_VUO_FRAMEWORK
	#include <Vuo/Vuo.h>
#else
	#include "VuoCompiler.hh"
	#include "VuoFileUtilities.hh"
#endif

#include <iostream>
#include <stdexcept>

void printHelp(char *argv0)
{
	printf("Usage: %s [options] file\n"
		   "Options:\n"
		   "  --help                       Display this information.\n"
		   "  --header-search-path <dir>   Add the specified directory to the search path for include files. This option may be specified more than once. Only affects compiling .c files.\n"
		   "  --list-node-classes[=<arg>]  Display a list of all loaded node classes. <arg> can be 'path' or 'dot'.\n"
		   "  --output <file>              Place the compiled code into <file>.\n"
		   "  --target                     Target the given architecture, vendor, and operating system (e.g. 'x86_64-apple-macosx10.7.0').\n"
		   "  --verbose                    Output diagnostic information.\n",
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
		vector<char *> headerSearchPaths;
		bool doPrintHelp = false;
		bool isVerbose = false;
		string optimization = "off";
		VuoCompiler compiler;
		bool showLicenseWarning = true;

		static struct option options[] = {
			{"help", no_argument, NULL, 0},
			{"output", required_argument, NULL, 0},
			{"list-node-classes", optional_argument, NULL, 0},
			{"target", required_argument, NULL, 0},
			{"header-search-path", required_argument, NULL, 0},
			{"verbose", no_argument, NULL, 0},
			{"optimization", required_argument, NULL, 0},
			{"omit-license-warning", no_argument, NULL, 0},
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
				case 4:  // --header-search-path
					headerSearchPaths.push_back(optarg);
					break;
				case 5:  // --verbose
					isVerbose = true;
					compiler.setVerbose(true);
					break;
				case 6:  // --optimization
					optimization = optarg;
					break;
				case 7:  // --omit-license-warning
					showLicenseWarning = false;
					break;
			}
		}

		hasInputFile = (optind < argc) && ! doPrintHelp && ! doListNodeClasses;

#ifdef USING_VUO_FRAMEWORK
		compiler.addHeaderSearchPath(VuoFileUtilities::getVuoFrameworkPath() + "/Headers");
#endif

		for (vector<char *>::iterator i = headerSearchPaths.begin(); i != headerSearchPaths.end(); ++i)
			compiler.addHeaderSearchPath(*i);

		compiler.loadStoredLicense(showLicenseWarning);

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

			if (optimization == "on")
				compiler.setLoadAllModules(false);

			if (inputPath == "-")
			{
				if (outputPath.empty())
					outputPath = "a.bc";

				// Read STDIN into inputString.
				std::cin >> std::noskipws;
				std::istream_iterator<char> it(std::cin), end;
				std::string inputString(it, end);

				// Assume it's a .vuo composition (not .c/.cc/.m/.mm).
				compiler.compileCompositionString(inputString, outputPath);
			}
			else
			{
				string inputDir, inputFile, inputExtension;
				VuoFileUtilities::splitPath(inputPath, inputDir, inputFile, inputExtension);

				if (outputPath.empty())
					outputPath = inputDir + inputFile + ".bc";

				if (! target.empty())
					compiler.setTarget(target);

				if (inputExtension == "vuo")
					compiler.compileComposition(inputPath, outputPath, true, false);
				else if (inputExtension == "c" || inputExtension == "cc" || inputExtension == "m" || inputExtension == "mm")
					compiler.compileModule(inputPath, outputPath);
				else
					throw std::runtime_error("input file must have a file extension of .vuo, .c, .cc, .m, or .mm");
			}
		}

		return 0;
	}
	catch (std::exception &e)
	{
		fprintf(stderr, "%s: error: %s\n", hasInputFile ? (inputPath == "-" ? "stdin" : inputPath.c_str()) : argv[0], e.what());
		if (!hasInputFile)
		{
			fprintf(stderr, "\n");
			printHelp(argv[0]);
		}
		return 1;
	}
}

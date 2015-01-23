/**
 * @file
 * vuo-compile implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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


int main (int argc, char * const argv[])
{
	string outputPath = "";
	bool doListNodeClasses = false;
	string listNodeClassesOption = "";
	string target = "";
	vector<char *> headerSearchPaths;
	bool doPrintHelp = false;
	bool isVerbose = false;
	string inputPath;
	VuoCompiler compiler;

	static struct option options[] = {
		{"help", no_argument, NULL, 0},
		{"output", required_argument, NULL, 0},
		{"list-node-classes", optional_argument, NULL, 0},
		{"target", required_argument, NULL, 0},
		{"header-search-path", required_argument, NULL, 0},
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
			case 4:  // --header-search-path
				headerSearchPaths.push_back(optarg);
				break;
			case 5:  // --verbose
				isVerbose = true;
				compiler.setVerbose(true);
				break;
		}
	}

	bool hasInputFile = (optind < argc);

#ifdef USING_VUO_FRAMEWORK
	compiler.addHeaderSearchPath(VuoCompiler::getVuoFrameworkPath().str() + "/Headers");
#endif

	for (vector<char *>::iterator i = headerSearchPaths.begin(); i != headerSearchPaths.end(); ++i)
		compiler.addHeaderSearchPath(*i);

	if (doPrintHelp)
	{
		printf("Usage: %s [options] file\n"
			   "Options:\n"
			   "  --help                       Display this information.\n"
			   "  --header-search-path <dir>   Add the specified directory to the search path for include files. This option may be specified more than once. Only affects compiling .c files.\n"
			   "  --list-node-classes[=<arg>]  Display a list of all loaded node classes. <arg> can be 'path' or 'dot'.\n"
			   "  --output <file>              Place the compiled code into <file>.\n"
			   "  --target                     Target the given architecture, vendor, and operating system (e.g. 'x86_64-apple-macosx10.6.0').\n"
			   "  --verbose                    Output diagnostic information.\n",
			   argv[0]);
	}
	else if (doListNodeClasses)
	{
		if (listNodeClassesOption == "" || listNodeClassesOption == "path" || listNodeClassesOption == "dot")
			compiler.listNodeClasses(listNodeClassesOption);
		else
		{
			fprintf(stderr, "%s: unrecognized option '%s' for --list-node-classes\n", argv[0], listNodeClassesOption.c_str());
			return 1;
		}
	}
	else if (isVerbose && ! hasInputFile)
	{
		compiler.print();
	}
	else
	{
		if (! hasInputFile)
		{
			fprintf(stderr, "%s: no input file\n", argv[0]);
			return 1;
		}
		inputPath = argv[optind];

		string inputDir, inputFile, inputExtension;
		VuoFileUtilities::splitPath(inputPath, inputDir, inputFile, inputExtension);

		if (outputPath.empty())
			outputPath = inputDir + inputFile + ".bc";

		if (! target.empty())
			compiler.setTarget(target);

		if (inputExtension == "vuo")
			compiler.compileComposition(inputPath, outputPath);
		else if (inputExtension == "c" || inputExtension == "cc")
			compiler.compileModule(inputPath, outputPath);
		else
		{
			fprintf(stderr, "%s: input file '%s' must have a file extension of .vuo or .c\n", argv[0], inputPath.c_str());
			return 1;
		}
	}

	return 0;
}

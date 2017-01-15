/**
 * @file
 * vuo-export implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <getopt.h>
#include <Vuo/Vuo.h>

#include "VuoRendererComposition.hh"

/**
 * Shows usage information.
 */
void printHelp(char *argv0)
{
	printf("Usage: %s command [options] composition-file.vuo\n"
		   "\n"
		   "Global options:\n"
		   "    --help                         Display this information.\n"
		   "    --output <file>                Place the exported composition into <file>.\n"
		   "    --list-node-classes[=<arg>]    Display a list of all loaded node classes. <arg> can be 'path' or 'dot'.\n"
		   "    --library-search-path <dir>    Search for libraries in <dir>. This option may be specified more than once.\n"
		   "    --framework-search-path <dir>  Search for Mac OS X frameworks in <dir>. This option may be specified more than once.\n"
		   "    --verbose                      Output diagnostic information.\n"
		   "\n"
		   "Commands:\n"
		   "\n"
		   "    macosx — Builds the composition as a standalone Mac OS App. Options:\n"
		   "        --target                   Target the given architecture, vendor, and OS (e.g. 'x86_64-apple-macosx10.7.0').\n"
		   "\n"
		   "    movie — Builds and runs the composition, saving the rendered output to a movie file. Options:\n"
		   "        --size=1024x768            Suggested size, in pixels.\n"
		   "        --time=0.0                 Initial rendering time (protocol input) in seconds.\n"
		   "        --duration=10.0            Movie length in seconds.\n"
		   "        --framerate=30             Frames per second.\n"
		   "        --spatial-supersample=4    Antialias by multiplying the suggested size input values by 4,\n"
		   "                                   then scaling the output image back down by 4.\n"
		   "        --temporal-supersample=4   Adds motion blur by rendering 4 frames to produce each output frame,\n"
		   "                                   and averaging them together.\n"
		   "        --shutter-angle=360        When using temporal supersampling, specifies the percentage of time\n"
		   "                                   the shutter is open (between 0 and 360).\n"
		   "        --image-format=h264        Movie image codec. Options:\n"
		   "                                       h264\n"
		   "                                       jpeg\n"
		   "                                       prores4444\n"
		   "                                       prores422\n"
		   "                                       (*) The ProRes codec is only available on systems with Final Cut Pro installed.\n"
		   "        --quality=1.0              Image compression quality, 0 (lowest) to 1 (highest).\n"
		   "        --port phase=0.2           Set published input port `phase` to value `0.2`. Use JSON.\n"
		   "                                   For VuoImage ports, you may specify a filename.\n",
		   argv0);
}

bool stop = false;	///< When this becomes true, movie exporting is cancelled.

/**
 * Responds to SIGINT (CTRL-C).
 */
void sigintHandler(int sigNum)
{
	stop = true;
	printf("\n");
	fflush(stdout);
}

int main (int argc, char * argv[])
{
	VuoRendererComposition::createAutoreleasePool();

	// https://bugreports.qt-project.org/browse/QTBUG-29197
	qputenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM", "1");

	// Tell Qt where to find its plugins.
	QApplication::setLibraryPaths(QStringList((VuoFileUtilities::getVuoFrameworkPath() + "/../resources/QtPlugins").c_str()));

	QApplication app(argc, argv);

	bool hasPotentialCommand = false;
	bool hasPotentialInputFile = false;
	string inputPath;

	VuoMovieExporter *exporter = NULL;

	try
	{
		string outputPath = "";
		bool doListNodeClasses = false;
		string listNodeClassesOption = "";
		string target = "";
		bool doPrintHelp = false;
		bool isVerbose = false;
		VuoMovieExporterParameters movieParameters;
		vector<char *> librarySearchPaths;
		vector<char *> frameworkSearchPaths;
		VuoCompiler compiler;
		compiler.loadStoredLicense(true);

		static struct option options[] = {
			{"help", no_argument, NULL, 0},
			{"output", required_argument, NULL, 0},
			{"list-node-classes", optional_argument, NULL, 0},
			{"target", required_argument, NULL, 0},
			{"library-search-path", required_argument, NULL, 0},
			{"framework-search-path", required_argument, NULL, 0},
			{"verbose", no_argument, NULL, 0},
			{"size", required_argument, NULL, 0},
			{"time", required_argument, NULL, 0},
			{"duration", required_argument, NULL, 0},
			{"framerate", required_argument, NULL, 0},
			{"spatial-supersample", required_argument, NULL, 0},
			{"temporal-supersample", required_argument, NULL, 0},
			{"image-format", required_argument, NULL, 0},
			{"quality", required_argument, NULL, 0},
			{"port", required_argument, NULL, 0},
			{"shutter-angle", required_argument, NULL, 0},
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
				case 6:  // --verbose
					isVerbose = true;
					compiler.setVerbose(true);
					break;
				case 7:  // --size
				{
					movieParameters.width = atoi(optarg);
					char *x = strchr(optarg, 'x');
					if (x)
						movieParameters.height = atoi(x + 1);
					break;
				}
				case 8:  // --time
					movieParameters.time = atof(optarg);
					break;
				case 9:  // --duration
					movieParameters.duration = atof(optarg);
					break;
				case 10:  // --framerate
					movieParameters.framerate = atof(optarg);
					break;
				case 11:  // --spatial-supersample
					movieParameters.spatialSupersample = atoi(optarg);
					break;
				case 12:  // --temporal-supersample
					movieParameters.temporalSupersample = atoi(optarg);
					break;
				case 13:  // --image-format
					if (strcmp(optarg, "h264") == 0)
						movieParameters.imageFormat = VuoMovieExporterParameters::H264;
					else if (strcmp(optarg, "jpeg") == 0)
						movieParameters.imageFormat = VuoMovieExporterParameters::JPEG;
					else if (strcmp(optarg, "prores4444") == 0)
					{
						if (!VuoMovieExporterParameters::isProResAvailable())
							throw std::runtime_error("The ProRes codec is only available on systems with Final Cut Pro installed.");

						movieParameters.imageFormat = VuoMovieExporterParameters::ProRes4444;
					}
					else if (strcmp(optarg, "prores422") == 0)
					{
						if (!VuoMovieExporterParameters::isProResAvailable())
							throw std::runtime_error("The ProRes codec is only available on systems with Final Cut Pro installed.");

						movieParameters.imageFormat = VuoMovieExporterParameters::ProRes422;
					}
					else
						throw std::runtime_error("unrecognized image format");
					break;
				case 14:  // --quality
					movieParameters.quality = atof(optarg);
					break;
				case 15:  // --port
				{
					string o(optarg);
					size_t equals = o.find('=');
					if (equals == string::npos)
						throw std::runtime_error("'--port' should contain an equals sign");
					else
					{
						string portName = o.substr(0, equals);
						string portValue = o.substr(equals + 1);

						// First, try parsing the JSON string as-is.
						json_object *js = json_tokener_parse(portValue.c_str());

						if (!js)
							// If that doesn't work, treat it as a string literal.
							js = json_object_new_string(portValue.c_str());

						movieParameters.inputs[portName] = js;
					}
					break;
				}
				case 16:  // --shutter-angle
					movieParameters.shutterAngle = atof(optarg);
					break;
			}
		}

		hasPotentialCommand   = (optind < argc--) && ! doPrintHelp && ! doListNodeClasses;
		hasPotentialInputFile = (optind < argc)   && ! doPrintHelp && ! doListNodeClasses;

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
				throw std::runtime_error("unrecognized option '" + listNodeClassesOption + "' for --list-node-classes");
		}
		else
		{
			if (! hasPotentialCommand)
				throw std::runtime_error("no command");

			string command = argv[optind];
			string inputDir, inputFile, inputExtension;
			if (hasPotentialInputFile)
			{
				inputPath = argv[optind + 1];
				VuoFileUtilities::splitPath(inputPath, inputDir, inputFile, inputExtension);
			}

			if (command == "macosx")
			{
				if (! hasPotentialInputFile)
					throw std::runtime_error("no input file");

				if (! VuoFileUtilities::fileContainsReadableData(inputPath))
					throw std::runtime_error("can't read input file");

				if (outputPath.empty())
					outputPath = inputDir + inputFile + ".app";

				if (! target.empty())
					compiler.setTarget(target);

				VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(inputPath, &compiler);
				VuoComposition *baseComposition = new VuoComposition();
				VuoCompilerComposition *compilerComposition = new VuoCompilerComposition(baseComposition, parser);
				compilerComposition->getBase()->setDirectory(inputDir);
				VuoRendererComposition *rendererComposition = new VuoRendererComposition(baseComposition);

				string exportErrString;
				rendererComposition->exportApp(outputPath.c_str(), &compiler, exportErrString);

				delete parser;
				delete compilerComposition;
				delete rendererComposition;
				delete baseComposition;
			}
			else if (command == "movie")
			{
				if (! hasPotentialInputFile)
					throw std::runtime_error("no input file");

				if (! VuoFileUtilities::fileContainsReadableData(inputPath))
					throw std::runtime_error("can't read input file");

				// If the user presses Ctrl-C, try to cleanly finish the movie.
				signal(SIGINT, sigintHandler);

				if (outputPath.empty())
					outputPath = inputDir + inputFile + ".mov";

				if (movieParameters.width < 1 || movieParameters.height < 1)
					throw std::runtime_error("width and height must be > 0");

				if (movieParameters.duration < 0.00001)
					throw std::runtime_error("duration must be > 0");

				if (movieParameters.framerate < 0.00001)
					throw std::runtime_error("framerate must be > 0");

				if (movieParameters.spatialSupersample < 1)
					throw std::runtime_error("spatial-supersample must be >= 1");

				if (movieParameters.temporalSupersample < 1)
					throw std::runtime_error("temporal-supersample must be >= 1");

				if (movieParameters.quality < 0)
					throw std::runtime_error("quality must be >= 0");

				if (isVerbose)
				{
					printf("Exporting to %s\n", outputPath.c_str());
					movieParameters.print();
					printf("\n");
				}

				if (isVerbose)
					QTextStream(stdout) << QString("Compiling…");

				exporter = new VuoMovieExporter(inputPath, outputPath, movieParameters);

				if (isVerbose)
					QTextStream(stdout) << QString("\r");

				unsigned int framesExportedSoFar = 0;
				unsigned int totalFrameCount = exporter->getTotalFrameCount();
				int frameNumberWidth = log10(totalFrameCount) + 1;
				bool moreFramesToExport = true;
				while (moreFramesToExport && !stop)
				{
					if (isVerbose)
						QTextStream(stdout) << QString("Exporting frame %1 of %2 (%3% done)\r")
											   .arg(framesExportedSoFar + 1, frameNumberWidth)
											   .arg(totalFrameCount, frameNumberWidth)
											   // floor() so that the next-to-last frames don't get rounded up to 100%;
											   // it would be confusing to see "100%" when it's not done yet.
											   .arg(floor(100.*framesExportedSoFar/totalFrameCount), 3);

					moreFramesToExport = exporter->exportNextFrame();
					++framesExportedSoFar;
				};
				delete exporter;

				if (isVerbose)
					QTextStream(stdout) << QString("Exported  frame %1 of %2 (%3% done)\n")
										   .arg(framesExportedSoFar, frameNumberWidth)
										   .arg(totalFrameCount, frameNumberWidth)
										   .arg(floor(100.*framesExportedSoFar/totalFrameCount), 3);
			}
			else
				throw std::runtime_error("unknown command");
		}

		return 0;
	}
	catch (std::exception &e)
	{
		fprintf(stderr, "\n%s: error: %s\n", hasPotentialInputFile ? inputPath.c_str() : argv[0], e.what());
		if (!hasPotentialInputFile)
		{
			fprintf(stderr, "\n");
			printHelp(argv[0]);
		}

		if (exporter)
		{
			try
			{
				delete exporter;
			}
			catch (std::exception &e)
			{
				fprintf(stderr, "\n%s: error: %s\n", hasPotentialInputFile ? inputPath.c_str() : argv[0], e.what());
			}
		}

		return 1;
	}
}

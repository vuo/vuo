/**
 * @file
 * Vuo Editor main() implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <getopt.h>

#define VUO_PCH_QT
#include "../../vuo.pch"

#include "VuoEditor.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorUtilities.hh"
#include "VuoEditorWindow.hh"
#include "VuoFileUtilities.hh"
#include "VuoHeap.h"
#include "VuoProtocol.hh"
#include "VuoRendererCommon.hh"
#include "VuoUrl.h"

int main(int argc, char *argv[])
{
	qInstallMessageHandler(VuoRendererCommon::messageHandler);

	Q_INIT_RESOURCE(VuoEditorApp);

	bool doPrintHelp = false;
	bool doOpenRecent = false;
	bool doOpenRandom = false;
	QString doOpenTemplate;
	bool doRun = false;
	bool doQuit = false;

	int getoptArgC = 0;
	char ** getoptArgV = (char **)malloc(sizeof(char *) * argc);
	for(int i=0;i<argc;++i)
	{
		// Don't pass the OS X Process Serial Number argument to getopt, since it can't handle long arguments with a single hyphen.
		if (strncmp(argv[i], "-psn_", 5) == 0)
			continue;

		// Don't pass single-hyphen (Qt-style) arguments to getopt, since it outputs warnings.
		if (strlen(argv[i]) >= 2
		 && argv[i][0] == '-'
		 && argv[i][1] != '-')
			continue;

		getoptArgV[getoptArgC++] = argv[i];
	}

	static struct option options[] = {
		{"help", no_argument, NULL, 0},
		{"draw-bounding-rects", no_argument, NULL, 0},
		{"generate-docs", required_argument, NULL, 0},
		{"open-recent", no_argument, NULL, 0},
		{"open-random", no_argument, NULL, 0},
		{"run", no_argument, NULL, 0},
		{"quit", no_argument, NULL, 0},
		{"template", required_argument, NULL, 0},
		{NULL, no_argument, NULL, 0}
	};
	int optionIndex=-1;
	int ret;
	while ((ret = getopt_long(getoptArgC, getoptArgV, "", options, &optionIndex)) != -1)
	{
		if (ret == '?')
			continue;

		switch(optionIndex)
		{
			case 0:  // --help
				doPrintHelp = true;
				break;
			case 1:	 // --draw-bounding-rects
				VuoRendererItem::setDrawBoundingRects(true);
				break;
			case 2:	 // --generate-docs
				VuoEditor::setDocumentationGenerationDirectory(optarg);
				break;
			case 3:  // --open-recent
				doOpenRecent = true;
				break;
			case 4:  // --open-random
				doOpenRandom = true;
				break;
			case 5:  // --run
				doRun = true;
				break;
			case 6:  // --quit
				doQuit = true;
				break;
			case 7:  // --template
				doOpenTemplate = optarg;
				break;
		}
	}

	if (doPrintHelp)
	{
		free(getoptArgV);
		printf("Usage: %s [options] [composition file(s)]\n"
			   "Options:\n"
			   "  --help                       Display this information.\n"
			   "  --draw-bounding-rects        Draws red bounding rectangles around items in the composition canvas.  Useful for debugging canvas interaction.\n"
			   "  --generate-docs <dir>        Saves the full collection of node class and node set documentation in <dir>, then exits.\n"
			   "  --open-recent                Reopens the most recent composition.\n"
			   "  --open-random                Opens a random example composition.\n"
			   "  --template <template>        Opens a new composition with the specified template.\n"
			   "                               E.g., \"--template VuoImageTransition\".\n"
			   "                               See Vuo.app/Contents/Frameworks/Vuo.framework/Resources/*.vuo for a full list.\n"
			   "  --run                        Starts all open compositions running.\n"
			   "  --quit                       Exits after optionally opening and running compositions.  Useful for testing performance.\n",
			   argv[0]);
		return 0;
	}
	else
	{
		// Tell Qt where to find its plugins.
		QApplication::setLibraryPaths(QStringList((VuoFileUtilities::getVuoFrameworkPath() + "/../QtPlugins").c_str()));

		// Qt's default cache is just 10 MB;
		// caching pixmaps of 100 nodes (say, 200 * 150 * 4 pixels per Retina point * 4 bytes per pixel) uses 48 MB
		// (and that doesn't even count cables, which have large bounding boxes).
		QPixmapCache::setCacheLimit(512 * 1024);

		VuoEditor v(argc,argv);

		if (doOpenRecent)
			v.openMostRecentFile();

		if (doOpenRandom)
			QTimer::singleShot(0, &v, &VuoEditor::openRandomExample);

		if (!doOpenTemplate.isEmpty())
		{
			QAction *templateAction = new QAction(nullptr);

			VuoProtocol *protocol = VuoProtocol::getProtocol(doOpenTemplate.toStdString());
			if (protocol)
			{
				templateAction->setData(qVariantFromValue(static_cast<void *>(protocol)));
				QObject::connect(templateAction, &QAction::triggered, &v, &VuoEditor::newCompositionWithProtocol);
			}
			else
			{
				templateAction->setData(doOpenTemplate);
				QObject::connect(templateAction, &QAction::triggered, &v, &VuoEditor::newCompositionWithTemplate);
			}

			QTimer::singleShot(0, templateAction, &QAction::trigger);
		}

		// If there are any arguments left over after getopt_long(), try to open them.
		for (int i = optind; i < getoptArgC; ++i)
		{
			VuoUrl u = VuoUrl_normalize(getoptArgV[i], VuoUrlNormalize_default);
			VuoLocal(u);
			v.openUrl(u);
		}

		free(getoptArgV);

		if (doRun)
			QTimer::singleShot(0, ^{
				foreach (VuoEditorWindow *window, VuoEditorUtilities::getOpenCompositionEditingWindows())
				{
					window->on_runComposition_triggered();
					// Wait for the composition to launch.
					window->getComposition()->isRunning();
				}
			});

		if (doQuit)
		{
			QTimer::singleShot(0, &v, &QApplication::closeAllWindows);
			QTimer::singleShot(0, &v, &QApplication::quit);
		}

		return v.exec();
	}
}

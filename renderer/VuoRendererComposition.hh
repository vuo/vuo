/**
 * @file
 * VuoRendererComposition interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUORENDERERCOMPOSITION_HH
#define VUORENDERERCOMPOSITION_HH

#include "VuoBaseDetail.hh"
#include "VuoCable.hh"
#include "VuoComposition.hh"
#include "VuoRendererPublishedPort.hh"

class VuoCompiler;
class VuoCompilerNode;
class VuoRendererNode;
class VuoRendererTypecastPort;
class VuoRendererCable;
class VuoCompilerGraphvizParser;
class VuoRendererSignaler;

/**
 * Provides a canvas upon which nodes and cables can be rendered.
 */
class VuoRendererComposition : public QGraphicsScene, public VuoBaseDetail<VuoComposition>
{
public:
	VuoRendererComposition(VuoComposition *baseComposition, bool renderMissingAsPresent = false, bool enableCaching = false);

	void setBackgroundTransparent(bool transparent);
	VuoRendererNode * createRendererNode(VuoNode *baseNode);
	void addNode(VuoNode *node);
	void addCable(VuoCable *cable);
	void addPublishedInputCable(VuoCable *c);
	void addPublishedOutputCable(VuoCable *c);
	void removeNode(VuoRendererNode *rn);
	void removeCable(VuoRendererCable *rc);
	void removePublishedInputCable(VuoRendererCable *rc);
	void removePublishedOutputCable(VuoRendererCable *rc);
	VuoRendererPublishedPort * publishPort(VuoPort *port, string name="", bool attemptMerge=false);
	VuoCable * createPublishedCable(VuoPort *vuoPseudoPort, VuoPort *internalPort);
	void registerExternalPublishedPort(VuoPublishedPort *publishedPort, bool isInput);
	int unregisterExternalPublishedPort(VuoPublishedPort *publishedPort, bool isInput);
	void setPublishedPortName(VuoRendererPublishedPort *publishedPort, string name);
	string getUniquePublishedPortName(string baseName, bool isInput);
	vector<VuoRendererNode *> collapseTypecastNodes(void);
	VuoRendererTypecastPort * collapseTypecastNode(VuoRendererNode *rn);
	void uncollapseTypecastNode(VuoRendererNode *typecastNode);
	VuoRendererNode * uncollapseTypecastNode(VuoRendererTypecastPort *typecast);
	void clearInternalPortEligibilityHighlighting(void);
	bool getRenderActivity(void);

	static void createAutoreleasePool(void);

	/// Potential outcomes of an app export attempt:
	enum appExportResult
	{
		exportSuccess,
		exportBuildFailure,
		exportSaveFailure
	};

	string takeSnapshot(void);
	VuoRendererComposition::appExportResult exportApp(const QString &savePath, VuoCompiler *compiler, string &errString);

protected:
	void setRenderActivity(bool render);
	void repaintAllComponents();

	VuoCompilerGraphvizParser *parser; ///< The Graphviz parser instance used by this composition.
	map<string, VuoRendererNode *> nodeNameTaken; ///< An index of every node in the composition, keyed by the node's Graphviz identifier.
	VuoRendererSignaler *signaler; ///< The Qt signaler used by this composition.
	bool cachingEnabled; ///< Should item renderings be cached?

private:
	VuoRendererPublishedPort * createRendererForPublishedPortInComposition(VuoPublishedPort *publishedPort);
	void addNodeInCompositionToCanvas(VuoNode *n);
	void addCableInCompositionToCanvas(VuoCable *c);
	int getIndexOfPublishedPort(VuoPublishedPort *port, bool isInput);
	void updatePublishedInputNode();
	void updatePublishedOutputNode();
	bool isPublishedPortNameTaken(string name, bool isInput);

	string createAppBundleDirectoryStructure();
	bool bundleExecutable(VuoCompiler *compiler, string targetExecutablePath, string &errString);
	void bundleVuoFrameworkFolder(string sourceVuoFrameworkPath, string targetVuoFrameworkPath, string onlyCopyExtension="");
	void bundleVuoSubframeworks(string sourceVuoFrameworkPath, string targetVuoFrameworkPath);

	bool renderMissingAsPresent; ///< Should node classes without implementations be rendered as though their implementations are present?
	bool renderActivity; ///< Should renderings reflect recent component activity (e.g., node executions, event firings)?
};

#endif // VUORENDERERCOMPOSITION_HH

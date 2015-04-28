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
	void createAndConnectDrawersToListInputPorts(VuoRendererNode *node, VuoCompiler *compiler);
	VuoRendererNode * createAndConnectMakeListNode(VuoNode *toNode, VuoPort *toPort, VuoCompiler *compiler, VuoRendererCable *&rendererCable);
	void addPublishedPort(VuoPublishedPort *publishedPort, bool isInput);
	int removePublishedPort(VuoPublishedPort *publishedPort, bool isInput);
	void setPublishedPortName(VuoRendererPublishedPort *publishedPort, string name);
	string getUniquePublishedPortName(string baseName, bool isInput);
	vector<VuoRendererNode *> collapseTypecastNodes(void);
	VuoRendererTypecastPort * collapseTypecastNode(VuoRendererNode *rn);
	void uncollapseTypecastNode(VuoRendererNode *typecastNode);
	VuoRendererNode * uncollapseTypecastNode(VuoRendererTypecastPort *typecast);
	void clearInternalPortEligibilityHighlighting(void);
	bool getRenderActivity(void);
	QGraphicsItem::CacheMode getCurrentDefaultCacheMode();

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
	void updateComponentCaching();
	void repaintAllComponents();
	VuoRendererPublishedPort * createRendererForPublishedPortInComposition(VuoPublishedPort *publishedPort);

	VuoCompilerGraphvizParser *parser; ///< The Graphviz parser instance used by this composition.
	VuoRendererSignaler *signaler; ///< The Qt signaler used by this composition.
	bool cachingEnabled; ///< Should item renderings be cached?

private:
	void addNodeInCompositionToCanvas(VuoNode *n);
	void addCableInCompositionToCanvas(VuoCable *c);
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

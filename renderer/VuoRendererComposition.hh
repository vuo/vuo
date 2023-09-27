/**
 * @file
 * VuoRendererComposition interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoBaseDetail.hh"
#include "VuoCable.hh"
#include "VuoRendererPublishedPort.hh"

class VuoCompiler;
class VuoCompilerDriver;
class VuoCompilerNode;
class VuoComposition;
class VuoComment;
class VuoRendererComment;
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

	void addComponentsInCompositionToCanvas();
	void setBackgroundTransparent(bool transparent);
	VuoRendererNode * createRendererNode(VuoNode *baseNode);
	VuoRendererComment * createRendererComment(VuoComment *baseComment);
	void addNode(VuoNode *node, bool nodeShouldBeRendered=true, bool nodeShouldBeGivenUniqueIdentifier=true);
	void addCable(VuoCable *cable);
	void addComment(VuoComment *comment);
	void removeNode(VuoRendererNode *rn);
	void removeCable(VuoRendererCable *rc);
	void removeComment(VuoRendererComment *rcomment);
	QList<QGraphicsItem *> createAndConnectInputAttachments(VuoRendererNode *node, VuoCompiler *compiler, bool createButDoNotAdd=false);
	VuoRendererNode * createAndConnectMakeListNode(VuoNode *toNode, VuoPort *toPort, VuoCompiler *compiler, VuoRendererCable *&rendererCable);
	void createAndConnectDictionaryAttachmentsForNode(VuoNode *node, VuoCompiler *compiler, set<VuoRendererNode *> &createdNodes, set<VuoRendererCable *> &createdCables);
	vector<string> extractInputVariableListFromExpressionsConstant(string constant, string nodeClassName);
	void addPublishedPort(VuoPublishedPort *publishedPort, bool isPublishedInput, VuoCompiler *compiler);
	int removePublishedPort(VuoPublishedPort *publishedPort, bool isPublishedInput, VuoCompiler *compiler);
	VuoRendererPublishedPort * createRendererForPublishedPortInComposition(VuoPublishedPort *publishedPort, bool isPublishedInput);
	void setPublishedPortName(VuoRendererPublishedPort *publishedPort, string name, VuoCompiler *compiler);
	string getUniquePublishedPortName(string baseName);
	vector<VuoRendererNode *> collapseTypecastNodes(void);
	VuoRendererTypecastPort * collapseTypecastNode(VuoRendererNode *rn);
	void uncollapseTypecastNodes();
	void uncollapseTypecastNode(VuoRendererNode *typecastNode);
	VuoRendererNode * uncollapseTypecastNode(VuoRendererTypecastPort *typecast);
	void clearInternalPortEligibilityHighlighting(void);
	VuoNode * getPublishedInputNode(void);
	VuoNode * getPublishedOutputNode(void);
	bool getRenderNodeActivity(void);
	bool getRenderPortActivity(void);
	bool getRenderHiddenCables(void);
	void setRenderHiddenCables(bool render);
	QGraphicsItem::CacheMode getCurrentDefaultCacheMode();
	map<VuoPort *, string> getPortConstantResourcePathsRelativeToDir(QDir newDir);
	string getAppIconResourcePathRelativeToDir(QDir newDir);
	void bundleResourceFiles(string targetResourceDir, bool tmpFilesOnly=false, QString bundledIconPath="");
	void modifyAllResourcePathsForBundle(void);
	static bool isTmpFile(string filePath);

	static void setGridOpacity(int opacity);
	static int getGridOpacity();

	/// What kind of decorative grid to render on the canvas background.
	enum GridType
	{
		NoGrid,
		LineGrid,
		PointGrid
	};
	static void setGridType(GridType type);
	static QPoint quantizeToNearestGridLine(QPointF point, int gridSpacing);
	static void createAutoreleasePool(void);

	// Drawing configuration
	static const int majorGridLineSpacing; ///< Distance, in pixels at 1:1 zoom, between major gridlines.
	static const int minorGridLineSpacing; ///< Distance, in pixels at 1:1 zoom, between minor gridlines.

	static const string deprecatedDefaultDescription; ///< The default description assigned to compositions prior to Vuo 2.0.

protected:
	void drawBackground(QPainter *painter, const QRectF &rect);
	void setRenderActivity(bool render, bool includePortActivity=true);
	void setComponentCaching(QGraphicsItem::CacheMode);
	void updateGeometryForAllComponents();
	bool isPortPublished(VuoRendererPort *port);
	static bool isDirectory(string path);

	VuoCompilerGraphvizParser *parser; ///< The Graphviz parser instance used by this composition.
	VuoRendererSignaler *signaler; ///< The Qt signaler used by this composition.
	VuoNode *publishedInputNode; ///< The published input node associated with this composition.
	VuoNode *publishedOutputNode; ///< The published output node associated with this composition.
	bool cachingEnabled; ///< Should item renderings be cached?
	bool renderHiddenCables; ///< Should cables be rendered even if they have been hidden (made "wireless")?

private:
	void addNodeInCompositionToCanvas(VuoNode *n);
	void addCableInCompositionToCanvas(VuoCable *c);
	void addCommentInCompositionToCanvas(VuoComment *c);
	QList<QGraphicsItem *> createAndConnectDrawersToListInputPorts(VuoRendererNode *node, VuoCompiler *compiler, bool createButDoNotAdd=false);
	QList<QGraphicsItem *> createAndConnectDrawersToReadOnlyDictionaryInputPorts(VuoRendererNode *node, VuoCompiler *compiler, bool createButDoNotAdd=false);
	VuoNode * createPublishedInputNode();
	VuoNode * createPublishedOutputNode();

	void bundleAuxiliaryFilesForSceneFile(QString sourceFilePath, QString targetFilePath);
	string modifyResourcePathForBundle(string path);
	string modifyResourcePathForNewDir(string path, QDir newDir);

	static int gridOpacity; ///< The opacity at which grid lines/points should be rendered on the canvas.
	static GridType gridType; ///< The type of grid to render.
	bool renderMissingAsPresent; ///< Should node classes without implementations be rendered as though their implementations are present?
	bool renderNodeActivity; ///< Should renderings reflect recent node activity (e.g., node executions)?
	bool renderPortActivity; ///< Should renderings reflect recent port activity (e.g., trigger port firings)?
};

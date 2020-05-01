/**
 * @file
 * VuoRendererPort interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoRendererItem.hh"

#include "VuoBaseDetail.hh"
#include "VuoType.hh"
#include "VuoPortClass.hh"
#include "VuoRendererInputDrawer.hh"
#include "VuoRendererColors.hh"

class VuoCompilerNodeClass;
class VuoCompilerNode;
class VuoRendererNode;
class VuoRendererInputListDrawer;
class VuoCable;
class VuoPort;
class VuoRendererPublishedPort;
class VuoRendererSignaler;

/**
 * Renders a node's port in a @c QGraphicsScene.  Typically automatically created by a @c VuoRendererNode instance.
 */
class VuoRendererPort : public VuoRendererItem, public VuoBaseDetail<VuoPort>
{
public:
	VuoRendererPort(VuoPort *basePort, VuoRendererSignaler *signaler,
					bool isOutput, bool isRefreshPort, bool isFunctionPort);
	~VuoRendererPort();

	QRectF boundingRect(void) const;
	QRectF getNameRect(void) const;
	bool hasPortAction(void) const;
	QRectF getActionIndicatorRect(void) const;
	QPainterPath shape(void) const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

	bool getEligibleForSelection(void);
	bool isEligibleForConnection(void);
	void setEligibilityHighlight(VuoRendererColors::HighlightType eligibility);
	VuoRendererColors::HighlightType eligibilityHighlight(void);
	void extendedHoverEnterEvent(bool cableDragUnderway=false, bool disableConnectedCableHighlight=false);
	void extendedHoverMoveEvent(bool cableDragUnderway=false, bool disableConnectedCableHighlight=false);
	void extendedHoverLeaveEvent();
	bool canConnectDirectlyWithoutSpecializationTo(VuoRendererPort *toPort, bool eventOnlyConnection);
	bool canConnectDirectlyWithSpecializationTo(VuoRendererPort *toPort, bool eventOnlyConnection);
	bool canConnectDirectlyWithSpecializationTo(VuoRendererPort *toPort, bool eventOnlyConnection, VuoRendererPort **portToSpecialize, string &specializedTypeName);
	VuoCable * getCableConnectedTo(VuoRendererPort *toPort, bool includePublishedCables);
	bool getInput(void) const;
	bool getOutput(void) const;
	bool getRefreshPort(void) const;
	bool getFunctionPort(void) const;
	void updateNameRect(void);
	void updateGeometry();
	QVariant itemChange(GraphicsItemChange change, const QVariant &value);
	QPainterPath getPortPath() const;
	static QRectF getPortRect(void);
	VuoRendererInputDrawer * getAttachedInputDrawer(void) const;
	virtual QRectF getPortConstantTextRect(void) const;
	static int getTextWidth(QString text);
	VuoType * getDataType(void) const;
	bool hasURLType(void) const;
	bool hasRelativeReadURLConstantValue(void) const;
	bool isConstant(void) const;
	bool effectivelyHasConnectedDataCable(bool includePublishedCables) const;
	static string format(const char *format, ...);
	static string stringAndFree(char *strz);
	string getConstantAsString(void) const;
	string getConstantAsTruncatedStringToRender(void) const;
	string getConstantAsStringToRender(void) const;
	string getStringForRealValue(double value) const;
	string getStringForRealValue(float value) const;
	void setConstant(string constantValue);
	string getPortNameToRender() const;
	string getPortNameToRenderWhenDisplayed() const;
	static QString getPortIdentifierRegExp();
	static QString sanitizePortIdentifier(QString portID);
	void setPortNameToRender(string name);
	bool getPublishable() const;
	vector<VuoRendererPublishedPort *> getPublishedPorts() const;
	vector<VuoRendererPublishedPort *> getPublishedPortsConnectedByDataCarryingCables(void) const;
	VuoNode::TintColor getPortTint() const;

	VuoRendererNode * getUnderlyingParentNode(void) const;
	VuoRendererNode * getRenderedParentNode(void) const;
	set<VuoRendererInputAttachment *> getAllUnderlyingUpstreamInputAttachments(void) const;
	set<VuoRendererPort *> getPortsConnectedWirelessly(bool includePublishedCables) const;

	VuoRendererPort * getTypecastParentPort() const;
	void setTypecastParentPort(VuoRendererPort *typecastParentPort);
	bool supportsDisconnectionByDragging(void);

	void resetTimeLastEventFired();
	void setFiredEvent();
	void setFadePercentageSinceEventFired(qreal percentage);
	void setCacheModeForPortAndChildren(QGraphicsItem::CacheMode mode);

	vector<QGraphicsItemAnimation *> getAnimations();
	void setAnimated(bool animated);

	bool isHiddenRefreshPort(void);

	bool carriesData(void) const;

	// Drawing configuration
	static const qreal portRadius; ///< Radius, in pixels at 1:1 zoom, of a circular port.
	static const qreal portSpacing; ///< Vertical distance, in pixels at 1:1 zoom, between the center points of two ports.
	static const qreal portContainerMargin;	///< Vertical distance, in pixels at 1:1 zoom, between the outer edge of the first/last port and the node frame rect.
	static const qreal portInset; ///< The vertical and horizontal inset used when rendering a circular or refresh port shape within its outer port rect.
	static const qreal portBarrierWidth; ///< The stroke width of event barriers.
	static const qreal portConstantTextPadding; ///< Empty space left and right of the constant text, inside the constant flag.

protected:
	void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
	void keyPressEvent(QKeyEvent *event);

private:

	QFont getPortNameFont(void) const;
	static void addRoundedTriangle(QPainterPath &p, QPointF center, qreal radius, qreal cornerRadius);

	// Port attributes affecting drawing
	bool isOutput;
	bool isFunctionPort;
	VuoRendererColors::HighlightType _eligibilityHighlight;
	bool isEligibleForSelection;
	bool isAnimated;
	VuoRendererPort *typecastParentPort;
	qint64 timeLastEventFired;
	vector<QGraphicsItemAnimation *> animations;

	friend class VuoRendererPublishedPort; 	///< VuoRendererPublishedPort needs paint(...) and boundingRect()
	friend class TestVuoRenderer;

protected:
	bool isRefreshPort; ///< Is this port a refresh port?

	QRectF cachedBoundingRect; ///< Bounding rect calculation is slow, and Qt calls it often, so cache it.
	void updateCachedBoundingRect();

	QRectF nameRect; ///< The bounding box of the port's label when rendered on the canvas.
	string customizedPortName; ///< The name of the port as it should be rendered.

	bool portNameRenderingEnabled(void) const;
	void updateEnabledStatus();
	VuoRendererInputDrawer * getAttachedInputDrawerRenderedWithHostPort(const VuoRendererPort *port) const;
	VuoRendererInputAttachment * getUnderlyingInputAttachment(void) const;

	QPainterPath cachedPortPath; ///< Path calculation is slow, and we call it often, so cache it.
	void updateCachedPortPath();

	static QRectF getPortConstantTextRectForText(QString text);
	static QPainterPath getPortPath(qreal inset, VuoPortClass::PortType portType, QString constantText, bool isInputPort, bool carriesData);
	QRectF getEventBarrierRect(void) const;
	virtual QPainterPath getWirelessAntennaPath() const;
	bool hasConnectedWirelessDataCable(bool includePublishedCables) const;
	bool hasConnectedWirelessEventCable(bool includePublishedCables) const;
	VuoNode::TintColor getWirelessAntennaTint() const;
	qint64 getTimeOfLastActivity();

	void paintPortName(QPainter *painter, VuoRendererColors *colors);
	void paintEventBarrier(QPainter *painter, VuoRendererColors *colors);
	void paintActionIndicator(QPainter *painter, VuoRendererColors *colors);
	void paintWirelessAntenna(QPainter *painter, VuoRendererColors *colors);
	string getDefaultPortNameToRender();
	string getPointStringForCoords(QList<float>) const;

	VuoRendererSignaler *signaler; ///< The Qt signaler used by this port.
};


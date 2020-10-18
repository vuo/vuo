/**
 * @file
 * VuoPortPopover implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoPortPopover.hh"
#include "VuoCompilerComposition.hh"
#include "VuoCompilerGraph.hh"
#include "VuoCompilerInputDataClass.hh"
#include "VuoCompilerInputEventPortClass.hh"
#include "VuoCompilerTriggerPort.hh"
#include "VuoRendererFonts.hh"
#include "VuoNodeClass.hh"
#include "VuoEditor.hh"
#include "VuoEditorComposition.hh"
#include "VuoHeap.h"
#include "VuoPopover.hh"
#include "VuoComposition.hh"


#ifdef __APPLE__
#include <objc/objc-runtime.h>
#include <objc/runtime.h>
#endif

const int VuoPortPopover::maxPopoverContentWidth = 512; ///< The maximum width of popover content (not including margins), in points.
const int VuoPortPopover::maxPopoverImageWidth   = 256; ///< The maximum width of an image inside a popover, in points.
const int VuoPortPopover::maxPopoverImageHeight  = 256; ///< The maximum height  of an image inside a popover, in points.
const qreal VuoPortPopover::minTextUpdateInterval = 100; // The minimum time (in ms) between updates to the popover text, including automatic refreshes.
const int VuoPortPopover::eventHistoryMaxSize = 20; // The maximum number of recent events to be used in calculating the event frequency.
const int VuoPortPopover::noEventObserved = -1;  // Indicates that we're watching for events, but haven't seen any yet.
const int VuoPortPopover::noDisplayableEventTime = -2; // Indicates that we're not watching for or displaying event information.
const string VuoPortPopover::noDataValueObserved = ""; // Indicates that we're watching for data values, but haven't seen any yet.
const string VuoPortPopover::noDisplayableDataValue = "Unknown"; // Indicates that we're not watching for or displaying data values.

/**
 * Creates a new popover widget to display information about a port.
 */
VuoPortPopover::VuoPortPopover(VuoPort *port, VuoEditorComposition *composition, QWidget *parent) :
	QTextBrowser(parent)
{
	// Text content
	this->portID = composition->getIdentifierForStaticPort(port);
	this->composition = composition;
	this->cachedDataValue = noDisplayableDataValue;
	this->timeOfLastEvent = noDisplayableEventTime;
	this->timeOfLastUpdate = noDisplayableEventTime;
	this->eventCount = 0;
	this->droppedEventCount = 0;
	this->isDetached = false;

	setReadOnly(true);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);  // not Qt::TextBrowserInteraction since it makes text selectable
	setOpenExternalLinks(true);

	imageResize = VuoImageResize_make();
	VuoRetain(imageResize);

	this->popoverTextQueue = dispatch_queue_create("org.vuo.editor.port", NULL);

	this->refreshTextTimer = new QTimer(this);
	this->refreshTextTimer->setObjectName("VuoPortPopover::refreshTextTimer");
	refreshTextTimerFiredSinceLastReset = false;
	connect(refreshTextTimer, &QTimer::timeout, this, &VuoPortPopover::updateTextAndResize);

	setCompositionRunning(false, false);

	if (port->getClass()->getPortType() == VuoPortClass::triggerPort)
	{
		VuoCompilerTriggerPort *triggerPort = static_cast<VuoCompilerTriggerPort *>(port->getCompiler());
		size_t allPortsReached = composition->getBase()->getCompiler()->getCachedGraph()->getPublishedOutputPortsDownstream(triggerPort).size();
		size_t triggerPortsReached = composition->getBase()->getCompiler()->getCachedGraph()->getPublishedOutputTriggersDownstream(triggerPort).size();
		allEventsBlocked = (triggerPortsReached == 0 && allPortsReached > 0);
		someEventsBlocked = (triggerPortsReached > 0 && allPortsReached > triggerPortsReached);

		if (allEventsBlocked || someEventsBlocked)
		{
			QToolButton *helpButton = new QToolButton(this);
			helpButton->setIcon(QIcon(":/Icons/question-circle.svg"));
			helpButton->setStyleSheet("QToolButton { border: none; }");
			helpButton->setCursor(Qt::PointingHandCursor);
			connect(helpButton, &QToolButton::clicked, this, &VuoPortPopover::helpButtonClicked);

			// Place the button in the bottom-right corner of the popover.
			QVBoxLayout *innerLayout = new QVBoxLayout(this);
			innerLayout->setContentsMargins(0, 0, 4, 4);
			innerLayout->addWidget(helpButton, 0, Qt::AlignBottom | Qt::AlignRight);
			this->setLayout(innerLayout);
		}
	}
	else
		allEventsBlocked = someEventsBlocked = false;

	// Style
	VuoEditor *editor = (VuoEditor *)qApp;
	connect(editor, &VuoEditor::darkInterfaceToggled, this, &VuoPortPopover::updateStyle);
	connect(editor, &VuoEditor::darkInterfaceToggled, this, &VuoPortPopover::updateTextAndResize);
	updateStyle();
	updateTextAndResize();

	this->dragInProgress = false;
}

/**
 * Destroys a popover.
 */
VuoPortPopover::~VuoPortPopover()
{
	VuoRelease(imageResize);
	dispatch_release(popoverTextQueue);
}

/**
 * Displays the new data value in the popover immediately, without waiting for the popover
 * to refresh on its regular schedule.
 *
 * @param value A string representation of the port's data value.
 */
void VuoPortPopover::updateDataValueImmediately(QString value)
{
	dispatch_sync(popoverTextQueue, ^{
					  mostRecentImage.clear();
					  this->cachedDataValue = value.toUtf8().constData();
					  this->updateTextThreadUnsafe();
					  resetRefreshTextInterval();
				  });
}

/**
 * Displays the new event time (if any) and data value (if any) the next time the popover's
 * contents are refreshed.
 *
 * If this function is called again before then, only the event time and data value from the
 * later call are displayed.
 *
 * @param event True if the popover's last event time should be updated.
 * @param data True if the popover's data value should be updated.
 * @param value A string representation of the port's data value.
 */
void VuoPortPopover::updateLastEventTimeAndDataValue(bool event, bool data, QString value)
{
	dispatch_sync(popoverTextQueue, ^{
					  qint64 timeBefore = this->timeOfLastUpdate;
					  qint64 timeNow = QDateTime::currentMSecsSinceEpoch();
					  this->timeOfLastUpdate = timeNow;

					  if (event)
					  {
						  this->timeOfLastEvent = timeNow;
						  this->eventHistory.enqueue(timeNow);
						  while (this->eventHistory.size() > eventHistoryMaxSize)
							  this->eventHistory.dequeue();

						  ++eventCount;
					  }

					  if (data)
					  {
						  this->cachedDataValue = value.toUtf8().constData();
					  }

					  if (timeBefore + minTextUpdateInterval < timeNow)
					  {
						  this->updateTextThreadUnsafe(true);
						  resetRefreshTextInterval();
					  }
				  });
}

/**
 * Increments the number of dropped events observed for the port (a trigger port).
 */
void VuoPortPopover::incrementDroppedEventCount()
{
	dispatch_sync(popoverTextQueue, ^{
					  ++droppedEventCount;

					  this->updateTextThreadUnsafe(true);
					  resetRefreshTextInterval();
				  });
}

/**
 * Updates the popover to include or exclude information relevant only if
 * the composition is currently running.
 */
void VuoPortPopover::setCompositionRunning(bool running, bool resetDataValue)
{
	dispatch_sync(popoverTextQueue, ^{
					  this->compositionRunning = running;

					  if (running)
					  {
						  if (resetDataValue)
							this->cachedDataValue = noDataValueObserved;

						  this->timeOfLastEvent = noEventObserved;
						  this->timeOfLastUpdate = noEventObserved;

						  while (!eventHistory.isEmpty())
							eventHistory.dequeue();

						  this->eventCount = 0;
						  this->droppedEventCount = 0;

						  // Refresh the popover text periodically to keep the reported time
						  // since the last event up-to-date.
						  refreshTextTimer->setInterval(minTextUpdateInterval);
						  refreshTextTimer->start();
					  }

					  else
					  {
						  refreshTextTimer->stop();
					  }

					  // @todo https://b33p.net/kosada/node/6755 : Possibly need to re-size to accommodate longer text.
					  updateTextThreadUnsafe();
				  });
}

/**
 * Updates the popover text and expands the popover to accommodate the
 * updated text if necessary.
 *
 * @threadNoQueue{popoverTextQueue}
 */
void VuoPortPopover::updateTextAndResize()
{
	dispatch_sync(popoverTextQueue, ^{
					  updateTextAndResizeThreadUnsafe();
					  refreshTextTimerFiredSinceLastReset = true;
				  });
}

/**
 * If this is an image port, retrieves the image from the running composition,
 * and returns the HTML to display it.
 */
QString VuoPortPopover::generateImageCode()
{
	VuoPort *port = composition->getPortWithStaticIdentifier(portID);
	VuoType *type = static_cast<VuoCompilerPortClass *>(port->getClass()->getCompiler())->getDataVuoType();
	if (!type || (type->getModuleKey() != "VuoImage" && type->getModuleKey() != "VuoVideoFrame"))
		return "";

	if (! compositionRunning)
		return mostRecentImage;

	json_object *imageJson = composition->getPortValueInRunningComposition(port);
	if (!imageJson)
		return mostRecentImage;

	VuoImage image = NULL;
	json_object *o;
	if (json_object_object_get_ex(imageJson, "ioSurface", &o))   // VuoImage_getInterprocessJson returns an object with an 'ioSurface' key.
		image = VuoImage_makeFromJson(imageJson);
	else if (json_object_object_get_ex(imageJson, "image", &o))  // VuoVideoFrame_getInterprocessJson returns an object with an 'image' key, which contains an 'ioSurface' key.
		image = VuoImage_makeFromJson(o);
	else
		VUserLog("Warning: Unknown interprocess JSON format.");
	json_object_put(imageJson);
	if (!image)
		return mostRecentImage;

	VuoRetain(image);

	int maxImageWidth = maxPopoverImageWidth;
	int maxImageHeight = maxPopoverImageHeight;

	int devicePixelRatio = window()->windowHandle()->screen()->devicePixelRatio();
	maxImageWidth  *= devicePixelRatio;
	maxImageHeight *= devicePixelRatio;

	if (image->pixelsWide > maxImageWidth
	 || image->pixelsHigh > maxImageHeight)
	{
		VuoImage resizedImage = VuoImageResize_resize(image, imageResize, VuoSizingMode_Proportional, maxImageWidth, maxImageHeight);
		if (resizedImage)
		{
			VuoRetain(resizedImage);
			VuoRelease(image);
			image = resizedImage;
		}
	}

	const unsigned char *buffer = VuoImage_getBuffer(image, GL_RGBA);
	unsigned long int bufferLength = image->pixelsWide * image->pixelsHigh * 4;
	unsigned char *bufferClamped = (unsigned char *)malloc(bufferLength);

	// The VuoImage might contain out-of-range premultiplied color values.
	// Clamp them, since QImage might wrap (especially when blending onto
	// the white part of the checkerboard background), which looks bad.
	for (unsigned long int i = 0; i < bufferLength; i += 4)
	{
		unsigned char a = buffer[i+3];
		bufferClamped[i  ] = MIN(a,buffer[i  ]);
		bufferClamped[i+1] = MIN(a,buffer[i+1]);
		bufferClamped[i+2] = MIN(a,buffer[i+2]);
		bufferClamped[i+3] = a;
	}

	QImage qi(bufferClamped, image->pixelsWide, image->pixelsHigh, QImage::Format_RGBA8888_Premultiplied, free, bufferClamped);
	VuoRelease(image);
	qi.setDevicePixelRatio(devicePixelRatio);
	qi = qi.mirrored(false, true);

	document()->addResource(QTextDocument::ImageResource, QUrl("vuo-port-popover://foreground.png"), QVariant(qi));
	mostRecentImage = "<div><img style='background-image: url(vuo-port-popover://background.png);' src='vuo-port-popover://foreground.png' /></div>";
	return mostRecentImage;
}

/**
 * Updates the popover text and expands the popover to accommodate the
 * updated text if necessary.
 *
 * @threadQueue{popoverTextQueue}
 */
void VuoPortPopover::updateTextAndResizeThreadUnsafe()
{
	// Exponentially increase the timeout interval of the popover
	// refresh timer as time passes since the most recent event or data.
	double secondsSinceLastUpdate = (QDateTime::currentMSecsSinceEpoch() - this->timeOfLastUpdate)/1000.;
	int updatedTextRefreshInterval = (secondsSinceLastUpdate <= 1? 0.1 :
					(secondsSinceLastUpdate < 20?   1 :
					(secondsSinceLastUpdate < 40?   2 :
					(secondsSinceLastUpdate < 120?  6 :
					(secondsSinceLastUpdate < 180?  9 :
					(secondsSinceLastUpdate < 600? 30 :
												  INT_MAX/1000))))))*1000;

	if (updatedTextRefreshInterval != refreshTextTimer->interval())
		this->refreshTextTimer->setInterval(updatedTextRefreshInterval);

	setHtml(generatePortPopoverText());

	// Calculate the size needed to display the text without wrapping,
	// limited to about half the width of a typical screen.
	document()->setPageSize(QSizeF(maxPopoverContentWidth, 10000));
	document()->setPageSize(QSizeF(ceil(document()->idealWidth()), 10000));
	QSize documentSize = document()->documentLayout()->documentSize().toSize();

	// Never shrink the popover.
	documentSize = documentSize.expandedTo(size());

	this->setFixedSize(documentSize);
}

/**
 * Updates the popover text.
 *
 * @threadQueue{popoverTextQueue}
 */
void VuoPortPopover::updateTextThreadUnsafe(bool includeEventIndicator)
{
   this->setText(generatePortPopoverText(includeEventIndicator));
}

/**
 * Resets the regular interval at which to refresh the popover text, making
 * sure not to reset it before it has had a chance to refresh since the
 * last reset.
 */
void VuoPortPopover::resetRefreshTextInterval()
{
	if (refreshTextTimerFiredSinceLastReset)
	{
		refreshTextTimerFiredSinceLastReset = false;
		this->refreshTextTimer->setInterval(minTextUpdateInterval);
	}

	QThread::yieldCurrentThread();
}

/**
 * Generates a descriptive text string containing information about the popover's associated port,
 * making use of its most up-to-date stored attribute values.
 *
 * If @c includeEventIndicator is true, the generated string will include a dot to indicate that
 * the port associated with this popover has just received, transmitted, or fired an event.
 */
QString VuoPortPopover::generatePortPopoverText(bool includeEventIndicator)
{
	VuoEditor *editor = (VuoEditor *)qApp;
	VuoPort *port = composition->getPortWithStaticIdentifier(portID);
	if (!port)
		return QString();

	bool isDark = editor->isInterfaceDark();
	QString textColor        = isDark ? "#cacaca" : "#000000";
	QString subtleTextColor  = isDark ? "#808080" : "#808080";
	QString subtlerTextColor = isDark ? "#505050" : "#bbbbbb";
	QString codeTextColor    = isDark ? "#383838" : "#ececec";

		qint64 timeOfLastEventSnapshot = this->timeOfLastEvent;
		QQueue<qint64> eventHistorySnapshot = this->eventHistory;
		string cachedDataValueSnapshot = this->cachedDataValue;
		bool compositionRunningSnapshot = this->compositionRunning;

		string portName = port->getRenderer()->getPortNameToRenderWhenDisplayed();
		VuoNode *parentNode = port->getRenderer()->getUnderlyingParentNode()->getBase();
		string nodeName = parentNode->getTitle();
		string nodeClass = parentNode->getNodeClass()->getClassName();
		VuoType *dataType = port->getRenderer()->getDataType();
		QString dataTypeDescription = composition->formatTypeNameForDisplay(dataType);
		bool displayValue = (dataType && (cachedDataValueSnapshot != noDisplayableDataValue));

		//: Appears in port popovers.
		//: Refers to whether any events passed through this port in a running composition while the popover was open.
		const string noEvent = ("(" + tr("none observed") + ")").toStdString();

		//: Appears in port popovers.
		//: Refers to how many events passed through this port in a running composition while the popover was open.
		const string unknownFrequency = "(" + tr("%1 observed", "", eventCount).arg(eventCount).toStdString() + ")";

		QString lastEventTime;
		string lastEventFrequency = "";

		bool displayLastEventTime = compositionRunningSnapshot;
		bool displayEventFrequency = (timeOfLastEventSnapshot != noDisplayableEventTime);

		// Time since last event
		if (displayLastEventTime && (timeOfLastEventSnapshot != noEventObserved))
		{
			qint64 timeNow = QDateTime::currentMSecsSinceEpoch();
			double secondsSinceLastEvent = (timeNow - timeOfLastEventSnapshot)/1000.;
			int roundedSecondsSinceLastEvent = (int)(secondsSinceLastEvent + 0.5);

			lastEventTime = secondsSinceLastEvent <= 1
				? tr("just now")
				: (secondsSinceLastEvent < 20
					? tr("%1 second(s) ago", "", roundedSecondsSinceLastEvent).arg(roundedSecondsSinceLastEvent)
					: (secondsSinceLastEvent < 40
						? tr("about half a minute ago")
						: (secondsSinceLastEvent < 120
							? tr("about a minute ago")
							: (secondsSinceLastEvent < 180
								? tr("a couple minutes ago")
								: (secondsSinceLastEvent < 600
									? tr("several minutes ago")
									: tr("more than 10 minutes ago"))))));
		}

		// Event frequency
		if (displayEventFrequency)
		{
			if (timeOfLastEventSnapshot == noEventObserved)
				lastEventFrequency = noEvent;

			else if (eventHistorySnapshot.size() > 2)
			{
				double recentEventIntervalMean = getEventIntervalMean(eventHistorySnapshot);
				double recentEventIntervalStdDev = getEventIntervalStdDev(eventHistorySnapshot);

				// Calculate the coefficient of variation (CV) in time intervals between events.
				// Don't display frequencies for events whose intervals are fluctuating wildly.
				double recentEventIntervalCV = recentEventIntervalStdDev/recentEventIntervalMean;
				const double maxCVForFrequencyDisplay = 2.0;
				if (recentEventIntervalCV <= maxCVForFrequencyDisplay)
				{
					double recentEventFrequency = 1./recentEventIntervalMean;

					// Use an appropriate time unit.
					//: Appears in port popovers.
					QString unit = tr("per second");
					double roundedEventFrequency = ((int)(10*recentEventFrequency+0.5))/10.;

					if (roundedEventFrequency == 0)
					{
						//: Appears in port popovers.
						unit = tr("per minute");
						roundedEventFrequency = ((int)(10*60*recentEventFrequency+0.5))/10.;
					}

					if (roundedEventFrequency == 0)
					{
						//: Appears in port popovers.
						unit = tr("per hour");
						roundedEventFrequency = ((int)(10*60*60*recentEventFrequency+0.5))/10.;
					}

					lastEventFrequency += "(~";
					lastEventFrequency += QLocale::system().toString(roundedEventFrequency, 'f', 1).toStdString();
					lastEventFrequency += " ";
					lastEventFrequency += unit.toStdString();
					lastEventFrequency += ")";
				}

				else
					lastEventFrequency = unknownFrequency;
			}

			else
				lastEventFrequency = unknownFrequency;
		}

		// Only indicate unknown frequencies after the composition has stopped.
		if ((lastEventFrequency == unknownFrequency) && compositionRunningSnapshot)
			lastEventFrequency = "";

		if (!lastEventTime.isEmpty() && !lastEventFrequency.empty())
			lastEventFrequency = " " + lastEventFrequency;

		if (includeEventIndicator && compositionRunningSnapshot)
			lastEventFrequency += " ·";
		else
			// Leave room for the event indicator when it's hidden,
			// so the popover doesn't have to resize when it appears.
			lastEventFrequency += " &nbsp;";

		// Event throttling
		QString eventThrottlingDescription;
		if (port->getClass()->getPortType() == VuoPortClass::triggerPort)
		{
			if (port->getEventThrottling() == VuoPortClass::EventThrottling_Enqueue)
				//: Appears in port popovers.
				eventThrottlingDescription = tr("enqueue events");
			else if (! compositionRunningSnapshot)
				//: Appears in port popovers.
				eventThrottlingDescription = tr("drop events");
			else
			{
				unsigned int totalEventCount = droppedEventCount + eventCount;
				unsigned int percentDropped = round( (float)droppedEventCount / (float)totalEventCount * 100 );
				//: Appears in port popovers.
				//: Refers to the number and percentage of events dropped in the running composition while the port popover was open.
				//: Example: "42 events dropped (3%)"
				eventThrottlingDescription = tr("%1 event(s) dropped (%2%)", "", droppedEventCount).arg(droppedEventCount).arg(percentDropped);
			}
		}

		// Help when trigger's event stream overlaps published input's event stream in subcomposition
		QString formattedTriggerBlockedHelp;
		if (allEventsBlocked || someEventsBlocked)
			formattedTriggerBlockedHelp = "<p>"
				+ (allEventsBlocked
					? tr("Events from this trigger are blocked from exiting this composition.")
					: tr("Some events from this trigger are blocked from exiting this composition."))
				// Leave room for questionmark-circle.svg.
				+ "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</p>";

		// Formatting
		QString formattedPortName = (isDetached?
										 QString("<b><font size=+2 color='%3'>%1: %2</font></b>").arg(nodeName.c_str(), portName.c_str(), textColor) :
										 QString("<b><font size=+2 color='%2'>%1</font></b>").arg(portName.c_str(), textColor));
		QString formattedDataTypeDescription = "<tr><th>" + tr("Data type") + ": </th><td>" + dataTypeDescription +"</td></tr>";
		QString formattedThrottlingDescription = "<tr><th>" + tr("Event throttling") + ": </th><td>" + eventThrottlingDescription + "</td></tr>";
		QString formattedLastEventLine = "<tr><th>" + tr("Last event") + ": </th>"
			+ "<td" + (lastEventFrequency == noEvent ? " class='subtler'" : "") + ">"
			+ lastEventTime + QString::fromStdString(lastEventFrequency) + "</td></tr>";

		// Special formatting for named enum types
		{
			json_object *details = NULL;
			VuoCompilerInputEventPortClass *portClass = dynamic_cast<VuoCompilerInputEventPortClass *>(port->getClass()->getCompiler());
			if (portClass)
				details = (portClass->getDataClass()? portClass->getDataClass()->getDetails() : NULL);
			json_object *menuItemsValue = NULL;
			if (details && dataType && dataType->getModuleKey() == "VuoInteger" && json_object_object_get_ex(details, "menuItems", &menuItemsValue))
			{
				int len = json_object_array_length(menuItemsValue);
				for (int i = 0; i < len; ++i)
				{
					json_object *menuItem = json_object_array_get_idx(menuItemsValue, i);
					if (json_object_is_type(menuItem, json_type_object))
					{
						json_object *value = NULL;
						if (json_object_object_get_ex(menuItem, "value", &value))
							if (json_object_is_type(value, json_type_int   ) && atol(cachedDataValueSnapshot.c_str()) == json_object_get_int64(value))
							{
								json_object *name = NULL;
								if (json_object_object_get_ex(menuItem, "name", &name))
								{
									cachedDataValueSnapshot += ": ";
									cachedDataValueSnapshot += json_object_get_string(name);
									break;
								}
							}
					}
				}
			}
		}

		//: Appears in port popovers.
		//: Refers to the port's current value.
		QString formattedDataValue = "<tr><th>" + tr("Value")
			+ ": </th><td>" + generateImageCode() + QString::fromStdString(cachedDataValueSnapshot) + "</td></tr>";

		QString popoverText = VUO_QSTRINGIFY(
					<style>
					code {
						font-family: 'Monaco';
						font-size: 12px;
						background-color: %1;
						white-space: pre-wrap;
					}
					table {
						font-size: 13px;
					}
					table th {
						font-weight: normal;
						text-align: right;
						color: %2;
						white-space: pre; // Don't wrap table headers.
					}
					table td {
						font-weight: bold;
						color: %2;
					}
					.subtler {
						color: %3;
					}
					p {
						font-size: 13px;
						font-weight: normal;
						color: %2;
					}
					</style>)
				.arg(codeTextColor)
				.arg(subtleTextColor)
				.arg(subtlerTextColor);
		popoverText.append(formattedPortName);
		popoverText.append("<table>");
		popoverText.append(formattedDataTypeDescription);

		if (port->getClass()->getPortType() == VuoPortClass::triggerPort)
			popoverText.append(formattedThrottlingDescription);

		if ((displayLastEventTime || displayEventFrequency) && (!lastEventTime.isEmpty() || !lastEventFrequency.empty()))
			popoverText.append(formattedLastEventLine);

		if (displayValue)
			popoverText.append(formattedDataValue);

		popoverText.append("</table>");

		popoverText.append(formattedTriggerBlockedHelp);

		return popoverText;
}

/**
 * Returns a boolean indicating whether the popover is detached from its port.
 */
bool VuoPortPopover::getDetached()
{
	return isDetached;
}

/**
 * Detaches the popover from its port.
 */
void VuoPortPopover::detach()
{
	this->isDetached = true;

	// The following will no longer be necessary once the popover is never
	// assigned a parent to begin with; see https://b33p.net/node/5211 .
	if (parentWidget())
	{
		QPoint newGlobalPos = parentWidget()->mapToGlobal(pos());
		setParent(NULL);
		move(newGlobalPos);
	}

	updateStyle();
	updateTextAndResize();
	show();

	emit popoverDetachedFromPort(portID);
}


/**
 * Updates the port popover's style as appropriate for its attached or detached status.
 */
void VuoPortPopover::updateStyle()
{
	setAlignment(Qt::AlignTop);
	setFont(VuoRendererFonts::getSharedFonts()->portPopoverFont());
	setAutoFillBackground(true);
	document()->setDocumentMargin(5);

	VuoEditor *editor = (VuoEditor *)qApp;
	bool isDark = editor->isInterfaceDark();
	QString borderColor     = isDark ? "#505050" : "#d1d1d1";
	QString backgroundColor = isDark ? "#282828" : "#f9f9f9";

	setStyleSheet("border: none;");

	if (! isDetached)
	{
		Qt::WindowFlags flags = windowFlags();
		flags |= Qt::FramelessWindowHint;
		setWindowFlags(flags);


		// Disabled until https://bugreports.qt-project.org/browse/QTBUG-40687 or https://b33p.net/kosada/node/5211 .
//		QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
//		shadow->setBlurRadius(10);
//		shadow->setColor(QColor(160, 160, 160, 128));
//		shadow->setOffset(2);
//		this->setGraphicsEffect(shadow);

		setAttribute(Qt::WA_TranslucentBackground);
		setAutoFillBackground(false);

		viewport()->setStyleSheet(QString(
								"border: 1px solid %1;"
								"border-radius: 4px;" // Rounded corners
								"background-color: %2;"
							 )
					  .arg(borderColor)
					  .arg(backgroundColor)
					  );
	}

	else
	{
		Qt::WindowFlags flags = windowFlags();
		flags |= Qt::Dialog; // This causes Qt to create an NSPanel instead of an NSWindow.
		flags |= Qt::Tool;
		flags &= ~Qt::FramelessWindowHint;
		flags &= ~Qt::WindowMinimizeButtonHint;
		setWindowFlags(flags);
		setFixedSize(size());
		setAttribute(Qt::WA_DeleteOnClose, false);

#if __APPLE__
		void *window = VuoPopover::getWindowForPopover(this);
		Class nsWindow = (Class)objc_getClass("NSWindow");

		unsigned long styleMask = 0;
		styleMask |= 1 << 0; // NSWindowStyleMaskTitled
		styleMask |= 1 << 1; // NSWindowStyleMaskClosable
		styleMask |= 1 << 3; // NSWindowStyleMaskResizable
		styleMask |= 1 << 4; // NSWindowStyleMaskUtilityWindow
		if (isDark)
			styleMask |= 1 << 13; // NSWindowStyleMaskHUDWindow
		objc_msgSend((id)window, sel_getUid("setStyleMask:"), styleMask);

		// Continue to show the panel even when the Vuo Editor isn't the focused application.
		{
			// [window setHidesOnDeactivate:NO];
			SEL setHidesOnDeactivateSEL = sel_registerName("setHidesOnDeactivate:");
			Method nsWindowSetHidesOnDeactivateMethod = class_getInstanceMethod(nsWindow, setHidesOnDeactivateSEL);
			IMP nsWindowSetHidesOnDeactivate = method_getImplementation(nsWindowSetHidesOnDeactivateMethod);
			nsWindowSetHidesOnDeactivate((id)window, method_getName(nsWindowSetHidesOnDeactivateMethod), 0);
		}

		// Disable the maximize button (since disabling flag Qt::WindowMaximizeButtonHint doesn't do it).
		{
			// zoomButton = [window standardWindowButton:NSWindowZoomButton];
			SEL standardWindowButtonSEL = sel_registerName("standardWindowButton:");
			Method nsWindowStandardWindowButtonMethod = class_getInstanceMethod(nsWindow, standardWindowButtonSEL);
			IMP nsWindowStandardWindowButton = method_getImplementation(nsWindowStandardWindowButtonMethod);
			int NSWindowZoomButton = 2;
			id zoomButton = nsWindowStandardWindowButton((id)window, method_getName(nsWindowStandardWindowButtonMethod), NSWindowZoomButton);

			if (zoomButton)
			{
				// [zoomButton setEnabled:NO];
				Class nsButton = (Class)objc_getClass("NSButton");
				SEL setEnabledSEL = sel_registerName("setEnabled:");
				Method nsButtonSetEnabledMethod = class_getInstanceMethod(nsButton, setEnabledSEL);
				IMP nsButtonSetEnabled = method_getImplementation(nsButtonSetEnabledMethod);
				nsButtonSetEnabled(zoomButton, method_getName(nsButtonSetEnabledMethod), 0);
			}
		}
#endif

		// @todo: This doesn't seem to have any effect:
		this->setGraphicsEffect(NULL);

		viewport()->setStyleSheet(QString(
								"border: none;"
								"background-color: %1;"
							 )
					  .arg(backgroundColor)
					  );
	}

	QPalette p;
	QColor bulletColor = isDark ? "#a0a0a0" : "#404040";
	p.setColor(QPalette::Normal, QPalette::Text, bulletColor);
	p.setColor(QPalette::Normal, QPalette::WindowText, bulletColor);
	setPalette(p);


	// Make a checkerboard background for VuoImage popovers.
	QImage checkerboard(maxPopoverImageWidth, maxPopoverImageHeight, QImage::Format_Grayscale8);
	checkerboard.fill(isDark ? "#232323" : "#ffffff");
	QPainter painter(&checkerboard);
	QColor c(isDark ? "#2c2c2c" : "#eeeeee");
	for (int y = 0; y < maxPopoverImageHeight; y += 32)
		for (int x = 0; x < maxPopoverImageWidth; x += 32)
			if (x%64 != y%64)
				painter.fillRect(x, y, 32, 32, c);
	document()->addResource(QTextDocument::ImageResource, QUrl("vuo-port-popover://background.png"), QVariant(checkerboard));
}

/**
 * Changes the popover to float on top of all windows (if @c top is true),
 * or to behave as a normal window (if @c top is false).  If @c top is false,
 * also lowers the popover, effectively hiding it.
 */
void VuoPortPopover::setWindowLevelAndVisibility(bool top)
{
	if (!getDetached())
		return;

	VuoPopover::setWindowLevelAndVisibility(top, this);
}

/**
 * Changes the popover to float on top of all windows (if @c top is true),
 * or to behave as a normal window (if @c top is false).
 */
void VuoPortPopover::setWindowLevel(bool top)
{
	if (!getDetached())
		return;

	VuoPopover::setWindowLevel(top, this);
}

/**
 * Handle mouse press events.
 */
void VuoPortPopover::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		if (! isDetached)
			detach();

		dragInProgress = true;
		positionBeforeDrag = event->globalPos();
	}

	QTextBrowser::mousePressEvent(event);
}

/**
 * Handle mouse move events.
 */
void VuoPortPopover::mouseMoveEvent(QMouseEvent *event)
{
	if ((event->buttons() & Qt::LeftButton) && dragInProgress)
	{
		QPoint delta = event->globalPos() - positionBeforeDrag;
		move(x() + delta.x(), y() + delta.y());
		positionBeforeDrag = event->globalPos();
	}

	QTextBrowser::mouseMoveEvent(event);
}

/**
 * Handle mouse release events.
 */
void VuoPortPopover::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
		dragInProgress = false;

	QTextBrowser::mouseReleaseEvent(event);
}

/**
 * Handle close events.
 */
void VuoPortPopover::closeEvent(QCloseEvent *event)
{
	emit popoverClosedForPort(portID);
	QTextBrowser::closeEvent(event);
}

/**
 * Handle resize events.
 */
void VuoPortPopover::resizeEvent(QResizeEvent *event)
{
	emit popoverResized();
	QTextBrowser::resizeEvent(event);
}

/**
 * Returns the mean of the time interval (in seconds) between
 * events for the events whose timestamps (in ms since epoch) are contained
 * in the input @c timestamps queue.
 *
 * If the queue does not contain enough data to calculate a mean interval
 * (i.e., it has fewer than 2 entries), this function returns -1.
 */
double VuoPortPopover::getEventIntervalMean(QQueue<qint64> timestamps)
{
	if (timestamps.size() <= 1)
		return -1;

	double secBetweenFirstAndLastEvents = (timestamps.last() - timestamps.first())/1000.;
	int numEventIntervals = timestamps.size()-1;

	return (secBetweenFirstAndLastEvents/(1.0*numEventIntervals));
}

/**
 * Returns the standard deviation of the time interval (in seconds) between
 * events for the events whose timestamps (in ms since epoch) are contained
 * in the input @c timestamps queue.
 *
 * If the queue does not contain enough data to calculate a standard deviation
 * (i.e., it has fewer than 3 entries), this function returns -1.
 */
double VuoPortPopover::getEventIntervalStdDev(QQueue<qint64> timestamps)
{
	int numIntervals = timestamps.size() - 1;
	if (numIntervals < 2)
		return -1;

	double meanIntervalInSec = getEventIntervalMean(timestamps);
	double diffSquaredSum = 0;
	for (int i = 1; i < timestamps.size(); ++i)
	{
		double intervalInSec = (timestamps[i] - timestamps[i-1])/1000.;
		diffSquaredSum += pow(abs(intervalInSec-meanIntervalInSec), 2);
	}

	return pow(diffSquaredSum/(1.0*(numIntervals-1)), 0.5);
}

void VuoPortPopover::helpButtonClicked()
{
	QDesktopServices::openUrl(QUrl("vuo-help:how-events-travel-through-a-subcomposition.html"));
}

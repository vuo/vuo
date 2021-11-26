/**
 * @file
 * VuoRunner interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include <map>
#include <set>

#include "VuoMacOSSDKWorkaround.h"
#include <dispatch/dispatch.h>
#include "VuoTelemetry.hh"
class VuoRunnerDelegate;
class VuoRunningCompositionLibraries;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include "json-c/json.h"
#pragma clang diagnostic pop


/**
 * This class runs and controls a composition that has been compiled and linked by VuoCompiler.
 *
 * The Vuo SDK provides examples of using this class under `example/runner`.
 *
 * ### Conventions for host applications
 *
 * If your application runs Image Generator, Image Filter, or Image Transition protocol compositions,
 * it should typically use VuoRunner like this:
 *
 *    - start()
 *    - repeat:
 *       - setPublishedInputPortValues()
 *       - firePublishedInputPortEvent()
 *       - waitForFiredPublishedInputPortEvent()
 *       - getPublishedOutputPortValue()
 *    - stop()
 *
 * When you call firePublishedInputPortEvent(), the list of published input ports that you pass should
 * contain exactly those published input ports whose values have changed since the previous event.
 *
 * ### Constructing a runner
 *
 * The composition can run in the same process as the VuoRunner or in a separate process,
 * depending on how the VuoRunner is constructed.
 *
 * To construct a VuoRunner, use one of the factory methods:
 *
 *    - VuoRunner::newSeparateProcessRunnerFromExecutable()
 *    - VuoRunner::newSeparateProcessRunnerFromDynamicLibrary()
 *    - VuoRunner::newCurrentProcessRunnerFromDynamicLibrary()
 *    - VuoCompiler::newSeparateProcessRunnerFromCompositionFile()
 *    - VuoCompiler::newSeparateProcessRunnerFromCompositionString()
 *    - VuoCompiler::newCurrentProcessRunnerFromCompositionFile()
 *    - VuoCompiler::newCurrentProcessRunnerFromCompositionString()
 *
 * ### Starting and stopping a composition
 *
 * To start a composition running, call start() or startPaused().
 *
 * To stop the composition, call stop(). The composition must be stopped by the time VuoRunner's
 * destructor is called. If the composition process ends on its own, the VuoRunner detects this
 * and stops itself.
 *
 * ### Controlling and querying a composition
 *
 * While the composition is running, the VuoRunner can control it by sending control request messages
 * to it. The VuoRunner functions that send control request messages must only be called while the
 * composition is running.
 *
 * Functions that send control request messages are mutually thread-safe, so they may be called
 * concurrently with each other.
 *
 * ### Receiving notifications from a composition
 *
 * While the composition is running, the VuoRunner receives telemetry messages from it. To receive
 * notifications of these messages, create a class that inherits from VuoRunnerDelegate, and call
 * VuoRunner::setDelegate().
 *
 * @see DevelopingApplications
 * @see VuoRunnerCocoa (a similar API, in Objective-C)
 */
class VuoRunner
{
public:
	class Port;
	static VuoRunner * newSeparateProcessRunnerFromExecutable(string executablePath, string sourceDir, bool continueIfRunnerDies, bool deleteExecutableWhenFinished);
	static VuoRunner * newSeparateProcessRunnerFromDynamicLibrary(string compositionLoaderPath, string compositionDylibPath, const std::shared_ptr<VuoRunningCompositionLibraries> &runningCompositionLibraries, string sourceDir, bool continueIfRunnerDies = false, bool deleteDylibsWhenFinished = false);
	static VuoRunner * newCurrentProcessRunnerFromDynamicLibrary(string dylibPath, string sourceDir, bool deleteDylibWhenFinished = false);
	~VuoRunner(void);
	void setRuntimeChecking(bool runtimeCheckingEnabled);
	void start(void);
	void startPaused(void);
	void runOnMainThread(void);
	void drainMainDispatchQueue(void);
	void pause(void);
	void unpause(void);
	void replaceComposition(string compositionDylibPath, string compositionDiff);
	void stop(void);
	void waitUntilStopped(void);
	void setPublishedInputPortValues(map<Port *, json_object *> portsAndValues);
	void firePublishedInputPortEvent(Port *port);
	void firePublishedInputPortEvent(const set<Port *> &ports);
	void waitForFiredPublishedInputPortEvent(void);
	json_object * getPublishedInputPortValue(Port *port);
	json_object * getPublishedOutputPortValue(Port *port);
	vector<Port *> getPublishedInputPorts(void);
	vector<Port *> getPublishedOutputPorts(void);
	Port * getPublishedInputPortWithName(string name);
	Port * getPublishedOutputPortWithName(string name);
	void setInputPortValue(string compositionIdentifier, string portIdentifier, json_object *value);
	void fireTriggerPortEvent(string compositionIdentifier, string portIdentifier);
	json_object * getInputPortValue(string compositionIdentifier, string portIdentifier);
	json_object * getOutputPortValue(string compositionIdentifier, string portIdentifier);
	string getInputPortSummary(string compositionIdentifier, string portIdentifier);
	string getOutputPortSummary(string compositionIdentifier, string portIdentifier);
	string subscribeToInputPortTelemetry(string compositionIdentifier, string portIdentifier);
	string subscribeToOutputPortTelemetry(string compositionIdentifier, string portIdentifier);
	void unsubscribeFromInputPortTelemetry(string compositionIdentifier, string portIdentifier);
	void unsubscribeFromOutputPortTelemetry(string compositionIdentifier, string portIdentifier);
	void subscribeToEventTelemetry(string compositionIdentifier);
	void unsubscribeFromEventTelemetry(string compositionIdentifier);
	void subscribeToAllTelemetry(string compositionIdentifier);
	void unsubscribeFromAllTelemetry(string compositionIdentifier);
	bool isStopped(void);
	void setDelegate(VuoRunnerDelegate *delegate);
	pid_t getCompositionPid();

	/**
	 * This class represents a published port in a composition. It maintains a list of the identifiers
	 * of internal ports within the composition for which the published port is an alias, but
	 * in contrast to a VuoPublishedPort object, does not maintain pointers to the actual port objects.
	 *
	 * @see VuoPublishedPort
	 */
	class Port
	{
	public:
		Port(string name, string type, json_object *details);
		string getName(void);
		string getType(void);
		json_object *getDetails(void);

		friend class VuoRunner;

	private:
		string name;
		string type;
		json_object *details;
	};

private:
	string executablePath;  ///< The path to the linked composition executable.
	string dylibPath;  ///< The path to the linked composition dynamic library.
	void *dylibHandle;  ///< A handle to the linked composition dynamic library.
	std::shared_ptr<VuoRunningCompositionLibraries> dependencyLibraries;  ///< Libraries referenced by the composition when running with the option for live coding.
	bool shouldContinueIfRunnerDies;  ///< True if the composition should keep running if the runner process ends.
	bool shouldDeleteBinariesWhenFinished;  ///< True if the composition binary file(s) should be deleted when the runner is finished using them.
	string sourceDir;  ///< The directory containing the composition's .vuo source file.
	bool isRuntimeCheckingEnabled;
	bool paused;  ///< True if the composition is in a paused state.
	bool stopped;	///< True if the composition is in a stopped state (either never started or started then stopped).
	bool lostContact;   ///< True if the runner stopped receiving communication from the composition.
	string listenError;   ///< The error encountered during @ref VuoRunner::listen.
	bool listenCanceled;  ///< True if the listen() loop should end.
	dispatch_semaphore_t stoppedSemaphore;  ///< Signaled when the composition stops.
	dispatch_semaphore_t terminatedZMQContextSemaphore;  ///< Signaled when ZMQContext is terminated by stopBecauseLostContact().
	dispatch_semaphore_t beganListeningSemaphore;  ///< Signaled when the listen() socket connects.
	dispatch_semaphore_t endedListeningSemaphore;  ///< Signaled when the listen() loop ends.
	dispatch_semaphore_t lastFiredEventSemaphore;  ///< Signaled when the event most recently fired from @ref firePublishedInputPortEvent() finishes propagating through the composition.
	bool lastFiredEventSignaled;  ///< Needed for @ref lastFiredEventSemaphore as a saturating semaphore.
	dispatch_queue_t controlQueue;  ///< Synchronizes control requests, so that each control request+reply is completed before the next begins.
	pid_t compositionPid;	///< The Unix process id of the running composition.
	int runnerReadCompositionWritePipe[2];	///< A Unix pipe used to wait for compositionPid to finish.

	void *ZMQContext;	///< The context used to initialize sockets.
	void *ZMQSelfReceive;	///< VuoRunner self-control socket. Not thread-safe.
	void *ZMQSelfSend;		///< VuoRunner self-control socket. Not thread-safe.
	void *ZMQControl;	///< The control socket. Not thread-safe.
	void *ZMQTelemetry;	///< The telemetry socket. Not thread-safe.
	void *ZMQLoaderControl;	 ///< The composition loader control socket. Not thread-safe.
	string ZMQControlURL;  ///< The URL for the control socket.
	string ZMQLoaderControlURL;  ///< The URL for the composition loader control socket.
	string ZMQTelemetryURL;  ///< The URL for the telemetry socket.

	VuoRunnerDelegate *delegate;  ///< The delegate that receives telemetry data. May be null.
	dispatch_queue_t delegateQueue;  ///< Synchronizes access to @ref delegate.

	vector<Port *> publishedInputPorts;
	vector<Port *> publishedOutputPorts;
	bool arePublishedInputPortsCached;  ///< True if the list of published input ports has been retrieved and cached.
	bool arePublishedOutputPortsCached; ///< True if the list of published output ports has been retrieved and cached.

	class Private;
	Private *p;

	void saturating_semaphore_signal(dispatch_semaphore_t dsema, bool *sent);
	void saturating_semaphore_wait(dispatch_semaphore_t dsema, bool *sent);

	VuoRunner(void);
	void startInternal(void);
	void listen();
	void setUpConnections(void);
	void cleanUpConnections(void);
	void vuoControlRequestSend(enum VuoControlRequest request, zmq_msg_t *messages, unsigned int messageCount);
	void vuoLoaderControlRequestSend(enum VuoLoaderControlRequest request, zmq_msg_t *messages, unsigned int messageCount);
	void vuoControlReplyReceive(enum VuoControlReply expectedReply);
	void vuoLoaderControlReplyReceive(enum VuoLoaderControlReply expectedReply);
	string receiveString(string fallbackIfNull);
	vector<string> receiveListOfStrings(void);
	vector<Port *> getCachedPublishedPorts(bool input);
	vector<Port *> refreshPublishedPorts(bool input);
	bool isInCurrentProcess(void);
	bool isUsingCompositionLoader(void);
	void stopBecauseLostContact(string errorMessage);
	void copyDylibAndChangeId(string dylibPath, string &outputDylibPath);

	friend void *VuoRunner_listen(void *context);
	friend class TestControlAndTelemetry;
	friend class TestVuoRunner;
};


/**
 * An abstract class to be implemented by a client that will receive telemetry messages from the composition.
 * Use VuoRunner::setDelegate() to connect a VuoRunnerDelegate to a VuoRunner. The VuoRunner calls the
 * VuoRunnerDelegate functions when it receives telemetry messages from the composition.
 *
 * VuoRunner calls the VuoRunnerDelegate functions sequentially (not concurrently). VuoRunner waits for each
 * VuoRunnerDelegate function call to return before it calls another. If VuoRunner receives additional
 * telemetry messages while a VuoRunnerDelegate function call is in progress, the messages are enqueued
 * and the additional VuoRunnerDelegate functions are called after the in-progress one completes.
 *
 * When VuoRunner is replacing or stopping the composition, it waits for any in-progress VuoRunnerDelegate
 * function calls to return.
 *
 * Inherit from this class if you want to implement all of the delegate methods.
 * If you'd only like to implement some delegate methods, inherit from VuoRunnerDelegateAdapter.
 */
class VuoRunnerDelegate
{
public:
	/**
	 * This delegate method is invoked twice per second, to provide a heartbeat indicating that the composition is still alive.
	 *
	 * (It formerly provided some basic usage stats, but the parameter values are now always 0.)
	 */
	virtual void receivedTelemetryStats(unsigned long utime, unsigned long stime) = 0;

	/**
	 * This delegate method is invoked every time a node has started executing.
	 * @param compositionIdentifier A unique identifier representing the composition instance (top-level composition or a subcomposition within it) that contains the node.
	 * @param nodeIdentifier A unique identifier representing the node that started executing (see VuoCompilerNode::getIdentifier()).
	 * @version200Changed{Added `compositionIdentifier` argument.}
	 */
	virtual void receivedTelemetryNodeExecutionStarted(string compositionIdentifier, string nodeIdentifier) = 0;

	/**
	 * This delegate method is invoked every time a node has finished executing.
	 * @param compositionIdentifier A unique identifier representing the composition instance (top-level composition or a subcomposition within it) that contains the node.
	 * @param nodeIdentifier A unique identifier representing the node that finished executing (see VuoCompilerNode::getIdentifier()).
	 * @version200Changed{Added `compositionIdentifier` argument.}
	 */
	virtual void receivedTelemetryNodeExecutionFinished(string compositionIdentifier, string nodeIdentifier) = 0;

	/**
	 * This delegate method is invoked every time any input port receives an event or data.
	 * @param compositionIdentifier A unique identifier representing the composition instance (top-level composition or a subcomposition within it) that contains the port.
	 * @param portIdentifier A unique identifier representing the port that has received an event or data (see VuoCompilerEventPort::getIdentifier()).
	 * @param receivedEvent True if the port received an event.
	 * @param receivedData True if the port received data.
	 * @param dataSummary A brief description of the new data value of the port, or an empty string if the port is event-only.
	 * @version200Changed{Added `compositionIdentifier` argument.}
	 */
	virtual void receivedTelemetryInputPortUpdated(string compositionIdentifier, string portIdentifier, bool receivedEvent, bool receivedData, string dataSummary) = 0;

	/**
	 * This delegate method is invoked every time any output port transmits/fires an event or data.
	 * @param compositionIdentifier A unique identifier representing the composition instance (top-level composition or a subcomposition within it) that contains the port.
	 * @param portIdentifier A unique identifier representing the port that has transmitted or fired an event (see VuoCompilerEventPort::getIdentifier()).
	 * @param sentEvent True if the port sent an event.
	 * @param sentData True if the port sent data.
	 * @param dataSummary A brief description of the new data value of the port, or an empty string if the port is event-only.
	 * @version200Changed{Added `compositionIdentifier`, `sentEvent` arguments.}
	 */
	virtual void receivedTelemetryOutputPortUpdated(string compositionIdentifier, string portIdentifier, bool sentEvent, bool sentData, string dataSummary) = 0;

	/**
	 * This delegate method is invoked every time any published output port in the top-level composition transmits an event.
	 * @param port The VuoRunner::Port that has transmitted an event (see VuoRunner::getPublishedOutputPorts() and VuoRunner::getPublishedOutputPortWithName()).
	 * @param sentData True if the port sent data along with the event.
	 * @param dataSummary A brief description of the new data value of the port, or an empty string if the port is event-only.
	 */
	virtual void receivedTelemetryPublishedOutputPortUpdated(VuoRunner::Port *port, bool sentData, string dataSummary) = 0;

	/**
	 * This delegate method is invoked every time any trigger port drops an event.
	 * @param compositionIdentifier A unique identifier representing the composition instance (top-level composition or a subcomposition within it) that contains the port.
	 * @param portIdentifier A unique identifier representing the port that has dropped an event (see VuoCompilerEventPort::getIdentifier()).
	 * @version200Changed{Added `compositionIdentifier` argument.}
	 */
	virtual void receivedTelemetryEventDropped(string compositionIdentifier, string portIdentifier) = 0;

	/**
	 * This delegate method is invoked every time an uncaught error occurs in the composition.
	 * @param message A message with information about the error.
	 */
	virtual void receivedTelemetryError(string message) = 0;

	/**
	 * This delegate method is invoked if the runner receives no telemetry messages from the composition for at least 1 second,
	 * indicating that the composition has crashed or quit on its own.
	 */
	virtual void lostContactWithComposition(void) = 0;

	virtual ~VuoRunnerDelegate() = 0;  // Fixes "virtual functions but non-virtual destructor" warning
};

/**
 * Suppresses the warning for an unused variable.
 */
#define VUO_UNUSED_VARIABLE __attribute__((unused))

/**
 * A class provided for convenience when inheriting from VuoRunnerDelegate. If inheriting from this
 * class instead of VuoRunnerDelegate directly, the derived class does not need to implement all
 * VuoRunnerDelegate methods.
 */
class VuoRunnerDelegateAdapter : public VuoRunnerDelegate
{
private:
	virtual void receivedTelemetryStats(unsigned long VUO_UNUSED_VARIABLE utime, unsigned long VUO_UNUSED_VARIABLE stime) { }
	virtual void receivedTelemetryNodeExecutionStarted(string VUO_UNUSED_VARIABLE compositionIdentifier, string VUO_UNUSED_VARIABLE nodeIdentifier) { }
	virtual void receivedTelemetryNodeExecutionFinished(string VUO_UNUSED_VARIABLE compositionIdentifier, string VUO_UNUSED_VARIABLE nodeIdentifier) { }
	virtual void receivedTelemetryInputPortUpdated(string VUO_UNUSED_VARIABLE compositionIdentifier, string VUO_UNUSED_VARIABLE portIdentifier, bool VUO_UNUSED_VARIABLE receivedEvent, bool VUO_UNUSED_VARIABLE receivedData, string VUO_UNUSED_VARIABLE dataSummary) { }
	virtual void receivedTelemetryOutputPortUpdated(string VUO_UNUSED_VARIABLE compositionIdentifier, string VUO_UNUSED_VARIABLE portIdentifier, bool VUO_UNUSED_VARIABLE sentEvent, bool VUO_UNUSED_VARIABLE sentData, string VUO_UNUSED_VARIABLE dataSummary) { }
	virtual void receivedTelemetryPublishedOutputPortUpdated(VuoRunner::Port VUO_UNUSED_VARIABLE *port, bool VUO_UNUSED_VARIABLE sentData, string VUO_UNUSED_VARIABLE dataSummary) { }
	virtual void receivedTelemetryEventDropped(string VUO_UNUSED_VARIABLE compositionIdentifier, string VUO_UNUSED_VARIABLE portIdentifier) { }
	virtual void receivedTelemetryError(string VUO_UNUSED_VARIABLE message) { }
	virtual void lostContactWithComposition(void) { }
};

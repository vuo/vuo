/**
 * @file
 * VuoNodeAndPortIdentifierCache implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoNodeAndPortIdentifierCache.hh"
#include "VuoEditorComposition.hh"
#include "VuoComposition.hh"
#include "VuoCompilerNode.hh"

/**
 * Caches the identifiers for all nodes and ports (including published ports) in @a composition.
 */
void VuoNodeAndPortIdentifierCache::addCompositionComponentsToCache(VuoComposition *composition)
{
	std::lock_guard<std::mutex> lock(mapMutex);

	for (VuoNode *node : composition->getNodes())
		addNodeToCache_internal(node);

	for (VuoPublishedPort *publishedPort : composition->getPublishedInputPorts())
		addPortToCache_internal(publishedPort);

	for (VuoPublishedPort *publishedPort : composition->getPublishedOutputPorts())
		addPortToCache_internal(publishedPort);
}

/**
 * Caches the identifiers for @a node and its ports.
 */
void VuoNodeAndPortIdentifierCache::addNodeToCache(VuoNode *node)
{
	std::lock_guard<std::mutex> lock(mapMutex);

	addNodeToCache_internal(node);
}

/**
 * Like @ref addNodeToCache, but assumes the lock is already held.
 */
void VuoNodeAndPortIdentifierCache::addNodeToCache_internal(VuoNode *node)
{
	if (node && node->hasCompiler())
		nodeWithGraphvizIdentifier[node->getCompiler()->getGraphvizIdentifier()] = node;

	for (VuoPort *port : node->getInputPorts())
		addPortToCache_internal(port, node);

	for (VuoPort *port : node->getOutputPorts())
		addPortToCache_internal(port, node);
}

/**
 * Caches the identifier for @a publishedPort.
 */
void VuoNodeAndPortIdentifierCache::addPublishedPortToCache(VuoPublishedPort *publishedPort)
{
	std::lock_guard<std::mutex> lock(mapMutex);

	addPortToCache_internal(publishedPort);
}

/**
 * Like @ref addPublishedPortToCache, but assumes the lock is already held.
 */
void VuoNodeAndPortIdentifierCache::addPortToCache_internal(VuoPort *port, VuoNode *node)
{
	string staticPortIdentifier = VuoEditorComposition::getIdentifierForStaticPort(port, node);
	portWithStaticIdentifier[staticPortIdentifier] = port;
	staticIdentifierForPort[port] = staticPortIdentifier;
}

/**
 * Removes all items from the cache.
 */
void VuoNodeAndPortIdentifierCache::clearCache(void)
{
	std::lock_guard<std::mutex> lock(mapMutex);

	nodeWithGraphvizIdentifier.clear();
	portWithStaticIdentifier.clear();
	staticIdentifierForPort.clear();
}

/**
 * Thread-safely looks up the node with cached @a identifier and calls @a callback on it.
 *
 * Returns true if the node is found, false otherwise. If the node is not found, @a callback is not called.
 *
 * When possible, it's safest to avoid using the `VuoNode` object (or anything attached to it, such as its `VuoRendererNode`)
 * outside of @a callback. If you do allow the node to leak out, make sure the node is not going to be removed from the
 * composition while you're using it.
 *
 * @a callback should not put synchronous calls on `VuoEditorComposition::activePopoversQueue` (to avoid deadlock).
 */
bool VuoNodeAndPortIdentifierCache::doForNodeWithIdentifier(const string &identifier, std::function<void(VuoNode *)> callback)
{
	std::lock_guard<std::mutex> lock(mapMutex);

	auto foundIter = nodeWithGraphvizIdentifier.find(identifier);
	if (foundIter != nodeWithGraphvizIdentifier.end())
	{
		callback(foundIter->second);
		return true;
	}

	return false;
}

/**
 * Thread-safely looks up the port with cached @a identifier and calls @a callback on it.
 *
 * Returns true if the port is found, false otherwise. If the port is not found, @a callback is not called.
 *
 * When possible, it's safest to avoid using the `VuoPort` object (or anything attached to it, such as its `VuoRendererPort`)
 * outside of @a callback. If you do allow the port to leak out, make sure the port is not going to be removed from the
 * composition while you're using it.
 *
 * @a callback should not put synchronous calls on `VuoEditorComposition::activePopoversQueue` (to avoid deadlock).
 */
bool VuoNodeAndPortIdentifierCache::doForPortWithIdentifier(const string &identifier, std::function<void(VuoPort *)> callback)
{
	std::lock_guard<std::mutex> lock(mapMutex);

	auto foundIter = portWithStaticIdentifier.find(identifier);
	if (foundIter != portWithStaticIdentifier.end())
	{
		callback(foundIter->second);
		return true;
	}

	return false;
}

/**
 * Returns the cached identifier for @a port, or an empty string if not found.
 */
string VuoNodeAndPortIdentifierCache::getIdentifierForPort(VuoPort *port)
{
	std::lock_guard<std::mutex> lock(mapMutex);

	auto foundIter = staticIdentifierForPort.find(port);
	if (foundIter != staticIdentifierForPort.end())
		return foundIter->second;

	return "";
}

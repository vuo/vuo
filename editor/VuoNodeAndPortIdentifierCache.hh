/**
 * @file
 * VuoNodeAndPortIdentifierCache interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include <mutex>
class VuoComposition;
class VuoNode;
class VuoPort;
class VuoPublishedPort;

/**
 * Cache of nodes' graphviz identifiers and ports' static identifiers, with thread-safe access.
 */
class VuoNodeAndPortIdentifierCache
{
public:
	void addCompositionComponentsToCache(VuoComposition *composition);
	void addNodeToCache(VuoNode *node);
	void addPublishedPortToCache(VuoPublishedPort *publishedPort);
	void clearCache(void);
	bool doForNodeWithIdentifier(const string &identifier, std::function<void(VuoNode *)> callback);
	bool doForPortWithIdentifier(const string &identifier, std::function<void(VuoPort *)> callback);
	string getIdentifierForPort(VuoPort *port);

private:
	void addNodeToCache_internal(VuoNode *node);
	void addPortToCache_internal(VuoPort *port, VuoNode *node = nullptr);

	map<string, VuoNode *> nodeWithGraphvizIdentifier;
	map<string, VuoPort *> portWithStaticIdentifier;
	map<VuoPort *, string> staticIdentifierForPort;

	std::mutex mapMutex;   ///< Synchronizes access to @ref nodeWithGraphvizIdentifier, @ref portWithStaticIdentifier, and @ref staticIdentifierForPort.
};

/**
 * @file
 * TestVuoDirectedAcyclicGraph interface and implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <Vuo/Vuo.h>

/**
 * Tests of the VuoDirectedAyclicGraph class.
 */
class TestVuoDirectedAcyclicGraph : public QObject
{
	Q_OBJECT

private:

	class Vertex : public QString, public VuoDirectedAcyclicGraph::Vertex
	{
	public:
		Vertex(const QString &s) : QString(s)
		{
		}

		string key(void)
		{
			return toUtf8().constData();
		}
	};

	void addVerticesFromString(VuoDirectedAcyclicGraph &graph, const QString &vertices, map<QString, Vertex *> &verticesByName)
	{
		QStringList verticesList = vertices.split(' ', QString::SkipEmptyParts);
		std::random_shuffle(verticesList.begin(), verticesList.end());
		foreach (QString v, verticesList)
		{
			Vertex *vertex = new Vertex(v);
			verticesByName[v] = vertex;
			graph.addVertex(vertex);
		}
	}

	void addEdgesFromString(VuoDirectedAcyclicGraph &graph, const QString &edges, map<QString, Vertex *> verticesByName)
	{
		QStringList edgesList = edges.split(' ', QString::SkipEmptyParts);
		std::random_shuffle(edgesList.begin(), edgesList.end());
		foreach (QString e, edgesList)
		{
			QStringList vv = e.split(',');
			Vertex *fromVertex = verticesByName[vv.front()];
			Vertex *toVertex = verticesByName[vv.back()];
			QVERIFY2(fromVertex, vv.front().toUtf8().constData());
			QVERIFY2(toVertex, vv.back().toUtf8().constData());
			graph.addEdge(fromVertex, toVertex);
		}
	}

	QString verticesVectorToString(const vector<VuoDirectedAcyclicGraph::Vertex *> &vertices)
	{
		QStringList list;
		foreach (VuoDirectedAcyclicGraph::Vertex *v, vertices)
		{
			Vertex *vertex = dynamic_cast<TestVuoDirectedAcyclicGraph::Vertex *>(v);
			list.append(*vertex);
		}
		list.sort();
		return list.join(' ');
	}

	QString verticesSetToString(const set<VuoDirectedAcyclicGraph::Vertex *> &vertices)
	{
		vector<VuoDirectedAcyclicGraph::Vertex *> verticesVec(vertices.begin(), vertices.end());
		return verticesVectorToString(verticesVec);
	}

private slots:

	void testReachableVertices_data()
	{
		QTest::addColumn<QString>("vertices");
		QTest::addColumn<QString>("edges");
		QTest::addColumn<QString>("query");
		QTest::addColumn<QString>("downstream");
		QTest::addColumn<QString>("upstream");

		QTest::newRow("no edges") << "A B C" << "" << "A" << "" << "";

		//    A
		//   / \
		//  B   C
		//    / | \
		//   D  E  F
		QString treeVertices("A B C D E F");
		QString treeEdges("A,B A,C C,D C,E C,F");
		QTest::newRow("tree lower leaf") << treeVertices << treeEdges << "E" << "" << "A C";
		QTest::newRow("tree upper leaf") << treeVertices << treeEdges << "B" << "" << "A";
		QTest::newRow("tree middle") << treeVertices << treeEdges << "C" << "D E F" << "A";
		QTest::newRow("tree root") << treeVertices << treeEdges << "A" << "B C D E F" << "";

		//    A
		//   /|
		//  B |
		//   \|
		//    C
		QString bypassVertices("A B C");
		QString bypassEdges("A,B A,C B,C");
		QTest::newRow("bypass leaf") << bypassVertices << bypassEdges << "C" << "" << "A B";
		QTest::newRow("bypass middle") << bypassVertices << bypassEdges << "B" << "C" << "A";
		QTest::newRow("bypass root") << bypassVertices << bypassEdges << "A" << "B C" << "";

		//    A
		//   / \
		//  B   C
		//   \ /
		//    D
		QString diamondVertices("A B C D");
		QString diamondEdges("A,B A,C B,D C,D");
		QTest::newRow("diamond leaf") << diamondVertices << diamondEdges << "D" << "" << "A B C";
		QTest::newRow("diamond middle") << diamondVertices << diamondEdges << "B" << "D" << "A";
		QTest::newRow("diamond root") << diamondVertices << diamondEdges << "A" << "B C D" << "";

		//    A
		//   / \
		//  B   C
		//   \ /
		//    D
		//   / \
		//  E   F
		//   \ /
		//    G
		QString doubleDiamondVertices("A B C D E F G");
		QString doubleDiamondEdges("A,B A,C B,D C,D D,E D,F E,G F,G");
		QTest::newRow("double diamond leaf") << doubleDiamondVertices << doubleDiamondEdges << "G" << "" << "A B C D E F";
		QTest::newRow("double diamond middle") << doubleDiamondVertices << doubleDiamondEdges << "D" << "E F G" << "A B C";
		QTest::newRow("double diamond root") << doubleDiamondVertices << doubleDiamondEdges << "A" << "B C D E F G" << "";
	}
	void testReachableVertices()
	{
		QFETCH(QString, vertices);
		QFETCH(QString, edges);
		QFETCH(QString, query);
		QFETCH(QString, downstream);
		QFETCH(QString, upstream);

		VuoDirectedAcyclicGraph graph;
		map<QString, Vertex *> verticesByName;

		addVerticesFromString(graph, vertices, verticesByName);
		addEdgesFromString(graph, edges, verticesByName);

		Vertex *queryVertex = verticesByName[query];
		QVERIFY(queryVertex);

		vector<VuoDirectedAcyclicGraph::Vertex *> actualDownstreamVec = graph.getDownstreamVertices(queryVertex);
		QString actualDownstream = verticesVectorToString(actualDownstreamVec);
		QCOMPARE(actualDownstream, downstream);

		vector<VuoDirectedAcyclicGraph::Vertex *> actualUpstreamVec = graph.getUpstreamVertices(queryVertex);
		QString actualUpstream = verticesVectorToString(actualUpstreamVec);
		QCOMPARE(actualUpstream, upstream);
	}

	void testCycleVertices_data()
	{
		QTest::addColumn<QString>("vertices");
		QTest::addColumn<QString>("edges");
		QTest::addColumn<QString>("cycle");

		QTest::newRow("tree") << "A B C" << "A,B A,C" << "";
		QTest::newRow("direct cycle") << "A B" << "A,B B,A" << "A B";
		QTest::newRow("indirect cycle") << "A B C" << "A,B B,C C,A" << "A B C";
		QTest::newRow("cycle with outgoing edge") << "A B C" << "A,B B,A B,C" << "A B";
	}
	void testCycleVertices()
	{
		QFETCH(QString, vertices);
		QFETCH(QString, edges);
		QFETCH(QString, cycle);

		VuoDirectedAcyclicGraph graph;
		map<QString, Vertex *> verticesByName;

		addVerticesFromString(graph, vertices, verticesByName);
		addEdgesFromString(graph, edges, verticesByName);

		set<VuoDirectedAcyclicGraph::Vertex *> actualCycleSet = graph.getCycleVertices();
		QString actualCycle = verticesSetToString(actualCycleSet);
		QCOMPARE(actualCycle, cycle);
	}

	void testLongestDownstreamPath_data()
	{
		QTest::addColumn<QString>("query");
		QTest::addColumn<int>("longest");

		QTest::newRow("tree leaf") << "D" << 0;
		QTest::newRow("tree middle") << "C" << 1;
		QTest::newRow("tree root") << "A" << 2;
	}
	void testLongestDownstreamPath()
	{
		QFETCH(QString, query);
		QFETCH(int, longest);

		//    A
		//   / \
		//  B   C
		//    / | \
		//   D  E  F
		QString vertices("A B C D E F");
		QString edges("A,B A,C C,D C,E C,F");

		VuoDirectedAcyclicGraph graph;
		map<QString, Vertex *> verticesByName;

		addVerticesFromString(graph, vertices, verticesByName);
		addEdgesFromString(graph, edges, verticesByName);

		Vertex *queryVertex = verticesByName[query];
		QVERIFY(queryVertex);

		int actualLongest = graph.getLongestDownstreamPath(queryVertex);
		QCOMPARE(actualLongest, longest);
	}

	void testModifyingGraph()
	{
		QString vertices("A B C D E");
		QString edges("A,B A,C C,D C,E");

		VuoDirectedAcyclicGraph graph;
		map<QString, Vertex *> verticesByName;
		vector<VuoDirectedAcyclicGraph::Vertex *> downstreamVec;
		QString downstream;
		vector<VuoDirectedAcyclicGraph::Vertex *> upstreamVec;
		QString upstream;
		VuoDirectedAcyclicGraph::Vertex *c = NULL;

		//    A
		//   / \
		//  B   C
		//    / |
		//   D  E
		addVerticesFromString(graph, vertices, verticesByName);
		addEdgesFromString(graph, edges, verticesByName);

		c = graph.findVertex("C");
		QVERIFY(c);

		downstreamVec = graph.getDownstreamVertices(verticesByName["A"]);
		downstream = verticesVectorToString(downstreamVec);
		QCOMPARE(downstream, QString("B C D E"));

		upstreamVec = graph.getUpstreamVertices(verticesByName["D"]);
		upstream = verticesVectorToString(upstreamVec);
		QCOMPARE(upstream, QString("A C"));

		//    A
		//   / \
		//  B   C
		//    / | \
		//   D  E  F
		addVerticesFromString(graph, "F", verticesByName);
		addEdgesFromString(graph, "C,F", verticesByName);

		downstreamVec = graph.getDownstreamVertices(verticesByName["A"]);
		downstream = verticesVectorToString(downstreamVec);
		QCOMPARE(downstream, QString("B C D E F"));

		//    A
		//   /
		//  B
		//
		//   D  E  F
		graph.removeVertex(c);

		c = graph.findVertex("C");
		QVERIFY(! c);

		downstreamVec = graph.getDownstreamVertices(verticesByName["A"]);
		downstream = verticesVectorToString(downstreamVec);
		QCOMPARE(downstream, QString("B"));

		upstreamVec = graph.getUpstreamVertices(verticesByName["D"]);
		upstream = verticesVectorToString(upstreamVec);
		QCOMPARE(upstream, QString(""));
	}

	void testReachableVerticesInNetwork_data()
	{
		QTest::addColumn<QStringList>("vertices");
		QTest::addColumn<QStringList>("edges");
		QTest::addColumn<QString>("edgesBetweenGraphs");
		QTest::addColumn<int>("queryGraph");
		QTest::addColumn<QString>("query");
		QTest::addColumn<QString>("downstream");
		QTest::addColumn<QString>("upstream");

		{
			//   0
			//  / \
			// 1   2
			//
			// 0:		1:		  2:
			//    A		    B         F
			//   / \	   / \       / \
			//  B   C     D   E     C   G

			QStringList vertices;
			vertices << "A B C" << "B D E" << "C F G";
			QStringList edges;
			edges << "A,B A,C" << "B,D B,E" << "F,C F,G";
			QString edgesBetweenGraphs("0,1 0,2");
			QTest::newRow("tree of trees: lower leaf vertex") << vertices << edges << edgesBetweenGraphs << 1 << "D" << "" << "A B B";
			QTest::newRow("tree of trees: upper leaf vertex = lower root vertex, query lower") << vertices << edges << edgesBetweenGraphs << 1 << "B" << "D E" << "A B";
			QTest::newRow("tree of trees: upper leaf vertex = lower root vertex, query upper") << vertices << edges << edgesBetweenGraphs << 0 << "B" << "B D E" << "A";
			QTest::newRow("tree of trees: upper leaf vertex = lower leaf vertex, query lower") << vertices << edges << edgesBetweenGraphs << 2 << "C" << "" << "A C F";
			QTest::newRow("tree of trees: upper leaf vertex = lower leaf vertex, query upper") << vertices << edges << edgesBetweenGraphs << 0 << "C" << "C" << "A";
			QTest::newRow("tree of trees: upper root vertex") << vertices << edges << edgesBetweenGraphs << 0 << "A" << "B B C C D E" << "";
		}

		{
			// Like VuoCompiler::makeDependencyNetwork()…
			//
			//     I2
			//    / | \
			//   /  |   G2
			//  /   | /    \
			//  |  I1       |
			//  |   | \     |
			//  |   |   G1  |
			//  |   | /     |
			//  --- I0 ------
			//
			// I2:              G2:           I1:              G1:        I0:
			//       I2a             G2a           I1a  I1b        G1a        I0a  I0b
			//     /  |  \           / \           / \              |
			//  G2a  I1a  I0a      I1b  I0b      I0b  G1a          I0b

			QStringList vertices;
			vertices << "I2a G2a I1a I0a" << "G2a I1b I0b" << "I1a I0b G1a I1b" << "G1a I0b" << "I0a I0b";
			QStringList edges;
			edges << "I2a,G2a I2a,I1a I2a,I0a" << "G2a,I1b G2a,I0b" << "I1a,I0b I1a,G1a I1b" << "G1a,I0b" << "";
			QString edgesBetweenGraphs("0,1 0,2 0,4 1,2 1,4 2,3 2,4 3,4");
			QTest::newRow("compiler dependencies: bottom leaf vertex") << vertices << edges << edgesBetweenGraphs << 4 << "I0b" << "" << "G1a G1a G2a G2a I0b I0b I0b I1a I1a I2a";
			QTest::newRow("compiler dependencies: mid root vertex") << vertices << edges << edgesBetweenGraphs << 2 << "I1a" << "G1a G1a I0b I0b I0b" << "I1a I2a";
			QTest::newRow("compiler dependencies: top root vertex") << vertices << edges << edgesBetweenGraphs << 0 << "I2a" << "G1a G1a G2a G2a I0a I0a I0b I0b I0b I0b I1a I1a I1b I1b" << "";
		}
	}
	void testReachableVerticesInNetwork()
	{
		QFETCH(QStringList, vertices);
		QFETCH(QStringList, edges);
		QFETCH(QString, edgesBetweenGraphs);
		QFETCH(int, queryGraph);
		QFETCH(QString, query);
		QFETCH(QString, downstream);
		QFETCH(QString, upstream);

		QCOMPARE(vertices.size(), edges.size());

		QList<VuoDirectedAcyclicGraph *> graphs;
		Vertex *queryVertex = NULL;

		for (int i = 0; i < vertices.size(); ++i)
		{
			map<QString, Vertex *> verticesByName;
			VuoDirectedAcyclicGraph *graph = new VuoDirectedAcyclicGraph();
			addVerticesFromString(*graph, vertices[i], verticesByName);
			addEdgesFromString(*graph, edges[i], verticesByName);
			graphs.push_back(graph);

			if (i == queryGraph)
			{
				queryVertex = verticesByName[query];
				QVERIFY(queryVertex);
			}
		}

		VuoDirectedAcyclicNetwork network;

		QStringList edgesList = edgesBetweenGraphs.split(' ', QString::SkipEmptyParts);
		std::random_shuffle(edgesList.begin(), edgesList.end());
		foreach (QString edge, edgesList)
		{
			QStringList vv = edge.split(',');
			int fromIndex = vv.front().toInt();
			int toIndex = vv.back().toInt();
			network.addEdge(graphs[fromIndex], graphs[toIndex]);
		}

		vector<VuoDirectedAcyclicGraph::Vertex *> actualDownstreamVec = network.getDownstreamVertices(queryVertex);
		QString actualDownstream = verticesVectorToString(actualDownstreamVec);
		QCOMPARE(actualDownstream, downstream);

		vector<VuoDirectedAcyclicGraph::Vertex *> actualUpstreamVec = network.getUpstreamVertices(queryVertex);
		QString actualUpstream = verticesVectorToString(actualUpstreamVec);
		QCOMPARE(actualUpstream, upstream);

		foreach (VuoDirectedAcyclicGraph *graph, graphs)
			delete graph;
	}

};

QTEST_APPLESS_MAIN(TestVuoDirectedAcyclicGraph)
#include "TestVuoDirectedAcyclicGraph.moc"

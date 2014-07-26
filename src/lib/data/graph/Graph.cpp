#include "data/graph/Graph.h"

#include "data/graph/token_component/TokenComponentSignature.h"
#include "utility/logging/logging.h"
#include "utility/utilityString.h"

Graph::Graph()
{
}

Graph::~Graph()
{
	m_edges.clear();
	m_nodes.clear();
}

Node* Graph::getNode(const std::string& fullName) const
{
	return findNode(
		[fullName](Node* n)
		{
			return n->getName() == fullName;
		}
	);
}

Edge* Graph::getEdge(Edge::EdgeType type, Node* from, Node* to) const
{
	return from->findEdgeOfType(type,
		[to](Edge* e)
		{
			return e->getTo() == to;
		}
	);
}

Node* Graph::getNodeById(Id id) const
{
	return findNode(
		[id](Node* n)
		{
			return n->getId() == id;
		}
	);
}

Edge* Graph::getEdgeById(Id id) const
{
	return findEdge(
		[id](Edge* e)
		{
			return e->getId() == id;
		}
	);
}

Token* Graph::getTokenById(Id id) const
{
	return findToken(
		[id](Token* t)
		{
			return t->getId() == id;
		}
	);
}

Node* Graph::createNodeHierarchy(const std::string& fullName)
{
	return createNodeHierarchy(Node::NODE_UNDEFINED, fullName);
}

Node* Graph::createNodeHierarchy(Node::NodeType type, const std::string& fullName)
{
	Node* node = getNode(fullName);
	if (node)
	{
		if (type != Node::NODE_UNDEFINED)
		{
			node->setType(type);
		}
		return node;
	}

	return insertNodeHierarchy(type, fullName);
}

Node* Graph::createNodeHierarchyWithDistinctSignature(const std::string& fullName, const std::string& signature)
{
	return createNodeHierarchyWithDistinctSignature(Node::NODE_UNDEFINED, fullName, signature);
}

Node* Graph::createNodeHierarchyWithDistinctSignature(
	Node::NodeType type, const std::string& fullName, const std::string& signature
){
	Node* node = getNode(fullName);
	if (node)
	{
		if (node->getComponent<TokenComponentSignature>()->getSignature() == signature)
		{
			if (type != Node::NODE_UNDEFINED)
			{
				node->setType(type);
			}
			return node;
		}

		node = insertNode(type, fullName, node->getParentNode());
	}
	else
	{
		node = insertNodeHierarchy(type, fullName);
	}

	node->addComponentSignature(std::make_shared<TokenComponentSignature>(signature));
	return node;
}

Edge* Graph::createEdge(Edge::EdgeType type, Node* from, Node* to)
{
	Edge* edge = getEdge(type, from, to);
	if (edge)
	{
		return edge;
	}

	return insertEdge(type, from, to);
}

void Graph::removeNode(Node* node)
{
	std::vector<std::shared_ptr<Node> >::const_iterator it;
	for (it = m_nodes.begin(); it != m_nodes.end(); it++)
	{
		if (it->get() == node)
		{
			break;
		}
	}

	if (it == m_nodes.end())
	{
		LOG_WARNING("Node was not found in the graph.");
		return;
	}

	node->forEachEdgeOfType(Edge::EDGE_MEMBER, [this, node](Edge* e)
	{
		if (node == e->getFrom())
		{
			this->removeNode(e->getTo());
		}
	});

	node->forEachEdge(
		[this](Edge* e)
		{
			this->removeEdgeInternal(e);
		}
	);

	if (node->getEdges().size())
	{
		LOG_ERROR("Node has still edges.");
	}

	m_nodes.erase(it);
}

void Graph::removeEdge(Edge* edge)
{
	std::vector<std::shared_ptr<Edge> >::const_iterator it;
	for (it = m_edges.begin(); it != m_edges.end(); it++)
	{
		if (it->get() == edge)
		{
			break;
		}
	}

	if (it == m_edges.end())
	{
		LOG_WARNING("Edge was not found in the graph.");
	}

	if (edge->getType() == Edge::EDGE_MEMBER)
	{
		LOG_ERROR("Can't remove member edge, without removing the child node.");
		return;
	}

	m_edges.erase(it);
}

Node* Graph::findNode(std::function<bool(Node*)> func) const
{
	std::vector<std::shared_ptr<Node> >::const_iterator it = find_if(m_nodes.begin(), m_nodes.end(),
		[&func](const std::shared_ptr<Node>& n)
		{
			return func(n.get());
		}
	);

	if (it != m_nodes.end())
	{
		return it->get();
	}

	return nullptr;
}

Edge* Graph::findEdge(std::function<bool(Edge*)> func) const
{
	std::vector<std::shared_ptr<Edge> >::const_iterator it = find_if(m_edges.begin(), m_edges.end(),
		[func](const std::shared_ptr<Edge>& e)
		{
			return func(e.get());
		}
	);

	if (it != m_edges.end())
	{
		return it->get();
	}

	return nullptr;
}

Token* Graph::findToken(std::function<bool(Token*)> func) const
{
	Node* node = findNode(func);
	if (node)
	{
		return node;
	}

	Edge* edge = findEdge(func);
	if (edge)
	{
		return edge;
	}

	return nullptr;
}

void Graph::forEachNode(std::function<void(Node*)> func) const
{
	for (const std::shared_ptr<Node>& node : m_nodes)
	{
		func(node.get());
	}
}

void Graph::forEachEdge(std::function<void(Edge*)> func) const
{
	for (const std::shared_ptr<Edge>& edge : m_edges)
	{
		func(edge.get());
	}
}

void Graph::forEachToken(std::function<void(Token*)> func) const
{
	forEachNode(func);
	forEachEdge(func);
}

void Graph::clear()
{
	m_edges.clear();
	m_nodes.clear();
}

const std::vector<std::shared_ptr<Node> >& Graph::getNodes() const
{
	return m_nodes;
}

const std::vector<std::shared_ptr<Edge> >& Graph::getEdges() const
{
	return m_edges;
}

Node* Graph::addNodeAsPlainCopy(Node* node)
{
	Node* n = getNodeById(node->getId());
	if (n)
	{
		return n;
	}

	std::shared_ptr<Node> copy = std::make_shared<Node>(*node);
	m_nodes.push_back(copy);
	return copy.get();
}

Edge* Graph::addEdgeAsPlainCopy(Edge* edge)
{
	Edge* e = getEdgeById(edge->getId());
	if (e)
	{
		return e;
	}

	Node* from = addNodeAsPlainCopy(edge->getFrom());
	Node* to = addNodeAsPlainCopy(edge->getTo());

	std::shared_ptr<Edge> copy = std::make_shared<Edge>(*edge, from, to);
	m_edges.push_back(copy);
	return copy.get();
}

const std::string Graph::DELIMITER = "::";

Node* Graph::insertNodeHierarchy(Node::NodeType type, const std::string& fullName)
{
	Node* node = nullptr;
	std::vector<std::string> names = utility::split(fullName, DELIMITER);
	std::string name;

	for (size_t i = 0; i < names.size(); i++)
	{
		if (i > 0)
		{
			name += DELIMITER;
		}
		name += names[i];

		Node* childNode = getNode(name);

		if (!childNode)
		{
			if (i == names.size() - 1)
			{
				childNode = insertNode(type, name, node);
			}
			else
			{
				childNode = insertNode(Node::NODE_UNDEFINED, name, node);
			}
		}

		node = childNode;
	}

	return node;
}

Node* Graph::insertNode(Node::NodeType type, const std::string& fullName, Node* parentNode)
{
	std::shared_ptr<Node> nodePtr = std::make_shared<Node>(type, fullName);
	m_nodes.push_back(nodePtr);

	Node* node = nodePtr.get();

	if (parentNode)
	{
		createEdge(Edge::EDGE_MEMBER, parentNode, node);
	}

	return node;
}

Edge* Graph::insertEdge(Edge::EdgeType type, Node* from, Node* to)
{
	std::shared_ptr<Edge> edgePtr = std::make_shared<Edge>(type, from, to);
	m_edges.push_back(edgePtr);
	return edgePtr.get();
}

void Graph::removeEdgeInternal(Edge* edge)
{
	std::vector<std::shared_ptr<Edge> >::const_iterator it;
	for (it = m_edges.begin(); it != m_edges.end(); it++)
	{
		if (it->get() == edge)
		{
			m_edges.erase(it);
			return;
		}
	}
}

std::ostream& operator<<(std::ostream& ostream, const Graph& graph)
{
	ostream << "Graph:\n";
	ostream << "nodes (" << graph.getNodes().size() << ")\n";
	graph.forEachNode(
		[&ostream](Node* n)
		{
			ostream << *n << '\n';
		}
	);

	ostream << "edges (" << graph.getEdges().size() << ")\n";
	graph.forEachEdge(
		[&ostream](Edge* e)
		{
			ostream << *e << '\n';
		}
	);

	return ostream;
}

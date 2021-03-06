#include "graph_decoder.h"

#include "serialize.h"

CyclesShaderEditor::CyclesNodeGraph::CyclesNodeGraph(std::string encoded_graph)
{
	std::list<EditorNode*> tmp_nodes;
	std::list<NodeConnection> tmp_connections;

	deserialize_graph(encoded_graph, tmp_nodes, tmp_connections);

	generate_output_lists(tmp_nodes, tmp_connections, nodes, connections);

	for (EditorNode* node : tmp_nodes) {
		delete node;
	}
	tmp_nodes.clear();
}
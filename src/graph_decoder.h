#pragma once

#include <list>
#include <string>
#include <vector>

#include "output.h"

namespace CyclesShaderEditor {

	// Description of a Cycles shader graph
	class CyclesNodeGraph {
	public:
		CyclesNodeGraph(std::string encoded_graph);

		std::vector<OutputNode> nodes;
		std::vector<OutputConnection> connections;
	};

}

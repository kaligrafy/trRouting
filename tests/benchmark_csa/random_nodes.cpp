#include <algorithm>
#include <iterator>
#include <random>

#include "random_nodes.hpp"
#include "node.hpp"

namespace TrRouting
{

  std::vector<std::reference_wrapper<const Node>> sampleRandomNodes(const std::map<boost::uuids::uuid, Node> &nodes, size_t count, unsigned int seed)
  {
    std::vector<std::reference_wrapper<const Node>> allNodes;
    allNodes.reserve(nodes.size());
    for (const auto &uuidAndNode : nodes)
    {
      allNodes.push_back(uuidAndNode.second);
    }

    std::vector<std::reference_wrapper<const Node>> sampledNodes;
    std::sample(allNodes.begin(), allNodes.end(), std::back_inserter(sampledNodes), count, std::mt19937(seed));
    return sampledNodes;
  }

}

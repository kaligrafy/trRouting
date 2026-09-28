#ifndef TR_RANDOM_NODES
#define TR_RANDOM_NODES

#include <algorithm>
#include <iterator>
#include <random>
#include <vector>
#include <functional>

namespace TrRouting
{
  class Node;

  /**
   * @brief Randomly sample distinct nodes, to use as benchmark origins or
   * destinations. The same seed and nodes return the same sample with the same
   * standard library, but the sample may differ between implementations (for
   * example libc++ and libstdc++).
   *
   * @tparam NodesMap A map whose values are nodes or references to nodes, for
   * example TransitData::getNodes() or StaticTransitNetwork::getNodes()
   * @param nodes The nodes to sample from
   * @param count Maximum number of nodes to return. All nodes are returned if
   * there are fewer than count
   * @param seed Seed of the pseudo-random generator
   * @return std::vector<std::reference_wrapper<const Node>> The sampled nodes,
   * in the same relative order as in nodes
   */
  template <typename NodesMap>
  std::vector<std::reference_wrapper<const Node>> sampleRandomNodes(const NodesMap &nodes, size_t count, unsigned int seed)
  {
    std::vector<std::reference_wrapper<const Node>> allNodes;
    allNodes.reserve(nodes.size());
    for (const auto &keyAndNode : nodes)
    {
      allNodes.push_back(keyAndNode.second);
    }

    std::vector<std::reference_wrapper<const Node>> sampledNodes;
    std::sample(allNodes.begin(), allNodes.end(), std::back_inserter(sampledNodes), count, std::mt19937(seed));
    return sampledNodes;
  }

}

#endif // TR_RANDOM_NODES

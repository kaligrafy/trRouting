#ifndef TR_RANDOM_NODES
#define TR_RANDOM_NODES

#include <map>
#include <vector>
#include <functional>
#include <boost/uuid/uuid.hpp>

namespace TrRouting
{
  class Node;

  /**
   * @brief Randomly sample distinct nodes, to use as benchmark origins or
   * destinations. The same seed and nodes return the same sample with the same
   * standard library, but the sample may differ between implementations (for
   * example libc++ and libstdc++).
   *
   * @param nodes The nodes to sample from
   * @param count Maximum number of nodes to return. All nodes are returned if
   * there are fewer than count
   * @param seed Seed of the pseudo-random generator
   * @return std::vector<std::reference_wrapper<const Node>> The sampled nodes,
   * in the same relative order as in nodes
   */
  std::vector<std::reference_wrapper<const Node>> sampleRandomNodes(const std::map<boost::uuids::uuid, Node> &nodes, size_t count, unsigned int seed);

}

#endif // TR_RANDOM_NODES

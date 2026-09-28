#ifndef TR_STATIC_TRANSIT_NETWORK
#define TR_STATIC_TRANSIT_NETWORK

#include <map>
#include <set>
#include <vector>
#include <functional>

#include "node.hpp"

namespace TrRouting
{
  class Path;

  /**
   * @brief Static transit network of a set of paths, without any time
   * information: the stops served by the paths and the walking transfers
   * between these stops.
   */
  class StaticTransitNetwork
  {
  public:
    /**
     * @brief Build the network of the paths
     *
     * @param paths The paths of the network, for example from getScenarioPaths.
     * The paths and their nodes must outlive the network.
     */
    StaticTransitNetwork(const std::vector<std::reference_wrapper<const Path>> &paths);

    /** @return The paths of the network, in the order given to the constructor */
    const std::vector<std::reference_wrapper<const Path>> &getPaths() const { return paths; }

    /** @return The stops served by at least one path, by Node::uid (unique for the run, unlike Node::id) */
    const std::map<Node::uid_t, std::reference_wrapper<const Node>> &getNodes() const { return nodes; }

    /**
     * @return The walking transfers (Node::transferableNodes) between the stops
     * of the network, as the uids of the stops reachable from each stop. Stops
     * without transfers are absent. A transfer listed in only one direction is
     * also added in the reverse direction, and transfers to the same stop are
     * ignored.
     */
    const std::map<Node::uid_t, std::set<Node::uid_t>> &getTransfers() const { return transfers; }

  private:
    std::vector<std::reference_wrapper<const Path>> paths;
    std::map<Node::uid_t, std::reference_wrapper<const Node>> nodes;
    std::map<Node::uid_t, std::set<Node::uid_t>> transfers;
  };

}

#endif // TR_STATIC_TRANSIT_NETWORK

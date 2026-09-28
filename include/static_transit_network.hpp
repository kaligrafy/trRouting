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

    /**
     * @param uid The uid of a stop of the network
     * @return std::vector<Node::uid_t> The stop and the stops reachable from it
     * by a walking transfer, ie where a path can be boarded after leaving
     * another one at this stop
     */
    std::vector<Node::uid_t> getStopAndTransferStops(Node::uid_t uid) const;

    /**
     * @return The positions of each stop in the paths, as (index of the path in
     * getPaths(), index of the stop in the path's nodesRef). A stop served
     * twice by a path has two positions in this path.
     */
    const std::map<Node::uid_t, std::vector<std::pair<size_t, size_t>>> &getPathPositions() const { return pathPositions; }

  private:
    std::vector<std::reference_wrapper<const Path>> paths;
    std::map<Node::uid_t, std::reference_wrapper<const Node>> nodes;
    std::map<Node::uid_t, std::set<Node::uid_t>> transfers;
    std::map<Node::uid_t, std::vector<std::pair<size_t, size_t>>> pathPositions;
  };

}

#endif // TR_STATIC_TRANSIT_NETWORK

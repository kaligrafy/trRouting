#include "static_transit_network.hpp"
#include "path.hpp"

namespace TrRouting
{

  StaticTransitNetwork::StaticTransitNetwork(const std::vector<std::reference_wrapper<const Path>> &_paths) : paths(_paths)
  {
    for (size_t pathIndex = 0; pathIndex < paths.size(); pathIndex++)
    {
      const std::vector<std::reference_wrapper<const Node>> &pathNodes = paths[pathIndex].get().nodesRef;
      for (size_t nodeIndex = 0; nodeIndex < pathNodes.size(); nodeIndex++)
      {
        const Node &node = pathNodes[nodeIndex];
        nodes.emplace(node.uid, node);
        pathPositions[node.uid].emplace_back(pathIndex, nodeIndex);
      }
    }

    // Transfers are only kept between stops of the network, so all nodes must be known first
    for (const auto &[fromUid, node] : nodes)
    {
      for (const NodeTimeDistance &transfer : node.get().transferableNodes)
      {
        const Node::uid_t toUid = transfer.node.uid;
        if (toUid != fromUid && nodes.count(toUid) > 0)
        {
          transfers[fromUid].insert(toUid);
          transfers[toUid].insert(fromUid);
        }
      }
    }
  }

  std::vector<Node::uid_t> StaticTransitNetwork::getStopAndTransferStops(Node::uid_t uid) const
  {
    std::vector<Node::uid_t> uids = {uid};
    auto stopTransfers = transfers.find(uid);
    if (stopTransfers != transfers.end())
    {
      uids.insert(uids.end(), stopTransfers->second.begin(), stopTransfers->second.end());
    }
    return uids;
  }

}

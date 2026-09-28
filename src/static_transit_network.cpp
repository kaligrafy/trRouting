#include "static_transit_network.hpp"
#include "path.hpp"

namespace TrRouting
{

  StaticTransitNetwork::StaticTransitNetwork(const std::vector<std::reference_wrapper<const Path>> &_paths) : paths(_paths)
  {
    for (const Path &path : paths)
    {
      for (const Node &node : path.nodesRef)
      {
        nodes.emplace(node.uid, node);
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

}

#include <algorithm>
#include <iterator>
#include <map>
#include <set>
#include <vector>

#include "min_boardings.hpp"
#include "static_transit_network.hpp"
#include "path.hpp"

namespace TrRouting
{

  BoardingsByNode getMinBoardings(const StaticTransitNetwork &network,
                                  const std::unordered_set<Node::uid_t> &destinationUids,
                                  const std::function<bool(int, const BoardingsByNode &)> &isRoundNeeded,
                                  const std::function<bool(Node::uid_t)> &isStopUsable)
  {
    BoardingsByNode boardingsByNode;
    std::set<Node::uid_t> visitedAlightingStops;
    std::copy_if(destinationUids.begin(), destinationUids.end(), std::inserter(visitedAlightingStops, visitedAlightingStops.end()), isStopUsable);
    std::vector<Node::uid_t> alightingStops(visitedAlightingStops.begin(), visitedAlightingStops.end());
    for (int round = 1; !alightingStops.empty() && isRoundNeeded(round, boardingsByNode); round++)
    {
      // Only the latest alighting position of each path matters, the stops before it include those of earlier positions
      std::map<size_t, size_t> latestAlightingPositions;
      for (const Node::uid_t alightingUid : alightingStops)
      {
        for (const auto &[pathIndex, position] : network.getPathPositions().at(alightingUid))
        {
          auto [latest, inserted] = latestAlightingPositions.emplace(pathIndex, position);
          latest->second = std::max(latest->second, position);
        }
      }

      std::vector<Node::uid_t> boardingStops;
      for (const auto &[pathIndex, alightingPosition] : latestAlightingPositions)
      {
        const Path &path = network.getPaths()[pathIndex];
        for (size_t position = 0; position < alightingPosition; position++)
        {
          const Node::uid_t uid = path.nodesRef[position].get().uid;
          if (isStopUsable(uid) && boardingsByNode.emplace(uid, round).second)
          {
            boardingStops.push_back(uid);
          }
        }
      }

      // The next round's paths can be left at the new boarding stops or at the stops with a walking transfer to them
      alightingStops.clear();
      for (const Node::uid_t boardingUid : boardingStops)
      {
        for (const Node::uid_t uid : network.getStopAndTransferStops(boardingUid))
        {
          if (isStopUsable(uid) && visitedAlightingStops.insert(uid).second)
          {
            alightingStops.push_back(uid);
          }
        }
      }
    }
    return boardingsByNode;
  }

}

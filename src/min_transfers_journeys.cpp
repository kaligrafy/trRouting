#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "min_transfers_journeys.hpp"
#include "static_transit_network.hpp"
#include "path.hpp"

namespace TrRouting
{

  namespace
  {
    // Number of boardings needed to reach a destination when boarding at a stop, by stop uid
    using BoardingsByNode = std::unordered_map<Node::uid_t, int>;
    using NodesUids = std::unordered_set<Node::uid_t>;

    /**
     * Rounds from the destinations back to the origins: round r finds the stops
     * where boarding reaches a destination with r boardings, by boarding a
     * path before a stop where round r - 1 can be boarded (or before a
     * destination for round 1). Rounds after the first one reaching an origin
     * are not needed.
     */
    BoardingsByNode getBoardingsToDestinations(const StaticTransitNetwork &network, const NodesUids &originUids, const NodesUids &destinationUids)
    {
      BoardingsByNode boardingsByNode;
      std::set<Node::uid_t> visitedAlightingStops(destinationUids.begin(), destinationUids.end());
      std::vector<Node::uid_t> alightingStops(visitedAlightingStops.begin(), visitedAlightingStops.end());
      const auto isOriginReached = [&]() { return std::any_of(originUids.begin(), originUids.end(), [&](Node::uid_t uid) { return boardingsByNode.count(uid) > 0; }); };
      for (int round = 1; !alightingStops.empty() && !isOriginReached(); round++)
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
            if (boardingsByNode.emplace(uid, round).second)
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
            if (visitedAlightingStops.insert(uid).second)
            {
              alightingStops.push_back(uid);
            }
          }
        }
      }
      return boardingsByNode;
    }

    using PathsJourney = std::vector<std::reference_wrapper<const Path>>;
    // Earliest boarding index of the next paths, by index of the path in the network
    using NextBoardings = std::map<size_t, size_t>;

    // Depth-first enumeration of the journeys, only following the stops from which the destination is reachable with the remaining boardings
    class JourneysEnumerator
    {
    public:
      JourneysEnumerator(const StaticTransitNetwork &_network, const BoardingsByNode &_boardingsByNode, const NodesUids &_destinationUids, size_t _maxJourneys)
        : network(_network), boardingsByNode(_boardingsByNode), destinationUids(_destinationUids), maxJourneys(_maxJourneys) {}

      /**
       * Add the paths that can be boarded at a stop, not already in the current journey. Each path is only kept
       * at its earliest boarding index, which reaches all the stops of later indexes.
       */
      void addBoardablePaths(Node::uid_t uid, NextBoardings &nextBoardings) const
      {
        for (const auto &[pathIndex, position] : network.getPathPositions().at(uid))
        {
          const Path &path = network.getPaths()[pathIndex];
          const bool isInJourney = std::any_of(currentJourney.begin(), currentJourney.end(), [&path](const Path &journeyPath) { return &journeyPath == &path; });
          if (position + 1 < path.nodesRef.size() && !isInJourney)
          {
            auto [nextBoarding, inserted] = nextBoardings.emplace(pathIndex, position);
            nextBoarding->second = std::min(nextBoarding->second, position);
          }
        }
      }

      /** Continue the current journey with each of the next boardings, after which `remainingBoardings` boardings reach the destination */
      void boardNextPaths(const NextBoardings &nextBoardings, int remainingBoardings)
      {
        for (const auto &[pathIndex, boardingIndex] : nextBoardings)
        {
          if (journeys.size() >= maxJourneys)
          {
            return;
          }
          currentJourney.push_back(network.getPaths()[pathIndex]);
          extend(boardingIndex, remainingBoardings);
          currentJourney.pop_back();
        }
      }

      std::vector<PathsJourney> journeys;

    private:
      /** Continue the current journey after its last path, boarded at boardingIndex, with `remainingBoardings` boardings to reach the destination */
      void extend(size_t boardingIndex, int remainingBoardings)
      {
        const Path &path = currentJourney.back();
        if (remainingBoardings == 0)
        {
          const bool reachesDestination = std::any_of(path.nodesRef.begin() + boardingIndex + 1, path.nodesRef.end(), [this](const Node &node) { return destinationUids.count(node.uid) > 0; });
          if (reachesDestination)
          {
            journeys.push_back(currentJourney);
          }
          return;
        }

        NextBoardings nextBoardings;
        for (size_t position = boardingIndex + 1; position < path.nodesRef.size(); position++)
        {
          for (const Node::uid_t uid : network.getStopAndTransferStops(path.nodesRef[position].get().uid))
          {
            auto boardings = boardingsByNode.find(uid);
            if (boardings != boardingsByNode.end() && boardings->second == remainingBoardings)
            {
              addBoardablePaths(uid, nextBoardings);
            }
          }
        }
        boardNextPaths(nextBoardings, remainingBoardings - 1);
      }

      const StaticTransitNetwork &network;
      const BoardingsByNode &boardingsByNode;
      const NodesUids &destinationUids;
      const size_t maxJourneys;
      PathsJourney currentJourney;
    };
  }

  std::vector<std::vector<std::reference_wrapper<const Path>>> getMinTransfersJourneys(const StaticTransitNetwork &network, const std::vector<std::reference_wrapper<const Node>> &origins, const std::vector<std::reference_wrapper<const Node>> &destinations, size_t maxJourneys)
  {
    // Only the stops of the network can be boarded or left
    const auto getNetworkUids = [&network](const std::vector<std::reference_wrapper<const Node>> &nodes) {
      NodesUids uids;
      for (const Node &node : nodes)
      {
        if (network.getNodes().count(node.uid) > 0)
        {
          uids.insert(node.uid);
        }
      }
      return uids;
    };
    const NodesUids originUids = getNetworkUids(origins);
    const NodesUids destinationUids = getNetworkUids(destinations);
    const BoardingsByNode boardingsByNode = getBoardingsToDestinations(network, originUids, destinationUids);

    // Only the origins with the minimum number of boardings start journeys
    std::map<int, std::vector<Node::uid_t>> originsByBoardings;
    for (const Node::uid_t uid : originUids)
    {
      auto boardings = boardingsByNode.find(uid);
      if (boardings != boardingsByNode.end())
      {
        originsByBoardings[boardings->second].push_back(uid);
      }
    }
    if (originsByBoardings.empty())
    {
      return {};
    }
    const auto &[minBoardings, firstOriginUids] = *originsByBoardings.begin();

    JourneysEnumerator enumerator(network, boardingsByNode, destinationUids, maxJourneys);
    NextBoardings firstBoardings;
    for (const Node::uid_t uid : firstOriginUids)
    {
      enumerator.addBoardablePaths(uid, firstBoardings);
    }
    enumerator.boardNextPaths(firstBoardings, minBoardings - 1);
    return enumerator.journeys;
  }

}

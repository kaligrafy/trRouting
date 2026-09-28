#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>

#include "min_transfers_journeys.hpp"
#include "static_transit_network.hpp"
#include "path.hpp"

namespace TrRouting
{

  namespace
  {
    // Number of boardings needed to reach the destination when boarding at a stop, by stop uid
    using BoardingsByNode = std::unordered_map<Node::uid_t, int>;

    /**
     * Rounds from the destination back to the origin: round r finds the stops
     * where boarding reaches the destination with r boardings, by boarding a
     * path before a stop where round r - 1 can be boarded (or before the
     * destination for round 1). Stops after the origin's round are not needed.
     */
    BoardingsByNode getBoardingsToDestination(const StaticTransitNetwork &network, Node::uid_t originUid, Node::uid_t destinationUid)
    {
      BoardingsByNode boardingsByNode;
      std::set<Node::uid_t> visitedAlightingStops = {destinationUid};
      std::vector<Node::uid_t> alightingStops = {destinationUid};
      for (int round = 1; !alightingStops.empty() && boardingsByNode.count(originUid) == 0; round++)
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
      JourneysEnumerator(const StaticTransitNetwork &_network, const BoardingsByNode &_boardingsByNode, Node::uid_t _destinationUid, size_t _maxJourneys)
        : network(_network), boardingsByNode(_boardingsByNode), destinationUid(_destinationUid), maxJourneys(_maxJourneys) {}

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
          const bool reachesDestination = std::any_of(path.nodesRef.begin() + boardingIndex + 1, path.nodesRef.end(), [this](const Node &node) { return node.uid == destinationUid; });
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
      const Node::uid_t destinationUid;
      const size_t maxJourneys;
      PathsJourney currentJourney;
    };
  }

  std::vector<std::vector<std::reference_wrapper<const Path>>> getMinTransfersJourneys(const StaticTransitNetwork &network, const Node &origin, const Node &destination, size_t maxJourneys)
  {
    if (origin.uid == destination.uid || network.getNodes().count(origin.uid) == 0 || network.getNodes().count(destination.uid) == 0)
    {
      return {};
    }
    const BoardingsByNode boardingsByNode = getBoardingsToDestination(network, origin.uid, destination.uid);
    auto originBoardings = boardingsByNode.find(origin.uid);
    if (originBoardings == boardingsByNode.end())
    {
      return {};
    }

    JourneysEnumerator enumerator(network, boardingsByNode, destination.uid, maxJourneys);
    NextBoardings firstBoardings;
    enumerator.addBoardablePaths(origin.uid, firstBoardings);
    enumerator.boardNextPaths(firstBoardings, originBoardings->second - 1);
    return enumerator.journeys;
  }

}

#include <algorithm>
#include <map>
#include <unordered_map>

#include "journey_legs.hpp"
#include "static_transit_network.hpp"
#include "path.hpp"

namespace TrRouting
{

  namespace
  {
    // Fewest segments traveled to board a leg at a stop, with the previous leg's boarding and alighting indexes to rebuild the legs
    struct Boarding
    {
      size_t segments;
      size_t previousBoardingIndex;
      size_t previousAlightingIndex;
    };

    // Indexes of each stop in a path, by stop uid, only where the path can be boarded (ie not at its last stop)
    std::unordered_map<Node::uid_t, std::vector<size_t>> getBoardingIndexes(const Path &path)
    {
      std::unordered_map<Node::uid_t, std::vector<size_t>> boardingIndexes;
      for (size_t index = 0; index + 1 < path.nodesRef.size(); index++)
      {
        boardingIndexes[path.nodesRef[index].get().uid].push_back(index);
      }
      return boardingIndexes;
    }
  }

  std::vector<JourneyLeg> getFewestSegmentsLegs(const StaticTransitNetwork &network, const std::vector<std::reference_wrapper<const Path>> &journey, const Node &origin, const Node &destination)
  {
    if (journey.empty())
    {
      return {};
    }

    // Boardings of each leg, by boarding index in the leg's path
    std::vector<std::map<size_t, Boarding>> legsBoardings(journey.size());
    auto firstBoardingIndexes = getBoardingIndexes(journey.front());
    for (const size_t index : firstBoardingIndexes[origin.uid])
    {
      legsBoardings.front().emplace(index, Boarding{0, 0, 0});
    }
    for (size_t leg = 0; leg + 1 < journey.size(); leg++)
    {
      const Path &path = journey[leg];
      auto nextBoardingIndexes = getBoardingIndexes(journey[leg + 1]);
      for (const auto &[boardingIndex, boarding] : legsBoardings[leg])
      {
        for (size_t alightingIndex = boardingIndex + 1; alightingIndex < path.nodesRef.size(); alightingIndex++)
        {
          const size_t segments = boarding.segments + alightingIndex - boardingIndex;
          for (const Node::uid_t uid : network.getStopAndTransferStops(path.nodesRef[alightingIndex].get().uid))
          {
            for (const size_t nextBoardingIndex : nextBoardingIndexes[uid])
            {
              auto [nextBoarding, inserted] = legsBoardings[leg + 1].emplace(nextBoardingIndex, Boarding{segments, boardingIndex, alightingIndex});
              if (!inserted && segments < nextBoarding->second.segments)
              {
                nextBoarding->second = Boarding{segments, boardingIndex, alightingIndex};
              }
            }
          }
        }
      }
    }

    // The last leg is left at the first destination stop after its boarding
    const Path &lastPath = journey.back();
    std::vector<JourneyLeg> legs;
    size_t fewestSegments = 0;
    for (const auto &[boardingIndex, boarding] : legsBoardings.back())
    {
      for (size_t alightingIndex = boardingIndex + 1; alightingIndex < lastPath.nodesRef.size(); alightingIndex++)
      {
        if (lastPath.nodesRef[alightingIndex].get().uid == destination.uid)
        {
          const size_t segments = boarding.segments + alightingIndex - boardingIndex;
          if (legs.empty() || segments < fewestSegments)
          {
            legs = {JourneyLeg{lastPath, boardingIndex, alightingIndex}};
            fewestSegments = segments;
          }
          break;
        }
      }
    }
    if (legs.empty())
    {
      return {};
    }

    // Rebuild the previous legs from the last one, then put them in travel order
    for (size_t leg = journey.size() - 1; leg > 0; leg--)
    {
      const Boarding &boarding = legsBoardings[leg].at(legs.back().boardingIndex);
      legs.push_back(JourneyLeg{journey[leg - 1], boarding.previousBoardingIndex, boarding.previousAlightingIndex});
    }
    std::reverse(legs.begin(), legs.end());
    return legs;
  }

}

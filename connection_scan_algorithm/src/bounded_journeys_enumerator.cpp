#include <algorithm>
#include <optional>

#include "bounded_journeys_enumerator.hpp"
#include "static_transit_network.hpp"
#include "parameters.hpp"
#include "connection.hpp"
#include "path.hpp"
#include "trip.hpp"
#include "toolbox.hpp"

namespace TrRouting
{

  BoundedJourneysEnumerator::BoundedJourneysEnumerator(const StaticTransitNetwork &_network,
                                                       const TripsByPath &_tripsByPath,
                                                       const std::vector<int> &_latestArrivals,
                                                       const std::unordered_map<Node::uid_t, NodeTimeDistance> &_nodesEgress,
                                                       const BoardingsByNode &_minBoardings,
                                                       const CommonParameters &_parameters,
                                                       int _maxArrivalTime,
                                                       size_t _maxJourneys)
    : network(_network), tripsByPath(_tripsByPath), latestArrivals(_latestArrivals), nodesEgress(_nodesEgress), minBoardings(_minBoardings),
      parameters(_parameters), maxArrivalTime(_maxArrivalTime), maxJourneys(_maxJourneys) {}

  std::vector<PathsJourney> BoundedJourneysEnumerator::enumerate(const std::unordered_map<Node::uid_t, NodeTimeDistance> &nodesAccess, size_t maxPaths, size_t extraTransfers)
  {
    // The first path is boarded within the maximum waiting time at the origin, if any
    std::map<size_t, PathArrivals> firstPaths;
    const int maxFirstWaitingTime = parameters.getMaxInnerTimeOfTripBufferSeconds() > 0 ? parameters.getMaxInnerTimeOfTripBufferSeconds() : MAX_INT;
    for (const auto &[uid, access] : nodesAccess)
    {
      auto positions = network.getPathPositions().find(uid);
      if (positions == network.getPathPositions().end())
      {
        continue;
      }
      for (const auto &[pathIndex, boardingIndex] : positions->second)
      {
        board(pathIndex, boardingIndex, parameters.getTimeOfTrip() + access.time, maxFirstWaitingTime, firstPaths);
      }
    }

    journeys.clear();
    std::optional<size_t> minPaths;
    for (maxLevelPaths = 1; maxLevelPaths <= (minPaths.has_value() ? minPaths.value() + extraTransfers : maxPaths) && journeys.size() < maxJourneys; maxLevelPaths++)
    {
      extendWithNextPaths(firstPaths);
      if (!minPaths.has_value() && !journeys.empty())
      {
        minPaths = maxLevelPaths;
      }
    }
    return journeys;
  }

  /**
   * Board the earliest trip of a path leaving a stop at or after `readyTime`, and keep the earliest arrivals
   * at the next stops of this path. `maxWaitingTime` limits the waiting at the stop, MAX_INT for no limit.
   */
  void BoundedJourneysEnumerator::board(size_t pathIndex, size_t boardingIndex, int readyTime, int maxWaitingTime, std::map<size_t, PathArrivals> &nextPaths) const
  {
    const Path &path = network.getPaths()[pathIndex];
    auto trips = tripsByPath.find(&path);
    if (boardingIndex + 1 >= path.nodesRef.size() || trips == tripsByPath.end())
    {
      return;
    }
    const auto departure = [boardingIndex](const Trip *trip) { return trip->forwardConnections[boardingIndex].get().getDepartureTime(); };
    auto trip = std::lower_bound(trips->second.begin(), trips->second.end(), readyTime, [&](const Trip *trip, int time) { return departure(trip) < time; });
    for (; trip != trips->second.end() && departure(*trip) - readyTime <= maxWaitingTime; trip++)
    {
      const Connection &connection = (*trip)->forwardConnections[boardingIndex];
      if (!connection.canBoard() || connection.getDepartureTime() - connection.getMinWaitingTimeOrDefault(parameters.getMinWaitingTimeSeconds()) < readyTime)
      {
        continue;
      }
      auto [arrivals, inserted] = nextPaths.try_emplace(pathIndex, PathArrivals{std::vector<int>(path.nodesRef.size(), MAX_INT), std::vector<const Trip *>(path.nodesRef.size(), nullptr)});
      for (size_t index = boardingIndex + 1; index < path.nodesRef.size(); index++)
      {
        const int arrivalTime = (*trip)->forwardConnections[index - 1].get().getArrivalTime();
        if (arrivalTime < arrivals->second.times[index])
        {
          arrivals->second.times[index] = arrivalTime;
          arrivals->second.trips[index] = *trip;
        }
      }
      return;
    }
  }

  // Continue the current sequence with each next path from which a stop can be reached in time
  void BoundedJourneysEnumerator::extendWithNextPaths(const std::map<size_t, PathArrivals> &nextPaths)
  {
    if (currentPaths.size() >= maxLevelPaths)
    {
      return;
    }
    for (const auto &[pathIndex, arrivals] : nextPaths)
    {
      if (journeys.size() >= maxJourneys)
      {
        return;
      }
      const Path &path = network.getPaths()[pathIndex];
      bool isReachableInTime = false;
      for (size_t index = 1; index < path.nodesRef.size() && !isReachableInTime; index++)
      {
        isReachableInTime = arrivals.times[index] <= latestArrivals[path.nodesRef[index].get().uid];
      }
      if (isReachableInTime)
      {
        extend(pathIndex, arrivals);
      }
    }
  }

  // Add the path to the current sequence, keep it if it reaches the destination with the number of paths of the level, and continue with the next paths
  void BoundedJourneysEnumerator::extend(size_t pathIndex, const PathArrivals &arrivals)
  {
    const Path &path = network.getPaths()[pathIndex];
    currentPaths.push_back(pathIndex);
    const size_t remainingBoardings = maxLevelPaths - currentPaths.size();
    int arrivalTime = MAX_INT;
    std::map<size_t, PathArrivals> nextPaths;
    for (size_t index = 1; index < path.nodesRef.size(); index++)
    {
      const Node &node = path.nodesRef[index];
      const int time = arrivals.times[index];
      if (time == MAX_INT || time > latestArrivals[node.uid] || !arrivals.trips[index]->forwardConnections[index - 1].get().canUnboard())
      {
        continue;
      }
      auto egress = nodesEgress.find(node.uid);
      if (egress != nodesEgress.end() && time + egress->second.time <= maxArrivalTime)
      {
        arrivalTime = std::min(arrivalTime, time + egress->second.time);
      }
      for (const NodeTimeDistance &transferableNode : node.transferableNodes)
      {
        auto positions = network.getPathPositions().find(transferableNode.node.uid);
        auto boardings = minBoardings.find(transferableNode.node.uid);
        if (remainingBoardings == 0 || transferableNode.time > parameters.getMaxTransferWalkingTravelTimeSeconds() || positions == network.getPathPositions().end()
            || boardings == minBoardings.end() || static_cast<size_t>(boardings->second) > remainingBoardings)
        {
          continue;
        }
        for (const auto &[nextPathIndex, boardingIndex] : positions->second)
        {
          if (std::find(currentPaths.begin(), currentPaths.end(), nextPathIndex) == currentPaths.end())
          {
            board(nextPathIndex, boardingIndex, time + transferableNode.time, MAX_INT, nextPaths);
          }
        }
      }
    }
    if (arrivalTime < MAX_INT && remainingBoardings == 0)
    {
      PathsJourney journey{{}, arrivalTime};
      for (const size_t journeyPathIndex : currentPaths)
      {
        journey.paths.push_back(network.getPaths()[journeyPathIndex]);
      }
      journeys.push_back(journey);
    }
    extendWithNextPaths(nextPaths);
    currentPaths.pop_back();
  }

}

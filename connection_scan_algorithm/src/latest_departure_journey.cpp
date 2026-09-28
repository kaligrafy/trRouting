#include <vector>

#include "latest_departure_journey.hpp"
#include "parameters.hpp"
#include "connection.hpp"
#include "path.hpp"
#include "trip.hpp"
#include "toolbox.hpp"

namespace TrRouting
{

  namespace
  {
    // Latest boarding at a stop of a path: the trip, the index of the stop where it is left and the latest time to be ready at the stop
    struct LatestBoarding
    {
      const Trip *trip = nullptr;
      size_t alightingIndex = 0;
      int readyTime = -1;
    };

    /**
     * Latest boarding at each stop of a path, departing at or before its
     * maximum departure time, to leave it at a later stop at or before its
     * latest arrival time. `trips` are sorted by departure time.
     */
    std::vector<LatestBoarding> getLatestBoardings(const Path &path, const std::vector<const Trip *> &trips, const std::vector<int> &latestArrivals, const std::vector<int> &maxDepartureTimes, short defaultMinWaitingTime)
    {
      const size_t stopsCount = path.nodesRef.size();
      std::vector<LatestBoarding> boardings(stopsCount);
      size_t unresolvedCount = stopsCount - 1;
      for (auto trip = trips.rbegin(); trip != trips.rend() && unresolvedCount > 0; trip++)
      {
        // Earliest stop after the boarding index where this trip can be left in time
        std::optional<size_t> alightingIndex;
        for (size_t index = stopsCount - 1; index > 0; index--)
        {
          const Connection &connection = (*trip)->forwardConnections[index - 1];
          if (connection.canUnboard() && connection.getArrivalTime() <= latestArrivals[index])
          {
            alightingIndex = index;
          }
          if (alightingIndex.has_value() && boardings[index - 1].trip == nullptr && connection.canBoard() && connection.getDepartureTime() <= maxDepartureTimes[index - 1])
          {
            boardings[index - 1] = {*trip, alightingIndex.value(), connection.getDepartureTime() - connection.getMinWaitingTimeOrDefault(defaultMinWaitingTime)};
            unresolvedCount--;
          }
        }
      }
      return boardings;
    }

    // Walking transfer to the next path of a journey: index of the boarding stop on the next path, walking time and distance
    struct Transfer
    {
      size_t boardingIndex;
      int time;
      int distance;
    };
  }

  std::optional<std::pair<std::deque<JourneyStep>, int>> getLatestDepartureJourney(const PathsJourney &pathsJourney,
                                                                                  const TripsByPath &tripsByPath,
                                                                                  const std::unordered_map<Node::uid_t, NodeTimeDistance> &nodesAccess,
                                                                                  const std::unordered_map<Node::uid_t, NodeTimeDistance> &nodesEgress,
                                                                                  const CommonParameters &parameters)
  {
    const auto &paths = pathsJourney.paths;
    const size_t legsCount = paths.size();
    std::vector<std::vector<LatestBoarding>> boardings(legsCount);
    std::vector<std::vector<std::optional<Transfer>>> transfers(legsCount);
    const std::vector<const Trip *> noTrips;
    for (size_t leg = legsCount; leg-- > 0;)
    {
      const Path &path = paths[leg];
      std::vector<int> latestArrivals(path.nodesRef.size(), -1);
      transfers[leg].assign(path.nodesRef.size(), std::nullopt);
      // Index of the latest boarding at each stop of the next path
      std::unordered_map<Node::uid_t, size_t> nextBoardingIndexes;
      for (size_t nextIndex = 0; leg + 1 < legsCount && nextIndex + 1 < paths[leg + 1].get().nodesRef.size(); nextIndex++)
      {
        if (boardings[leg + 1][nextIndex].trip != nullptr)
        {
          auto [bestIndex, inserted] = nextBoardingIndexes.emplace(paths[leg + 1].get().nodesRef[nextIndex].get().uid, nextIndex);
          if (boardings[leg + 1][nextIndex].readyTime > boardings[leg + 1][bestIndex->second].readyTime)
          {
            bestIndex->second = nextIndex;
          }
        }
      }
      for (size_t index = 1; index < path.nodesRef.size(); index++)
      {
        const Node &node = path.nodesRef[index];
        if (leg + 1 == legsCount)
        {
          auto egress = nodesEgress.find(node.uid);
          if (egress != nodesEgress.end())
          {
            latestArrivals[index] = pathsJourney.arrivalTime - egress->second.time;
          }
          continue;
        }
        for (const NodeTimeDistance &transferableNode : node.transferableNodes)
        {
          auto nextIndex = nextBoardingIndexes.find(transferableNode.node.uid);
          if (transferableNode.time > parameters.getMaxTransferWalkingTravelTimeSeconds() || nextIndex == nextBoardingIndexes.end())
          {
            continue;
          }
          const int readyTime = boardings[leg + 1][nextIndex->second].readyTime - transferableNode.time;
          if (readyTime > latestArrivals[index])
          {
            latestArrivals[index] = readyTime;
            transfers[leg][index] = Transfer{nextIndex->second, transferableNode.time, transferableNode.distance};
          }
        }
      }
      // The first path is boarded within the maximum waiting time at the origin, if any
      std::vector<int> maxDepartureTimes(path.nodesRef.size(), MAX_INT);
      for (size_t index = 0; leg == 0 && parameters.getMaxInnerTimeOfTripBufferSeconds() > 0 && index < path.nodesRef.size(); index++)
      {
        auto access = nodesAccess.find(path.nodesRef[index].get().uid);
        if (access != nodesAccess.end())
        {
          maxDepartureTimes[index] = parameters.getTimeOfTrip() + access->second.time + parameters.getMaxInnerTimeOfTripBufferSeconds();
        }
      }
      auto trips = tripsByPath.find(&path);
      boardings[leg] = getLatestBoardings(path, trips == tripsByPath.end() ? noTrips : trips->second, latestArrivals, maxDepartureTimes, parameters.getMinWaitingTimeSeconds());
    }

    // Latest departure from the origin, with the same conditions as the reverse calculation
    const Path &firstPath = paths.front();
    std::optional<size_t> firstBoardingIndex;
    int departureTime = -1;
    for (size_t index = 0; index + 1 < firstPath.nodesRef.size(); index++)
    {
      auto access = nodesAccess.find(firstPath.nodesRef[index].get().uid);
      const LatestBoarding &boarding = boardings[0][index];
      if (access == nodesAccess.end() || boarding.trip == nullptr)
      {
        continue;
      }
      const int accessDepartureTime = boarding.readyTime - access->second.time;
      if (accessDepartureTime >= parameters.getTimeOfTrip() && accessDepartureTime > departureTime)
      {
        departureTime = accessDepartureTime;
        firstBoardingIndex = index;
      }
    }
    if (!firstBoardingIndex.has_value())
    {
      return std::nullopt;
    }

    const NodeTimeDistance &access = nodesAccess.at(firstPath.nodesRef[firstBoardingIndex.value()].get().uid);
    std::deque<JourneyStep> journey;
    journey.push_back(JourneyStep(std::nullopt, std::nullopt, std::nullopt, access.time, false, access.distance));
    size_t boardingIndex = firstBoardingIndex.value();
    for (size_t leg = 0; leg < legsCount; leg++)
    {
      const Path &path = paths[leg];
      const LatestBoarding &boarding = boardings[leg][boardingIndex];
      const Node &alightingNode = path.nodesRef[boarding.alightingIndex];
      const bool isLastLeg = leg + 1 == legsCount;
      const std::optional<Transfer> &transfer = transfers[leg][boarding.alightingIndex];
      if (!isLastLeg && !transfer.has_value())
      {
        return std::nullopt;
      }
      journey.push_back(JourneyStep(std::cref(boarding.trip->forwardConnections[boardingIndex].get()),
                                    std::cref(boarding.trip->forwardConnections[boarding.alightingIndex - 1].get()),
                                    std::cref(*boarding.trip),
                                    isLastLeg ? 0 : transfer->time,
                                    !isLastLeg && alightingNode == paths[leg + 1].get().nodesRef[transfer->boardingIndex].get(),
                                    isLastLeg ? 0 : transfer->distance));
      if (isLastLeg)
      {
        const NodeTimeDistance &egress = nodesEgress.at(alightingNode.uid);
        journey.push_back(JourneyStep(std::nullopt, std::nullopt, std::nullopt, egress.time, false, egress.distance));
      }
      else
      {
        boardingIndex = transfer->boardingIndex;
      }
    }
    return std::make_pair(journey, departureTime);
  }

}

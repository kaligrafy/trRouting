#include <algorithm>
#include <stdexcept>
#include <unordered_set>

#include "calculator.hpp"
#include "bounded_journeys_enumerator.hpp"
#include "latest_departure_journey.hpp"
#include "min_boardings.hpp"
#include "parameters.hpp"
#include "routing_result.hpp"
#include "connection_set.hpp"
#include "static_transit_network.hpp"
#include "path.hpp"
#include "point.hpp"

namespace TrRouting
{

  AlternativesResult Calculator::boundedAlternatives(const RouteParameters &parameters, const StaticTransitNetwork &network, size_t extraTransfers, size_t maxJourneys)
  {
    if (!parameters.isForwardCalculation())
    {
      throw std::invalid_argument("boundedAlternatives only supports a departure time");
    }
    std::unique_ptr<SingleCalculationResult> fastest = calculateSingle(parameters);
    const int maxTravelTime = parameters.getAlternativesMaxTravelTimeSeconds(fastest->totalTravelTime, fastest->departureTime);

    const Point *origin = parameters.getOrigin();
    AccessibilityParameters boundsParameters(std::make_unique<Point>(origin->latitude, origin->longitude),
      CommonParameters(parameters.getScenario(), parameters.getTimeOfTrip(), parameters.getMinWaitingTimeSeconds(), maxTravelTime,
        parameters.getMaxAccessWalkingTravelTimeSeconds(), parameters.getMaxEgressWalkingTravelTimeSeconds(),
        parameters.getMaxTransferWalkingTravelTimeSeconds(), parameters.getMaxInnerTimeOfTripBufferSeconds(), true));
    reset(boundsParameters, *parameters.getOrigin(), *parameters.getDestination(), false);

    // The forward calculation marks the trips usable from the origin within the maximum travel time
    std::unordered_map<Node::uid_t, JourneyStep> forwardEgressJourneysSteps;
    forwardCalculationAllNodes(boundsParameters, forwardEgressJourneysSteps);

    // The reverse calculation from the maximum arrival time gives the latest arrival at each stop to still reach the destination
    arrivalTimeSeconds = parameters.getTimeOfTrip() + maxTravelTime;
    for (const NodeTimeDistance &egressFootpath : egressFootpaths)
    {
      nodesReverseTentativeTime[egressFootpath.node.uid] = arrivalTimeSeconds - nodesEgress.at(egressFootpath.node.uid).time;
    }
    std::unordered_map<Node::uid_t, JourneyStep> reverseAccessJourneysSteps;
    reverseCalculationAllNodes(boundsParameters, reverseAccessJourneysSteps);

    TripsByPath tripsByPath;
    for (const Trip &trip : connectionSet->getTrips())
    {
      const TripQueryData &tripQueryData = tripsQueryOverlay[trip.uid];
      if (tripQueryData.usable && !tripQueryData.disabled && trip.forwardConnections.size() + 1 == trip.path.nodesRef.size())
      {
        tripsByPath[&trip.path].push_back(&trip);
      }
    }
    for (auto &[path, trips] : tripsByPath)
    {
      std::sort(trips.begin(), trips.end(), [](const Trip *trip, const Trip *other) { return trip->forwardConnections[0].get().getDepartureTime() < other->forwardConnections[0].get().getDepartureTime(); });
    }

    // The fastest journey bounds the minimum number of paths. Only the stops with a latest arrival can reach the destination in time.
    const size_t maxPaths = fastest->numberOfTransfers + 1 + extraTransfers;
    std::unordered_set<Node::uid_t> egressUids;
    for (const auto &[uid, egress] : nodesEgress)
    {
      if (network.getNodes().count(uid) > 0)
      {
        egressUids.insert(uid);
      }
    }
    const BoardingsByNode minBoardings = getMinBoardings(network, egressUids,
      [maxPaths](int round, const BoardingsByNode &) { return static_cast<size_t>(round) <= maxPaths; },
      [this](Node::uid_t uid) { return nodesReverseTentativeTime[uid] >= 0; });

    BoundedJourneysEnumerator enumerator(network, tripsByPath, nodesReverseTentativeTime, nodesEgress, minBoardings, boundsParameters, arrivalTimeSeconds, maxJourneys);
    AlternativesResult alternatives;
    for (const PathsJourney &pathsJourney : enumerator.enumerate(nodesAccess, maxPaths, extraTransfers))
    {
      auto journey = getLatestDepartureJourney(pathsJourney, tripsByPath, nodesAccess, nodesEgress, boundsParameters);
      if (!journey.has_value())
      {
        continue;
      }
      // A journey losing steps to optimization has a superfluous path: its shorter sequence is enumerated separately, with its own best times
      const size_t stepsCount = journey->first.size();
      optimizeJourney(journey->first);
      if (journey->first.size() == stepsCount)
      {
        alternatives.alternatives.push_back(getJourneyResult(parameters, journey->first, journey->second));
      }
    }
    alternatives.totalAlternativesCalculated = alternatives.alternatives.size();
    return alternatives;
  }

}

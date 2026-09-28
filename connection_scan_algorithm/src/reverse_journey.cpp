#include "calculator.hpp"
#include "constants.hpp"
#include "parameters.hpp"
#include "node.hpp"
#include "trip.hpp"
#include "line.hpp"
#include "mode.hpp"
#include "path.hpp"
#include "routing_result.hpp"
#include "agency.hpp"
#include "transit_data.hpp"
#include "spdlog/spdlog.h"

namespace TrRouting
{
  std::unique_ptr<SingleCalculationResult> Calculator::reverseJourneyStep(const RouteParameters &parameters, int bestDepartureTime, std::optional<std::reference_wrapper<const Node>> bestAccessNode, const std::unordered_map<Node::uid_t, JourneyStep> & reverseAccessJourneysSteps)
  {
    std::unique_ptr<SingleCalculationResult> singleResult = std::make_unique<SingleCalculationResult>();

    if (!bestAccessNode.has_value()) // route node found
    {
      throw NoRoutingFoundException(NoRoutingReason::NO_ROUTING_FOUND);
    }

    const Node & resultingNode = bestAccessNode.value().get();
    if (reverseAccessJourneysSteps.count(resultingNode.uid) > 0) { // ignore nodes with no line

      std::deque<JourneyStep> journey;

      // recreate journey:
      JourneyStep resultingNodeJourneyStep = reverseAccessJourneysSteps.at(resultingNode.uid);

      std::optional<std::reference_wrapper<const Node>> bestEgressNode;
      while (resultingNodeJourneyStep.hasConnections())
        {
          // here we inverse the transfers (putting them after the exit connection, instead of before the enter connection):
          if (journey.size() > 0)
            {
              journey[journey.size()-1].copyTransferTimeDistance(resultingNodeJourneyStep);
            }
          journey.push_back(resultingNodeJourneyStep);
          auto exitConnectionOptRef = resultingNodeJourneyStep.getFinalExitConnection();
          bestEgressNode = exitConnectionOptRef.value().get().getArrivalNode();
          resultingNodeJourneyStep = reverseJourneysSteps.at(bestEgressNode.value().get().uid);
        }

      journey.push_front(JourneyStep(std::nullopt,
                                     std::nullopt,
                                     std::nullopt,
                                     nodesAccess.at(resultingNode.uid).time,
                                     false,
                                     nodesAccess.at(resultingNode.uid).distance));

      journey.push_back(JourneyStep(std::nullopt,
                                    std::nullopt,
                                    std::nullopt,
                                    nodesEgress.at(bestEgressNode.value().get().uid).time,
                                    false,
                                    nodesEgress.at(bestEgressNode.value().get().uid).distance));

      std::vector<int> optimizeCases = optimizeJourney(journey);

      spdlog::debug("-- {} optimization case used {} ", optimizeCases.size(), optimizeCasesToString(optimizeCases) );

      singleResult = getJourneyResult(parameters, journey, bestDepartureTime);
    }

    return singleResult;
  }

  std::unique_ptr<AllNodesResult> Calculator::reverseJourneyStepAllNodes(const AccessibilityParameters &parameters, const std::unordered_map<Node::uid_t, JourneyStep> & reverseAccessJourneysSteps)
  {
    std::unique_ptr<AllNodesResult> allNodesResult = std::make_unique<AllNodesResult>();

    int              reachableNodesCount  {0};

    int nodesCount     = transitData.getNodes().size();

    for(auto nodeIte = transitData.getNodes().begin(); nodeIte != transitData.getNodes().end(); nodeIte++) {
      const Node & resultingNode = nodeIte->second;

      std::deque<JourneyStep> journey;

      // recreate journey:
      if (reverseAccessJourneysSteps.count(resultingNode.uid) == 0) { // ignore nodes with no line
        continue;
      }
      JourneyStep resultingNodeJourneyStep = reverseAccessJourneysSteps.at(resultingNode.uid);

      std::optional<std::reference_wrapper<const Node>> bestEgressNode;
      while (resultingNodeJourneyStep.hasConnections()) {
        // here we inverse the transfers (putting them after the exit connection, instead of before the enter connection):
        if (journey.size() > 0)
        {
          journey[journey.size()-1].copyTransferTimeDistance(resultingNodeJourneyStep);
        }
        journey.push_back(resultingNodeJourneyStep);
        auto exitConnectionOptRef = resultingNodeJourneyStep.getFinalExitConnection();
        bestEgressNode = exitConnectionOptRef.value().get().getArrivalNode();
        resultingNodeJourneyStep = reverseJourneysSteps.at(bestEgressNode.value().get().uid);
      }

      journey.push_back(JourneyStep(std::nullopt,
                                    std::nullopt,
                                    std::nullopt,
                                    nodesEgress.at(bestEgressNode.value().get().uid).time,
                                    false,
                                    nodesEgress.at(bestEgressNode.value().get().uid).distance));


      std::vector<int> optimizeCases = optimizeJourney(journey);

      spdlog::debug("-- {} optimization case used {} ", optimizeCases.size(), optimizeCasesToString(optimizeCases) );

      int numberOfTransfers    {-1};
      for (auto & journeyStep : journey) {
        // check if it is an in-vehicle journey:
        if (journeyStep.hasConnections())
        {
          auto tripOptRef = journeyStep.getFinalTrip();
          const Trip &journeyStepTrip = tripOptRef.value().get();

          if (!journeyStepTrip.line.mode.isTransferable()) {
            numberOfTransfers += 1;
          }
        }
      }

      if (reverseAccessJourneysSteps.at(resultingNode.uid).getFinalEnterConnection().has_value()) {
        auto enterConnectionOptRef = reverseAccessJourneysSteps.at(resultingNode.uid).getFinalEnterConnection();
        //TODO Should check that MinWaiting is not -1 or use OrDefault(0)
        //TODO Could move this operation into a function of class Connection
        int departureTimeD = enterConnectionOptRef.value().get().getDepartureTime() - enterConnectionOptRef.value().get().getMinWaitingTime();
        if (arrivalTimeSeconds - departureTimeD <=  parameters.getMaxTotalTravelTimeSeconds()) {
          reachableNodesCount++;

          AccessibleNodes node = AccessibleNodes(
                                                 resultingNode,
                                                 arrivalTimeSeconds,
                                                 arrivalTimeSeconds - departureTimeD,
                                                 numberOfTransfers
                                                 );
          allNodesResult.get()->nodes.push_back(node);
        }
      }
    }

    allNodesResult.get()->numberOfReachableNodes = reachableNodesCount;
    allNodesResult.get()->totalNodeCount = nodesCount;
    return allNodesResult;
  }
}

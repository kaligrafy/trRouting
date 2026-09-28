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

namespace TrRouting
{
  std::unique_ptr<SingleCalculationResult> Calculator::getJourneyResult(const RouteParameters &parameters, const std::deque<JourneyStep> &journey, int bestDepartureTime)
  {
    std::unique_ptr<SingleCalculationResult> singleResult = std::make_unique<SingleCalculationResult>();

    std::vector<Leg> legs;

    int totalInVehicleTime       { 0}; int transferArrivalTime    {-1};
    int totalWalkingTime         { 0}; int transferReadyTime      {-1}; int numberOfTransfers    {-1};
    int totalWaitingTime         { 0}; int departureTime          {-1}; int boardingSequence     {-1};
    int totalTransferWalkingTime { 0}; int arrivalTime            {-1}; int unboardingSequence   {-1};
    int totalTransferWaitingTime { 0}; int inVehicleTime          {-1};
    int totalDistance            { 0}; int distance               {-1};
    int inVehicleDistance        {-1}; int totalInVehicleDistance { 0}; int totalWalkingDistance { 0};
    int totalTransferDistance    {-1}; int accessDistance         { 0}; int egressDistance       { 0};
    int accessWalkingTime      {-1};
    int transferTime             {-1}; int egressWalkingTime      {-1};
    int waitingTime              {-1}; int accessWaitingTime      {-1};

    size_t journeyStepsCount = journey.size();
    size_t i = 0;
    for (auto & journeyStep : journey)
      {

        // check if it is an in-vehicle journey:
        if (journeyStep.hasConnections())
          {
            auto enterConnectionOptRef = journeyStep.getFinalEnterConnection();
            auto exitConnectionOptRef = journeyStep.getFinalExitConnection();
            auto tripOptRef = journeyStep.getFinalTrip();
            
            const Connection &journeyStepEnterConnection = enterConnectionOptRef.value().get();
            const Connection &journeyStepExitConnection = exitConnectionOptRef.value().get();
            const Node &journeyStepNodeDeparture = journeyStepEnterConnection.getDepartureNode();
            const Node &journeyStepNodeArrival = journeyStepExitConnection.getArrivalNode();
            const Trip &journeyStepTrip = tripOptRef.value().get();
            
            transferTime                = journeyStep.getTransferTravelTime();
            distance                    = journeyStep.getTransferDistance();
            inVehicleDistance           = 0;
            departureTime               = journeyStepEnterConnection.getDepartureTime();
            arrivalTime                 = journeyStepExitConnection.getArrivalTime();
            boardingSequence            = journeyStepEnterConnection.getSequenceInTrip();
            unboardingSequence          = journeyStepExitConnection.getSequenceInTrip();
            inVehicleTime               = arrivalTime   - departureTime;
            waitingTime                 = departureTime - transferArrivalTime;
            transferArrivalTime         = arrivalTime   + transferTime;
            transferReadyTime           = transferArrivalTime;

            if (journey.size() > i + 1 && journey[i+1].getFinalEnterConnection().has_value())
              {
                auto nextEnterConnectionOptRef = journey[i+1].getFinalEnterConnection();
                transferReadyTime += nextEnterConnectionOptRef.value().get().getMinWaitingTimeOrDefault(parameters.getMinWaitingTimeSeconds());
              }

            totalInVehicleTime         += inVehicleTime;
            totalWaitingTime           += waitingTime;
            if (!journeyStepTrip.line.mode.isTransferable())
              {
                numberOfTransfers += 1;
              }

            legs.push_back(std::make_tuple(std::ref(journeyStepTrip), boardingSequence - 1, unboardingSequence - 1));

            if (unboardingSequence > 1 && (static_cast<size_t>(unboardingSequence - 1) < journeyStepTrip.path.segmentsDistanceMeters.size())) // check if distances are available for this path
              {
                for (int seqI = boardingSequence - 1; seqI < unboardingSequence; seqI++)
                  {
                    inVehicleDistance += journeyStepTrip.path.segmentsDistanceMeters[seqI];
                  }
                totalDistance += inVehicleDistance;
                if (journeyStepTrip.line.mode.isTransferable())
                  {
                    totalWalkingDistance     += inVehicleDistance;
                    totalWalkingTime         += inVehicleTime;
                    totalTransferDistance    += inVehicleDistance;
                    totalTransferWalkingTime += inVehicleTime;
                  }
                else
                  {
                    totalInVehicleDistance += inVehicleDistance;
                  }
              }
            else
              {
                inVehicleDistance      = -1;
                totalDistance          = -1;
                totalInVehicleDistance = -1;
              }

            if (i == 1) // first leg
              {
                accessWaitingTime   = waitingTime;
              }
            else
              {
                totalTransferWaitingTime += waitingTime;
              }

            singleResult.get()->steps.push_back(std::make_unique<BoardingStep>(
                                                                               journeyStepTrip,
                                                                               boardingSequence,
                                                                               boardingSequence,
                                                                               journeyStepNodeDeparture,
                                                                               departureTime,
                                                                               waitingTime
                                                                               ));

            singleResult.get()->steps.push_back(std::make_unique<UnboardingStep>(
                                                                                 journeyStepTrip,
                                                                                 unboardingSequence,
                                                                                 unboardingSequence + 1,
                                                                                 journeyStepNodeArrival,
                                                                                 arrivalTime,
                                                                                 inVehicleTime,
                                                                                 inVehicleDistance
                                                                                 ));
            if (i < journeyStepsCount - 2) // if not the last transit leg
              {
                totalTransferWalkingTime += transferTime;
                totalWalkingTime         += transferTime;
                if (totalDistance != -1)
                  {
                    totalDistance += distance;
                  }
                totalWalkingDistance     += distance;
                totalTransferDistance    += distance;
                singleResult.get()->steps.push_back(std::make_unique<WalkingStep>(
                                                                                  walking_step_type::TRANSFER,
                                                                                  transferTime,
                                                                                  distance,
                                                                                  arrivalTime,
                                                                                  transferArrivalTime,
                                                                                  transferReadyTime
                                                                                  ));
              }
          }
        else // access or egress journey step
          {

            transferTime          = journeyStep.getTransferTravelTime();
            distance              = journeyStep.getTransferDistance();
            if (totalDistance != -1)
              {
                totalDistance += distance;
              }
            totalWalkingDistance += distance;
            if (i == 0) // access
              {

                transferArrivalTime  = bestDepartureTime + transferTime;
                transferReadyTime    = transferArrivalTime;

                if (journey.size() > i + 1 && journey[i+1].getFinalEnterConnection().has_value())
                  {
                    auto nextEnterConnectionOptRef = journey[i+1].getFinalEnterConnection();
                    transferReadyTime += nextEnterConnectionOptRef.value().get().getMinWaitingTimeOrDefault(parameters.getMinWaitingTimeSeconds());
                  }

                totalWalkingTime    += transferTime;
                accessWalkingTime    = transferTime;
                accessDistance       = distance;
                singleResult.get()->steps.push_back(std::make_unique<WalkingStep>(
                                                                                  walking_step_type::ACCESS,
                                                                                  transferTime,
                                                                                  distance,
                                                                                  bestDepartureTime,
                                                                                  transferArrivalTime,
                                                                                  transferReadyTime
                                                                                  ));
              }
            else // egress
              {
                totalWalkingTime   += transferTime;
                egressWalkingTime   = transferTime;
                transferArrivalTime = arrivalTime + transferTime;
                egressDistance      = distance;
                singleResult.get()->steps.push_back(std::make_unique<WalkingStep>(
                                                                                  walking_step_type::EGRESS,
                                                                                  transferTime,
                                                                                  distance,
                                                                                  arrivalTime,
                                                                                  arrivalTime + transferTime
                                                                                  ));
                arrivalTime = transferArrivalTime;
              }
          }
        i++;
      }

    singleResult.get()->departureTime = bestDepartureTime;
    singleResult.get()->arrivalTime = arrivalTime;
    singleResult.get()->totalTravelTime = arrivalTime - bestDepartureTime;
    singleResult.get()->totalDistance = totalDistance;
    singleResult.get()->totalInVehicleTime = totalInVehicleTime;
    singleResult.get()->totalInVehicleDistance = totalInVehicleDistance;
    singleResult.get()->totalNonTransitTravelTime = totalWalkingTime;
    singleResult.get()->totalNonTransitDistance = totalWalkingDistance;
    singleResult.get()->numberOfBoardings = numberOfTransfers + 1;
    singleResult.get()->numberOfTransfers = numberOfTransfers == -1 ? 0 : numberOfTransfers;
    singleResult.get()->transferWalkingTime = totalTransferWalkingTime;
    singleResult.get()->transferWalkingDistance = totalTransferDistance;
    singleResult.get()->accessTravelTime = accessWalkingTime;
    singleResult.get()->accessDistance = accessDistance;
    singleResult.get()->egressTravelTime = egressWalkingTime;
    singleResult.get()->egressDistance = egressDistance;
    singleResult.get()->transferWaitingTime = totalTransferWaitingTime;
    singleResult.get()->firstWaitingTime = accessWaitingTime;
    singleResult.get()->totalWaitingTime = totalWaitingTime;
    singleResult.get()->legs = legs;

    return singleResult;
  }
}

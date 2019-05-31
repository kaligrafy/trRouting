#include "calculator.hpp"

namespace TrRouting
{
    
  std::string Calculator::alternativesRouting()
  {

    RoutingResult  routingResult;
    std::string    response;
    nlohmann::json json;
    nlohmann::json alternativeJson;
    
    if (odTrip != nullptr) // if odTrip is provided
    {
      json["odTripUuid"] = boost::uuids::to_string(odTrip->uuid);
    }

    json["alternatives"] = nlohmann::json::array();

    std::vector<int>                 foundLinesIdx;
    std::vector< std::vector<int> >  allCombinations;
    std::map<std::vector<int>, bool> alreadyCalculatedCombinations;
    std::map<std::vector<int>, bool> alreadyFoundLinesIdx;
    std::map<std::vector<int>, int>  foundLinesIdxTravelTimeSeconds;
    std::vector<int>                 combinationsKs;
    int maxTravelTime;
    int alternativeSequence = 1;
    int maxAlternatives = params.maxAlternatives;
    int lastFoundedAtNum = 0;
    int departureTimeSeconds = -1;
    std::vector<std::string> lineShortnames;
    
    if (params.debugDisplay)
    {
      std::cout << "alternatives parameters:" << std::endl;
      std::cout << "  maxTotalTravelTimeSeconds: " << params.maxTotalTravelTimeSeconds << std::endl;
      std::cout << "  minAlternativeMaxTravelTimeSeconds: " << params.minAlternativeMaxTravelTimeSeconds << std::endl;
      std::cout << "  alternativesMaxAddedTravelTimeSeconds: " << params.alternativesMaxAddedTravelTimeSeconds << std::endl;
      std::cout << "  alternativesMaxTravelTimeRatio: " << params.alternativesMaxTravelTimeRatio << std::endl;
      std::cout << "  maxTotalTravelTimeSeconds: " << params.maxTotalTravelTimeSeconds << std::endl;
      std::cout << "calculating fastest alternative..." << std::endl;
    }
    
    routingResult = calculate();

    if (routingResult.status == "success")
    {
      lineShortnames.clear();
      for(auto lineIdx : routingResult.linesIdx)
      {
        lineShortnames.push_back(lines[lineIdx].shortname);
      }
      
      alternativeJson = {};
      alternativeJson["status"]                        = routingResult.status;
      alternativeJson["travelTimeSeconds"]             = routingResult.travelTimeSeconds;
      alternativeJson["minimizedTravelTimeSeconds"]    = routingResult.travelTimeSeconds - routingResult.firstWaitingTimeSeconds + params.minWaitingTimeSeconds;
      alternativeJson["departureTimeSeconds"]          = routingResult.departureTimeSeconds;
      alternativeJson["minimizedDepartureTimeSeconds"] = routingResult.minimizedDepartureTimeSeconds;
      alternativeJson["arrivalTimeSeconds"]            = routingResult.arrivalTimeSeconds;
      alternativeJson["numberOfTransfers"]             = routingResult.numberOfTransfers;
      alternativeJson["inVehicleTravelTimeSeconds"]    = routingResult.inVehicleTravelTimeSeconds;
      alternativeJson["transferTravelTimeSeconds"]     = routingResult.transferTravelTimeSeconds;
      alternativeJson["waitingTimeSeconds"]            = routingResult.waitingTimeSeconds;
      alternativeJson["accessTravelTimeSeconds"]       = routingResult.accessTravelTimeSeconds;
      alternativeJson["egressTravelTimeSeconds"]       = routingResult.egressTravelTimeSeconds;
      alternativeJson["transferWaitingTimeSeconds"]    = routingResult.transferWaitingTimeSeconds;
      alternativeJson["firstWaitingTimeSeconds"]       = routingResult.firstWaitingTimeSeconds;
      alternativeJson["nonTransitTravelTimeSeconds"]   = routingResult.nonTransitTravelTimeSeconds;
      alternativeJson["inVehicleTravelTimesSeconds"]   = routingResult.inVehicleTravelTimesSeconds;
      alternativeJson["lineUuids"]                     = routingResult.lineUuids;
      alternativeJson["lineShortnames"]                = lineShortnames;
      alternativeJson["modeShortnames"]                = routingResult.modeShortnames;
      alternativeJson["agencyUuids"]                   = routingResult.agencyUuids;
      alternativeJson["boardingNodeUuids"]             = routingResult.boardingNodeUuids;
      alternativeJson["unboardingNodeUuids"]           = routingResult.unboardingNodeUuids;
      alternativeJson["tripUuids"]                     = routingResult.tripUuids;
      alternativeJson["alternativeSequence"]           = alternativeSequence;

      alternativeSequence += 1;

      json["alternatives"].push_back(alternativeJson);
      json["status"] = "success";
      departureTimeSeconds = routingResult.departureTimeSeconds + routingResult.firstWaitingTimeSeconds - params.minWaitingTimeSeconds;
            
      maxTravelTime = params.alternativesMaxTravelTimeRatio * routingResult.travelTimeSeconds;
      if (maxTravelTime < params.minAlternativeMaxTravelTimeSeconds)
      {
        params.maxTotalTravelTimeSeconds = params.minAlternativeMaxTravelTimeSeconds;
      }
      else if (maxTravelTime > routingResult.travelTimeSeconds + params.alternativesMaxAddedTravelTimeSeconds)
      {
        params.maxTotalTravelTimeSeconds = routingResult.travelTimeSeconds + params.alternativesMaxAddedTravelTimeSeconds;
      }
      else
      {
        params.maxTotalTravelTimeSeconds = maxTravelTime;
      }
      
      params.departureTimeSeconds = departureTimeSeconds;
      
      if (params.debugDisplay)
        std::cout << "  fastestTravelTimeSeconds: " << routingResult.travelTimeSeconds << std::endl;
      
      foundLinesIdx = routingResult.linesIdx;
      std::stable_sort(foundLinesIdx.begin(),foundLinesIdx.end());
      alreadyFoundLinesIdx[foundLinesIdx]           = true;
      foundLinesIdxTravelTimeSeconds[foundLinesIdx] = routingResult.travelTimeSeconds;
      lastFoundedAtNum = 1;
      //std::cout << "fastest line ids: ";
      //for (auto lineUuid : foundLineUuids)
      //{
      //  std::cout << lines[lineIndexesByUuid[lineUuid]].shortname << " ";
      //}
      //std::cout << std::endl;
      combinationsKs.clear();
      for (int i = 1; i <= foundLinesIdx.size(); i++) { combinationsKs.push_back(i); }
      for (auto k : combinationsKs)
      {
        Combinations<int> combinations(foundLinesIdx, k);
        //std::cout << "\nk = " << k << std::endl;
        for (auto newCombination : combinations)
        {
          std::stable_sort(newCombination.begin(), newCombination.end());
          allCombinations.push_back(newCombination);
          alreadyCalculatedCombinations[newCombination] = true;
        }
      }
      
      std::vector<int> combination;
      for (int i = 0; i < allCombinations.size(); i++)
      {
        if (alternativeSequence <= maxAlternatives)
        {

          combination = allCombinations.at(i);
          params.exceptLinesIdx = combination;

          if (params.debugDisplay)
          {
            std::cout << "calculating alternative " << alternativeSequence << "..." << std::endl;
          }

          //std::cout << "except line Uuids: ";
          //for (auto lineUuid : combination)
          //{
          //  std::cout << lines[lineIndexesByUuid[lineUuid]].shortname << " ";
          //}
          //std::cout << std::endl;
          routingResult = calculate(false, false);
          
          if (routingResult.status == "success")
          {
          
            foundLinesIdx = routingResult.linesIdx;
            std::stable_sort(foundLinesIdx.begin(),foundLinesIdx.end());
            
            if (foundLinesIdx.size() > 0 && alreadyFoundLinesIdx.count(foundLinesIdx) == 0)
            {
              lineShortnames.clear();
              for(auto lineIdx : routingResult.linesIdx)
              {
                lineShortnames.push_back(lines[lineIdx].shortname);
              }
              
              alternativeJson = {};
              alternativeJson["status"]                        = routingResult.status;
              alternativeJson["travelTimeSeconds"]             = routingResult.travelTimeSeconds;
              alternativeJson["minimizedTravelTimeSeconds"]    = routingResult.travelTimeSeconds - routingResult.firstWaitingTimeSeconds + params.minWaitingTimeSeconds;
              alternativeJson["departureTimeSeconds"]          = routingResult.departureTimeSeconds;
              alternativeJson["minimizedDepartureTimeSeconds"] = routingResult.minimizedDepartureTimeSeconds;
              alternativeJson["arrivalTimeSeconds"]            = routingResult.arrivalTimeSeconds;
              alternativeJson["numberOfTransfers"]             = routingResult.numberOfTransfers;
              alternativeJson["inVehicleTravelTimeSeconds"]    = routingResult.inVehicleTravelTimeSeconds;
              alternativeJson["transferTravelTimeSeconds"]     = routingResult.transferTravelTimeSeconds;
              alternativeJson["waitingTimeSeconds"]            = routingResult.waitingTimeSeconds;
              alternativeJson["accessTravelTimeSeconds"]       = routingResult.accessTravelTimeSeconds;
              alternativeJson["egressTravelTimeSeconds"]       = routingResult.egressTravelTimeSeconds;
              alternativeJson["transferWaitingTimeSeconds"]    = routingResult.transferWaitingTimeSeconds;
              alternativeJson["firstWaitingTimeSeconds"]       = routingResult.firstWaitingTimeSeconds;
              alternativeJson["nonTransitTravelTimeSeconds"]   = routingResult.nonTransitTravelTimeSeconds;
              alternativeJson["inVehicleTravelTimesSeconds"]   = routingResult.inVehicleTravelTimesSeconds;
              alternativeJson["lineUuids"]                     = routingResult.lineUuids;
              alternativeJson["lineShortnames"]                = lineShortnames;
              alternativeJson["modeShortnames"]                = routingResult.modeShortnames;
              alternativeJson["agencyUuids"]                   = routingResult.agencyUuids;
              alternativeJson["boardingNodeUuids"]             = routingResult.boardingNodeUuids;
              alternativeJson["unboardingNodeUuids"]           = routingResult.unboardingNodeUuids;
              alternativeJson["tripUuids"]                     = routingResult.tripUuids;
              alternativeJson["alternativeSequence"]           = alternativeSequence;
              json["alternatives"].push_back(alternativeJson);
            
              //std::cout << "travelTimeSeconds: " << routingResult.travelTimeSeconds << " line Uuids: ";
              //for (auto lineUuid : foundLineUuids)
              //{
              //  std::cout << lines[lineIndexesByUuid[lineUuid]].shortname << " ";
              //}
              //std::cout << std::endl;
              combinationsKs.clear();
              
              lastFoundedAtNum = alternativeSequence;
              alreadyFoundLinesIdx[foundLinesIdx] = true;
              foundLinesIdxTravelTimeSeconds[foundLinesIdx] = routingResult.travelTimeSeconds;
              for (int i = 1; i <= foundLinesIdx.size(); i++) { combinationsKs.push_back(i); }
              for (auto k : combinationsKs)
              {
                Combinations<int> combinations(foundLinesIdx, k);
                //std::cout << "\nk = " << k << std::endl;
                for (auto newCombination : combinations)
                {
                  newCombination.insert( newCombination.end(), combination.begin(), combination.end() );
                  std::stable_sort(newCombination.begin(), newCombination.end());
                  if (alreadyCalculatedCombinations.count(newCombination) == 0)
                  {
                    allCombinations.push_back(newCombination);
                    alreadyCalculatedCombinations[newCombination] = true;
                  }
                }
              }
              
              combination.clear();

              alternativeSequence += 1;

            }
          }
        }
      }

      std::cout << std::endl;
      int i {1};
      //for (auto foundLineUuids : alreadyFoundLineUuids)
      //{
      //  std::cout << i << ". ";
      //  for(auto lineUuid : foundLineUuids.first)
      //  {
      //    std::cout << lines[lineIndexesByUuid[lineUuid]].shortname << " ";
      //  }
      //  std::cout << " tt: " << (foundLineUuidsTravelTimeSeconds[foundLineUuids.first] / 60);
      //  i++;
      //  std::cout << std::endl;
      //}
      
      std::cout << "last alternative found at: " << lastFoundedAtNum << " on a total of " << maxAlternatives << " calculations" << std::endl;
      
    }
    else
    {
      json["status"] = "failed";
    }

    response = json.dump(2);

    return response;

  }

  

}
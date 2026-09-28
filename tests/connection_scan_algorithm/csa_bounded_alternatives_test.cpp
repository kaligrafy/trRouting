#include <boost/uuid/uuid.hpp>

#include "gtest/gtest.h"
#include "calculator.hpp"
#include "csa_route_calculation_test.hpp"
#include "routing_result.hpp"
#include "scenario_paths.hpp"
#include "static_transit_network.hpp"
#include "path.hpp"
#include "trip.hpp"

using PathUuidsJourneys = std::vector<std::vector<boost::uuids::uuid>>;

// Uuids of the paths of each alternative, in order
static PathUuidsJourneys toPathUuids(const TrRouting::AlternativesResult &result)
{
  PathUuidsJourneys journeys;
  for (const auto &alternative : result.alternatives)
  {
    journeys.emplace_back();
    for (const auto &step : alternative->steps)
    {
      if (auto boarding = dynamic_cast<const TrRouting::BoardingStep *>(step.get()))
      {
        journeys.back().push_back(boarding->trip.path.uuid);
      }
    }
  }
  return journeys;
}

// Parameters: origin, destination, minimum waiting time, extra transfers, expected paths of the alternatives
class BoundedAlternativesTest : public SingleRouteCalculationFixtureTests,
                                public ::testing::WithParamInterface<std::tuple<TrRouting::Point, TrRouting::Point, int, size_t, PathUuidsJourneys>>
{
protected:
  TrRouting::RouteParameters getParameters(const TrRouting::Point &origin, const TrRouting::Point &destination, int minWaitingTime = DEFAULT_MIN_WAITING_TIME, bool forward = true)
  {
    return TrRouting::RouteParameters(std::make_unique<TrRouting::Point>(origin.latitude, origin.longitude),
                                      std::make_unique<TrRouting::Point>(destination.latitude, destination.longitude),
                                      transitData.getScenarios().at(TestDataFetcher::scenarioUuid), getTimeInSeconds(9, 45), minWaitingTime,
                                      DEFAULT_MAX_TOTAL_TIME, DEFAULT_MAX_ACCESS_TRAVEL_TIME, DEFAULT_MAX_EGRESS_TRAVEL_TIME, DEFAULT_MAX_TRANSFER_TRAVEL_TIME,
                                      DEFAULT_MAX_INNER_TIME_OF_TRIP_BUFFER, true, forward);
  }

  TrRouting::StaticTransitNetwork getNetwork()
  {
    return TrRouting::StaticTransitNetwork(TrRouting::getScenarioPaths(transitData, transitData.getScenarios().at(TestDataFetcher::scenarioUuid)));
  }
};

TEST_P(BoundedAlternativesTest, AlternativesOfOd)
{
  auto [origin, destination, minWaitingTime, extraTransfers, expectedJourneys] = GetParam();
  TrRouting::RouteParameters parameters = getParameters(origin, destination, minWaitingTime);
  TrRouting::Calculator calculator(transitData, geoFilter);

  TrRouting::AlternativesResult result = calculator.boundedAlternatives(parameters, getNetwork(), extraTransfers);
  const PathUuidsJourneys journeys = toPathUuids(result);
  EXPECT_EQ(expectedJourneys, journeys);
  EXPECT_EQ(expectedJourneys.size(), static_cast<size_t>(result.totalAlternativesCalculated));

  // The journeys of the sequences also found by alternativesRouting have the same times
  TrRouting::AlternativesResult current = calculator.alternativesRouting(parameters);
  const PathUuidsJourneys currentJourneys = toPathUuids(current);
  for (size_t i = 0; i < result.alternatives.size(); i++)
  {
    auto currentJourney = std::find(currentJourneys.begin(), currentJourneys.end(), journeys[i]);
    if (currentJourney != currentJourneys.end())
    {
      const TrRouting::SingleCalculationResult &currentResult = *current.alternatives[currentJourney - currentJourneys.begin()];
      EXPECT_EQ(currentResult.departureTime, result.alternatives[i]->departureTime);
      EXPECT_EQ(currentResult.arrivalTime, result.alternatives[i]->arrivalTime);
      EXPECT_EQ(currentResult.numberOfTransfers, result.alternatives[i]->numberOfTransfers);
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
  BoundedAlternatives, BoundedAlternativesTest,
  ::testing::Values(
    // South/North to North1 and walking, then with a walking transfer from MidNode to the Extra path, as in alternativesRouting
    std::make_tuple(TrRouting::Point(45.5242, -73.5817), TrRouting::Point(45.5541, -73.6186), 180, 2, PathUuidsJourneys({
      {TestDataFetcher::pathSNUuid}, {TestDataFetcher::pathSNUuid, TestDataFetcher::pathExtraUuid}})),
    // Without extra transfers, only the journeys with the fewest transfers, even if the fastest journey has one more
    std::make_tuple(TrRouting::Point(45.5242, -73.5817), TrRouting::Point(45.5541, -73.6186), 180, 0, PathUuidsJourneys({{TestDataFetcher::pathSNUuid}})),
    // The transfer at MidNode from South/North to East/West is missed with a minimum waiting time of 3 minutes, but not of 90 seconds
    std::make_tuple(TrRouting::Point(45.5242, -73.5817), TrRouting::Point(45.5295, -73.624), 180, 2, PathUuidsJourneys({{TestDataFetcher::pathSNUuid}})),
    std::make_tuple(TrRouting::Point(45.5242, -73.5817), TrRouting::Point(45.5295, -73.624), 90, 2, PathUuidsJourneys({
      {TestDataFetcher::pathSNUuid}, {TestDataFetcher::pathSNUuid, TestDataFetcher::pathEWUuid}}))
  )
);

TEST_F(BoundedAlternativesTest, NoRouting)
{
  TrRouting::RouteParameters parameters = getParameters(TrRouting::Point(45.7242, -73.7817), TrRouting::Point(45.7541, -73.8186));
  TrRouting::Calculator calculator(transitData, geoFilter);

  EXPECT_THROW(calculator.boundedAlternatives(parameters, getNetwork()), TrRouting::NoRoutingFoundException);
}

TEST_F(BoundedAlternativesTest, ArrivalTimeNotSupported)
{
  TrRouting::RouteParameters parameters = getParameters(TrRouting::Point(45.5242, -73.5817), TrRouting::Point(45.5541, -73.6186), DEFAULT_MIN_WAITING_TIME, false);
  TrRouting::Calculator calculator(transitData, geoFilter);

  EXPECT_THROW(calculator.boundedAlternatives(parameters, getNetwork()), std::invalid_argument);
}

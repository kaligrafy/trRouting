#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "scenario_network_benchmark.hpp"
#include "calculator.hpp"
#include "euclideangeofilter.hpp"
#include "parameters.hpp"
#include "point.hpp"
#include "path.hpp"
#include "trip.hpp"
#include "random_nodes.hpp"
#include "routing_result.hpp"

using PathsSequence = std::vector<boost::uuids::uuid>;

// Uuids of the paths of an alternative, in order
static PathsSequence getPathsSequence(const TrRouting::SingleCalculationResult &alternative)
{
  PathsSequence sequence;
  for (const auto &step : alternative.steps)
  {
    if (auto boarding = dynamic_cast<const TrRouting::BoardingStep *>(step.get()))
    {
      sequence.push_back(boarding->trip.path.uuid);
    }
  }
  return sequence;
}

// Value of an environment variable, or its default value if not set
static size_t getEnvOrDefault(const char *name, size_t defaultValue)
{
  const char *value = std::getenv(name);
  return value == nullptr ? defaultValue : std::stoul(value);
}

class BoundedAlternativesBenchmarkTest : public ScenarioNetworkBenchmarkTest {};

/**
 * Compare boundedAlternatives with alternativesRouting between random pairs of
 * stops served by the scenario, at 8:00. Prints one CSV line per pair with a
 * result, then the mean times and the comparison of the journeys of the
 * sequences of paths found by both. Optional environment variables:
 * - TR_BENCHMARK_PAIRS_COUNT: number of pairs, 100 by default
 * - TR_BENCHMARK_EXTRA_TRANSFERS: extra transfers of boundedAlternatives, 2 by default
 * - TR_BENCHMARK_MAX_JOURNEYS: maximum journeys of boundedAlternatives, 10000 by default
 */
TEST_F(BoundedAlternativesBenchmarkTest, BoundedVsCurrentAlternatives)
{
  const size_t pairsCount = getEnvOrDefault("TR_BENCHMARK_PAIRS_COUNT", 100);
  const size_t extraTransfers = getEnvOrDefault("TR_BENCHMARK_EXTRA_TRANSFERS", 2);
  const size_t maxJourneys = getEnvOrDefault("TR_BENCHMARK_MAX_JOURNEYS", 10000);

  auto origins = TrRouting::sampleRandomNodes(network->getNodes(), pairsCount, 1);
  auto destinations = TrRouting::sampleRandomNodes(network->getNodes(), pairsCount, 2);
  // The samples keep the order of the nodes, shuffle the destinations to pair stops independently of their order
  std::shuffle(destinations.begin(), destinations.end(), std::mt19937(3));

  TrRouting::EuclideanGeoFilter geoFilter;
  TrRouting::Calculator calculator(*transitData, geoFilter);
  const auto getParameters = [](const TrRouting::Node &origin, const TrRouting::Node &destination) {
    return TrRouting::RouteParameters(std::make_unique<TrRouting::Point>(origin.point->latitude, origin.point->longitude),
                                      std::make_unique<TrRouting::Point>(destination.point->latitude, destination.point->longitude),
                                      *scenario, 8 * 3600, 3 * 60, 180 * 60, 20 * 60, 20 * 60, 20 * 60, 15 * 60, true, true);
  };
  const auto getDurationMs = [](std::chrono::steady_clock::time_point start) { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count(); };

  size_t pairs = 0, currentCount = 0, boundedCount = 0, foundCount = 0, missingWithPathTwiceCount = 0, missingWithMoreTransfersCount = 0;
  double currentMs = 0, boundedMs = 0;
  // Journeys of boundedAlternatives compared with the one of alternativesRouting for the same sequence of paths
  std::map<std::string, size_t> comparisons;
  std::cout << "origin_code,destination_code,current_ms,current_alternatives,bounded_ms,bounded_alternatives,current_found" << std::endl;
  for (size_t i = 0; i < std::min(origins.size(), destinations.size()); i++)
  {
    auto start = std::chrono::steady_clock::now();
    std::map<PathsSequence, const TrRouting::SingleCalculationResult *> currentBySequence;
    TrRouting::AlternativesResult current;
    try
    {
      current = calculator.alternativesRouting(getParameters(origins[i], destinations[i]));
    }
    catch (TrRouting::NoRoutingFoundException &)
    {
      continue;
    }
    const double pairCurrentMs = getDurationMs(start);
    start = std::chrono::steady_clock::now();
    const TrRouting::AlternativesResult bounded = calculator.boundedAlternatives(getParameters(origins[i], destinations[i]), *network, extraTransfers, maxJourneys);
    const double pairBoundedMs = getDurationMs(start);

    for (const auto &alternative : current.alternatives)
    {
      currentBySequence[getPathsSequence(*alternative)] = alternative.get();
    }
    size_t found = 0;
    std::set<PathsSequence> boundedSequences;
    for (const auto &journey : bounded.alternatives)
    {
      boundedSequences.insert(getPathsSequence(*journey));
      auto currentJourney = currentBySequence.find(getPathsSequence(*journey));
      if (currentJourney == currentBySequence.end())
      {
        continue;
      }
      found++;
      const TrRouting::SingleCalculationResult &currentResult = *currentJourney->second;
      comparisons[journey->arrivalTime < currentResult.arrivalTime ? "arrives earlier"
                  : journey->arrivalTime > currentResult.arrivalTime ? "arrives later"
                  : journey->departureTime > currentResult.departureTime ? "same arrival, leaves later"
                  : journey->departureTime < currentResult.departureTime ? "same arrival, leaves earlier"
                  : "same arrival and departure"]++;
    }
    // boundedAlternatives uses a path at most once per sequence, with at most extraTransfers more than the minimum
    const size_t minPaths = boundedSequences.empty() ? 0 : std::min_element(boundedSequences.begin(), boundedSequences.end(), [](const auto &sequence, const auto &other) { return sequence.size() < other.size(); })->size();
    for (const auto &[sequence, alternative] : currentBySequence)
    {
      if (boundedSequences.count(sequence) > 0)
      {
        continue;
      }
      if (std::set<boost::uuids::uuid>(sequence.begin(), sequence.end()).size() < sequence.size())
      {
        missingWithPathTwiceCount++;
      }
      else if (sequence.size() > minPaths + extraTransfers)
      {
        missingWithMoreTransfersCount++;
      }
    }
    std::cout << origins[i].get().code << "," << destinations[i].get().code << "," << pairCurrentMs << "," << currentBySequence.size() << ","
              << pairBoundedMs << "," << bounded.alternatives.size() << "," << found << std::endl;
    pairs++;
    currentMs += pairCurrentMs;
    boundedMs += pairBoundedMs;
    currentCount += currentBySequence.size();
    boundedCount += bounded.alternatives.size();
    foundCount += found;
  }

  ASSERT_GT(pairs, 0u);
  std::cout << pairs << " pairs: alternativesRouting " << currentMs / pairs << " ms and " << (double)currentCount / pairs << " alternatives, boundedAlternatives "
            << boundedMs / pairs << " ms and " << (double)boundedCount / pairs << " alternatives, finding " << foundCount << " of the " << currentCount << " alternatives. Of the others, " << missingWithPathTwiceCount << " use a path twice and " << missingWithMoreTransfersCount
            << " have more than " << extraTransfers << " transfers more than the minimum" << std::endl;
  for (const auto &[comparison, count] : comparisons)
  {
    std::cout << comparison << ": " << count << std::endl;
  }
}

## Running the benchmarks

The benchmarks require cache data to be available in the tests/benchmark_csa/cache/demo_transition directory. The cache data to match the benchmark can be found here:

https://nuage.facil.services/s/Gs7bwfEGaBjHCLg 

Simply unzip in the tests/benchmark_csa/cache directory. It corresponds to the STM's fall 2018 18S_S service (Société des Transports de Montréal).

To run, either run `make check` after having configured the repository with `./configure --enable-benchmark`, or simply run the `gtest` application in this directory after a first run of `make check`.

## Generating the static OSM network of a scenario

The `OsmNetworkBenchmarkTest` writes the paths usable in a scenario as an OSM file (one oneway way per path, through its stops, and one oneway footway per direction of each walking transfer between stops) and times the generation. It is skipped unless these environment variables are set:

```
TR_BENCHMARK_CACHE_PATH=/path/to/cache/directory \
TR_BENCHMARK_SCENARIO_ID=<scenario uuid> \
TR_BENCHMARK_OSM_OUTPUT=/path/to/network.osm \
./gtest --gtest_filter='OsmNetworkBenchmarkTest.*'
```

## Finding the journeys with the minimum number of transfers

The `MinTransfersJourneysBenchmarkTest` finds all the journeys (distinct sequences of paths) with the minimum number of transfers between random pairs of stops served by a scenario, without any time information. It prints one CSV line per pair, then a summary by minimum number of transfers. It is skipped unless the cache and scenario environment variables are set:

```
TR_BENCHMARK_CACHE_PATH=/path/to/cache/directory \
TR_BENCHMARK_SCENARIO_ID=<scenario uuid> \
TR_BENCHMARK_PAIRS_COUNT=100 \
./gtest --gtest_filter='MinTransfersJourneysBenchmarkTest.JourneysBetweenRandomStops'
```

The `JourneysGeoJson` test writes all the journeys between an origin and a destination as a GeoJSON file. The first two features are the origin and destination points, then each journey is a feature with the number of transfers, the lines of its legs and its own color (`stroke`). Each leg is boarded and left where it travels the fewest segments between stops. The origin and destination are either stops, given by their uuid (`TR_BENCHMARK_ORIGIN_ID` and `TR_BENCHMARK_DESTINATION_ID`), or places, given as `latitude,longitude` (`TR_BENCHMARK_ORIGIN_LAT_LON` and `TR_BENCHMARK_DESTINATION_LAT_LON`). For a place, the journeys can start or end at any stop within 20 minutes of walking, at 5 km/h as the crow flies; the walking time does not count:

```
TR_BENCHMARK_CACHE_PATH=/path/to/cache/directory \
TR_BENCHMARK_SCENARIO_ID=<scenario uuid> \
TR_BENCHMARK_ORIGIN_LAT_LON=45.5243,-73.5718 \
TR_BENCHMARK_DESTINATION_ID=<destination stop uuid> \
TR_BENCHMARK_GEOJSON_OUTPUT=/path/to/journeys.geojson \
./gtest --gtest_filter='MinTransfersJourneysBenchmarkTest.JourneysGeoJson'
```

## Comparing the bounded alternatives with the current alternatives

The `BoundedAlternativesBenchmarkTest` compares `Calculator::boundedAlternatives` with `Calculator::alternativesRouting` between random pairs of stops served by a scenario, at 8:00. It prints one CSV line per pair with a result, then the mean times, the number of current alternatives found by `boundedAlternatives` with the reasons why the others are not, and how the journeys of the sequences of paths found by both compare. The number of pairs, the extra transfers and the maximum number of journeys of `boundedAlternatives` are optional (100, 2 and 10000 by default):

```
TR_BENCHMARK_CACHE_PATH=/path/to/cache/directory \
TR_BENCHMARK_SCENARIO_ID=<scenario uuid> \
TR_BENCHMARK_PAIRS_COUNT=100 \
TR_BENCHMARK_EXTRA_TRANSFERS=2 \
TR_BENCHMARK_MAX_JOURNEYS=10000 \
./gtest --gtest_filter='BoundedAlternativesBenchmarkTest.*'
```
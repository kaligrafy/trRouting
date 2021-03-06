#include <errno.h>
#include <experimental/filesystem>

#include "gtest/gtest.h" // we will add the path to C preprocessor later
#include "parameters.hpp"
#include "cache_fetcher.hpp"
#include "cache_fetcher_test.hpp"
#include "station.hpp"
#include "capnp/stationCollection.capnp.h"

namespace fs = std::experimental::filesystem;

class StationCacheFetcherFixtureTests : public BaseCacheFetcherFixtureTests
{
protected:
    std::vector<std::unique_ptr<TrRouting::Station>>     stations;
    std::map<boost::uuids::uuid, int>        agencyIndexesByUuid;

public:
    void SetUp( ) override
    { 
        BaseCacheFetcherFixtureTests::SetUp();
        // Copy the invalid file
        fs::copy_file(PROJECT_NAME + "/" + INVALID_CUSTOM_PATH + "/genericInvalid.capnpbin", PROJECT_NAME + "/" + INVALID_CUSTOM_PATH + "/stations.capnpbin");
    }

    void TearDown( ) override
    { 
        BaseCacheFetcherFixtureTests::TearDown();
        // Remove the invalid file
        fs::remove(PROJECT_NAME + "/" + INVALID_CUSTOM_PATH + "/stations.capnpbin");
    }
};

TEST_F(StationCacheFetcherFixtureTests, TestGetStationsInvalid)
{
    int retVal = cacheFetcher.getStations(stations, agencyIndexesByUuid, params, INVALID_CUSTOM_PATH);
    ASSERT_EQ(-EBADMSG, retVal);
    ASSERT_EQ(0, stations.size());
}

TEST_F(StationCacheFetcherFixtureTests, TestGetStationsFileNotExists)
{
    int retVal = cacheFetcher.getStations(stations, agencyIndexesByUuid, params, BASE_CUSTOM_PATH);
    ASSERT_EQ(-ENOENT, retVal);
    ASSERT_EQ(0, stations.size());
}

// TODO Test with valid stations

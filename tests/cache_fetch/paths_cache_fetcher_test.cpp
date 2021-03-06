#include <errno.h>
#include <experimental/filesystem>

#include "gtest/gtest.h" // we will add the path to C preprocessor later
#include "parameters.hpp"
#include "cache_fetcher.hpp"
#include "cache_fetcher_test.hpp"
#include "path.hpp"
#include "capnp/pathCollection.capnp.h"

namespace fs = std::experimental::filesystem;

class PathCacheFetcherFixtureTests : public BaseCacheFetcherFixtureTests
{
protected:
    std::vector<std::unique_ptr<TrRouting::Path>> objects;
    std::map<boost::uuids::uuid, int> objectIndexesByUuid;
    std::map<boost::uuids::uuid, int> lineIndexesByUuid;
    std::map<boost::uuids::uuid, int> nodeIndexesByUuid;

public:
    void SetUp( ) override
    {
        BaseCacheFetcherFixtureTests::SetUp();
        // Copy the invalid file
        fs::copy_file(PROJECT_NAME + "/" + INVALID_CUSTOM_PATH + "/genericInvalid.capnpbin", PROJECT_NAME + "/" + INVALID_CUSTOM_PATH + "/paths.capnpbin");
    }

    void TearDown( ) override
    {
        BaseCacheFetcherFixtureTests::TearDown();
        // Remove the invalid file
        fs::remove(PROJECT_NAME + "/" + INVALID_CUSTOM_PATH + "/paths.capnpbin");
    }
};

TEST_F(PathCacheFetcherFixtureTests, TestGetPathsInvalid)
{
    int retVal = cacheFetcher.getPaths(objects, objectIndexesByUuid, lineIndexesByUuid, nodeIndexesByUuid, params, INVALID_CUSTOM_PATH);
    ASSERT_EQ(-EBADMSG, retVal);
    ASSERT_EQ(0, objects.size());
}

// TODO Add tests for various services, lines, agencies that don't exist. But first, we should be able to create cache files with mock test data
TEST_F(PathCacheFetcherFixtureTests, TestGetPathsValid)
{
    int retVal = cacheFetcher.getPaths(objects, objectIndexesByUuid, lineIndexesByUuid, nodeIndexesByUuid, params, VALID_CUSTOM_PATH);
    ASSERT_EQ(0, retVal);
    ASSERT_EQ(4, objects.size());
}

TEST_F(PathCacheFetcherFixtureTests, TestGetPathsFileNotExists)
{
    int retVal = cacheFetcher.getPaths(objects, objectIndexesByUuid, lineIndexesByUuid, nodeIndexesByUuid, params, BASE_CUSTOM_PATH);
    ASSERT_EQ(-ENOENT, retVal);
    ASSERT_EQ(0, objects.size());
}


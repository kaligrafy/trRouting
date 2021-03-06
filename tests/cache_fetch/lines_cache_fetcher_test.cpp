#include <errno.h>
#include <experimental/filesystem>

#include "gtest/gtest.h" // we will add the path to C preprocessor later
#include "parameters.hpp"
#include "cache_fetcher.hpp"
#include "cache_fetcher_test.hpp"
#include "line.hpp"
#include "capnp/lineCollection.capnp.h"

namespace fs = std::experimental::filesystem;

class LineCacheFetcherFixtureTests : public BaseCacheFetcherFixtureTests
{
protected:
    std::vector<std::unique_ptr<TrRouting::Line>> objects;
    std::map<boost::uuids::uuid, int> objectIndexesByUuid;
    std::map<boost::uuids::uuid, int> agencyIndexesByUuid;
    std::map<std::string, int> modeIndexesByShortname;

public:
    void SetUp( ) override
    {
        BaseCacheFetcherFixtureTests::SetUp();
        // Copy the invalid file
        fs::copy_file(PROJECT_NAME + "/" + INVALID_CUSTOM_PATH + "/genericInvalid.capnpbin", PROJECT_NAME + "/" + INVALID_CUSTOM_PATH + "/lines.capnpbin");
    }

    void TearDown( ) override
    {
        BaseCacheFetcherFixtureTests::TearDown();
        // Remove the invalid file
        fs::remove(PROJECT_NAME + "/" + INVALID_CUSTOM_PATH + "/lines.capnpbin");
    }
};

TEST_F(LineCacheFetcherFixtureTests, TestGetLinesInvalid)
{
    int retVal = cacheFetcher.getLines(objects, objectIndexesByUuid, agencyIndexesByUuid, modeIndexesByShortname, params, INVALID_CUSTOM_PATH);
    ASSERT_EQ(-EBADMSG, retVal);
    ASSERT_EQ(0, objects.size());
}

TEST_F(LineCacheFetcherFixtureTests, TestGetLinesValid)
{
    int retVal = cacheFetcher.getLines(objects, objectIndexesByUuid, agencyIndexesByUuid, modeIndexesByShortname, params, VALID_CUSTOM_PATH);
    ASSERT_EQ(0, retVal);
    ASSERT_EQ(2, objects.size());
}

TEST_F(LineCacheFetcherFixtureTests, TestGetLinesFileNotExists)
{
    int retVal = cacheFetcher.getLines(objects, objectIndexesByUuid, agencyIndexesByUuid, modeIndexesByShortname, params, BASE_CUSTOM_PATH);
    ASSERT_EQ(-ENOENT, retVal);
    ASSERT_EQ(0, objects.size());
}


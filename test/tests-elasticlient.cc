/**
 * \file
 * Tests for elasticlient library.
 */
#include <gtest/gtest.h>
#include <sstream>
#include <iostream>
#include <vector>
#include <json/json.h>
#include <cpr/cpr.h>
#include <httpmockserver/mock_server.h>
#include <httpmockserver/test_environment.h>

#include "elasticlient/logging.h"
#include "elasticlient/client.h"
#include "elasticlient/bulk.h"
#include "elasticlient/scroll.h"

/// Let test to access internal bulk functions.
#include "bulk-impl.h"
/// Let test to re-use logging feature.
#include "logging-impl.h"

namespace {


/// Simple log callback for elasticlient library.
void logCallback(elasticlient::LogLevel logLevel, const std::string &msg) {
    if (logLevel != elasticlient::LogLevel::DEBUG) {
        std::cout << "LOG " << (unsigned) logLevel << ": " << msg << std::endl;
    }
}

httpmock::TestEnvironment<httpmock::MockServerHolder>* mock_server_env = nullptr;


} // anonymous namespace


namespace elasticlient {


class HTTPMock: public httpmock::MockServer {
  public:
    explicit HTTPMock(unsigned port): httpmock::MockServer(port) {}
  private:
    Response responseHandler(
            const std::string &url,
            const std::string &method,
            const std::string &data,
            const std::vector<UrlArg> &,
            const std::vector<Header> &headers)
    {
        LOG(LogLevel::INFO, "Mock HTTP `%s` method `%s` called with %lu bytes of data.",
            method.c_str(), url.c_str(), data.size());
        LOG(LogLevel::INFO, "Mock HTTP data: `%s`.", data.c_str());

        // Strictly check when JSON content type header was sent when body is not empty
        if (!data.empty()) {
            bool jsonHeaderSent = false;
            for (const Header &header : headers) {
                if (header.key == "Content-Type"
                    && header.value == "application/json; charset=utf-8")
                {
                    jsonHeaderSent = true;
                    break;
                }
            }
            if (!jsonHeaderSent) {
                return Response(500, "JSON HTTP header not found when body was set!");
            }
        }

        // Mocked basic search
        if (method =="POST" && matchesPrefix(url, "/indexA/typeA/_search")) {
            return Response(201, data);
        }
        // Mocked search with specified index and document type
        if (method =="POST" && url == "/_search") {
            return Response(202, data);
        }
        // Mocked get document
        if (method =="GET" && url =="/indexA/typeA/123") {
            return Response(200, "GET_OK");
        }
        // Mocked index new document
        if (method =="POST" && url =="/indexA/typeA/321") {
            return Response(203, data);
        }
        // Mocked remove document
        if (method =="DELETE" && url =="/indexA/typeA/321") {
            return Response(200, "REMOVE_OK");
        }
        // Always return status 500 for /bulk_basics testcase
        if (matchesPrefix(url, "/bulk_basics/_bulk")) {
            return Response(500, "Internal error");
        }
        // Mocked initialize new scroll
        if (matchesPrefix(url, "/test_scroll_ok*/fake_index/_search")) {
            return Response(200, createScrollResponse("A0", false, 2, 2, 0));
        }
        // Mocked scroll next page
        if (method =="POST" && matchesPrefix(url, "/_search/scroll")) {
            if(data == "A0") {
                return Response(200, createScrollResponse("A1", false, 3, 2, 0));
            }
            else if(data == "A1") {
                return Response(200, createScrollResponse("A2", false, 0, 2, 0));
            }
            else if(data == "A2") {
                return Response(404, createScrollResponse("A3", false, 0, 1, 1));
            }
        }
        // Mocked scroll remove
        if (method == "DELETE" && matchesPrefix(url, "/_search/scroll")) {
            if(data == "A2") {;
                return Response(500, "{}");
            }
        }

        // Return "URI not found" for the rest...
        return Response(404, "Not Found");
    }

    /// Return true if \p url starts with \p str.
    bool matchesPrefix(const std::string &url, const std::string &str) const {
        return url.substr(0, str.size()) == str;
    }

    /// Prepare response for scroll requests
    std::string createScrollResponse(const std::string &scrollId,
                                     const bool timedOut,
                                     const int numOfHits,
                                     const int shardsSuccessful,
                                     const int shardsFailed,
                                     const Json::Value *error = nullptr) const
    {
        Json::Value response;
        response["_scroll_id"] = scrollId;
        response["took"] = 22;
        response["timed_out"] = timedOut;
        response["_shards"] = Json::Value();
        response["_shards"]["total"] = shardsSuccessful + shardsFailed;
        response["_shards"]["successful"] = shardsSuccessful;
        response["_shards"]["failed"] = shardsFailed;
        response["hits"] = Json::Value();
        response["hits"]["total"] = numOfHits;
        response["hits"]["hits"] = Json::arrayValue;
        for (int i = 0; i < numOfHits; i++) {
            response["hits"]["hits"].append(Json::Value());
        }
        if(error != nullptr) {
            response["error"] = error;
        }
        Json::FastWriter writer;

        return writer.write(response);
    }

    Json::Value parseJSONData(const std::string &data) const {
        Json::Value root;
        Json::Reader reader;
        reader.parse(data, root, false);
        return root;
    }
};


class ElasticlientTest: public ::testing::Test {
    std::vector<std::string> mockedHosts;

  protected:
    ElasticlientTest() {
        mockedHosts = {
            "http://localhost:" + std::to_string(mock_server_env->getMock()->getPort()) + "/"};
    }
    std::vector<std::string> &getMockedHosts() {
        return mockedHosts;
    }
};


TEST_F(ElasticlientTest, hostsFailed) {
    Client elasticClient({"http://fake.fake123:45100/", "http://fake.fake123:45101/"});
    ASSERT_THROW(elasticClient.search("fake", "fake", "{}"), ConnectionException);
}


TEST_F(ElasticlientTest, search) {
    Client elasticClient(getMockedHosts());
    std::string body = "{\"search\": \"A\"}";
    cpr::Response r = elasticClient.search("indexA", "typeA", body);
    ASSERT_EQ(201, r.status_code);
    ASSERT_EQ(body, r.text);

    r = elasticClient.search("", "", body);
    ASSERT_EQ(202, r.status_code);
    ASSERT_EQ(body, r.text);
}


TEST_F(ElasticlientTest, get) {
    Client elasticClient(getMockedHosts());
    cpr::Response r = elasticClient.get("indexA", "typeA", "123");
    ASSERT_EQ(200, r.status_code);
    ASSERT_EQ("GET_OK", r.text);
}


TEST_F(ElasticlientTest, index) {
    Client elasticClient(getMockedHosts());
    std::string body = "{\"name\": \"John\"}";
    cpr::Response r = elasticClient.index("indexA", "typeA", "321", body);
    ASSERT_EQ(203, r.status_code);
    ASSERT_EQ(body, r.text);
}


TEST_F(ElasticlientTest, remove) {
    Client elasticClient(getMockedHosts());
    cpr::Response r = elasticClient.remove("indexA", "typeA", "321");
    ASSERT_EQ(200, r.status_code);
    ASSERT_EQ("REMOVE_OK", r.text);
}


TEST_F(ElasticlientTest, bulkInternal) {
    // check if control field is generated correctly
    ASSERT_EQ(
        "{\"index\": {\"_type\": \"type1\", \"_id\": \"1\"}}",
        createControl("index", "type1", "1"));
    ASSERT_EQ(
        "{\"index\": {\"_type\": \"type1\"}}",
        createControl("index", "type1", ""));

    // Create bulk with two elements
    SameIndexBulkData bulk("my_index");
    ASSERT_TRUE(bulk.empty());
    bulk.indexDocument("my_type", "id1", "{data1}");
    ASSERT_FALSE(bulk.empty());
    bulk.createDocument("my_type", "id2", "{data2}");
    ASSERT_EQ(2, bulk.size());

    // expect newline-delimited output
    const std::string expected =
        "{\"index\": {\"_type\": \"my_type\", \"_id\": \"id1\"}}\n"
        "{data1}\n"
        "{\"create\": {\"_type\": \"my_type\", \"_id\": \"id2\"}}\n"
        "{data2}\n";
    ASSERT_EQ(expected, bulk.body());
}


TEST_F(ElasticlientTest, bulkBasics) {
    // Create bulk with two elements
    SameIndexBulkData bulk("foo");
    bulk.indexDocument("typeX", "id1", "{data1}");
    bulk.indexDocument("typeX", "id2", "{data2}");

    Bulk indexer(std::make_shared<Client>(getMockedHosts()));
    int errors = indexer.perform(bulk);
    ASSERT_EQ(2, errors);

    bulk.clear();
    bulk.indexDocument("typeY", "id3", "{\"data\": \"OK\"}");
    errors = indexer.perform(bulk);
    ASSERT_EQ(1, errors);
}


TEST_F(ElasticlientTest, scroll) {
    Scroll scrollInstance(std::make_shared<Client>(getMockedHosts()), 100, "1m");
    Json::Value hits;
    scrollInstance.init("test_scroll_ok*", "fake_index", "{}");
    ASSERT_TRUE(scrollInstance.next(hits));
    ASSERT_EQ(2, hits["hits"].size());
    ASSERT_TRUE(scrollInstance.next(hits));
    ASSERT_EQ(3, hits["hits"].size());
    ASSERT_TRUE(scrollInstance.next(hits));
    ASSERT_EQ(0, hits["hits"].size());
    ASSERT_FALSE(scrollInstance.next(hits));
    scrollInstance.clear();
    scrollInstance.init("test_scroll_ok*", "fake_index", "{}");
    ASSERT_TRUE(scrollInstance.next(hits));
    ASSERT_EQ(2, hits["hits"].size());
    ASSERT_TRUE(scrollInstance.next(hits));
    ASSERT_EQ(3, hits["hits"].size());
    scrollInstance.clear();
    ASSERT_FALSE(scrollInstance.next(hits));
}


}  // namespace elasticlient


int main(int argc, char *argv[]) {
    elasticlient::setLogFunction(logCallback);
    ::testing::InitGoogleTest(&argc, argv);

    ::testing::Environment * const env = ::testing::AddGlobalTestEnvironment(
            httpmock::createMockServerEnvironment<elasticlient::HTTPMock>(9200));

    // set global env pointer
    mock_server_env = dynamic_cast<httpmock::TestEnvironment<httpmock::MockServerHolder> *>(env);

    return RUN_ALL_TESTS();
}

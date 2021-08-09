/**
 * \file
 * Implementation of the Elastic Bulk API.
 */

#include "bulk-impl.h"

#include <string>
#include <sstream>
#include <cpr/cpr.h>
#include <rapidjson/document.h>
#include "logging-impl.h"
#include "elasticlient/client.h"


namespace {


/// Check whether a document does not contains new-line character.
void validateDocument(const std::string &doc, const std::string &id) {
    // new-line char is not allowed in document
    if (doc.find_first_of("\n") != std::string::npos) {
        LOG(elasticlient::LogLevel::ERROR,
            "A document for %s contains newline character.", id.c_str());
        throw std::runtime_error("Cannot index document containing newline char");
    }
}


} // anonymous namespace


namespace elasticlient {


IBulkData::~IBulkData() {}


SameIndexBulkData::SameIndexBulkData(const std::string &indexName, std::size_t size)
  : impl(new Implementation(indexName, size))
{}


SameIndexBulkData::~SameIndexBulkData() {}


std::string SameIndexBulkData::indexName() const {
    return impl->indexName;
}


bool SameIndexBulkData::indexDocument(const std::string &docType,
                                      const std::string &id,
                                      const std::string &doc)
{
    return indexDocument(docType, id, doc, true);
}


bool SameIndexBulkData::indexDocument(const std::string &docType,
                                      const std::string &id,
                                      const std::string &doc,
                                      bool validate)
{
    if (validate) {
        validateDocument(doc, id);
    }

    impl->data.emplace_back(createControl("index", docType, id), doc);
    // return true if bulk has reached its desired capacity
    return impl->data.size() >= impl->size;
}


bool SameIndexBulkData::createDocument(const std::string &docType,
                                       const std::string &id,
                                       const std::string &doc)
{
    return createDocument(docType, id, doc, true);
}


bool SameIndexBulkData::createDocument(const std::string &docType,
                                       const std::string &id,
                                       const std::string &doc,
                                       bool validate)
{
    if (validate) {
        validateDocument(doc, id);
    }

    impl->data.emplace_back(createControl("create", docType, id), doc);
    // return true if bulk has reached its desired capacity
    return impl->data.size() >= impl->size;
}


bool SameIndexBulkData::updateDocument(const std::string &docType,
                                       const std::string &id,
                                       const std::string &doc)
{
    return updateDocument(docType, id, doc, true);
}


bool SameIndexBulkData::updateDocument(const std::string &docType,
                                       const std::string &id,
                                       const std::string &doc,
                                       bool validate)
{
    if (validate) {
        validateDocument(doc, id);
    }

    impl->data.emplace_back(createControl("update", docType, id), doc);
    // return true if bulk has reached its desired capacity
    return impl->data.size() >= impl->size;
}


void SameIndexBulkData::clear() {
    impl->data.resize(0);
}


bool SameIndexBulkData::empty() const {
    return impl->data.empty();
}


std::size_t SameIndexBulkData::size() const {
    return impl->data.size();
}


std::string SameIndexBulkData::body() const {
    std::ostringstream body;
    for (const BulkItem &element: impl->data) {
        body << element;
    }
    return body.str();
}


Bulk::Bulk(const std::shared_ptr<Client> &client)
  : impl(new Implementation(client))
{}


Bulk::Bulk(const std::vector<std::string> &hostUrlList,
           std::int32_t connectionTimeout)
  : impl(new Implementation(
              std::make_shared<Client>(hostUrlList, connectionTimeout)))
{}


Bulk::~Bulk() {}


Bulk::Bulk(Bulk &&) = default;


std::string createControl(const std::string &action,
                          const std::string &docType,
                          const std::string &docId)
{
    std::ostringstream out;
    out << "{\"" << action << "\": {"
           "\"_type\": \"" << docType << "\"";

    if (!docId.empty()) {
        out << ", \"_id\": \"" << docId << "\"";
    }

    out << "}}";

    return out.str();
}


void Bulk::Implementation::run(const IBulkData &bulk) {
    std::string body = bulk.body();
    std::string indexName = bulk.indexName();
    try {
        const cpr::Response r = client->performRequest(Client::HTTPMethod::POST,
                                                       indexName + "/_bulk",
                                                       body);
        if (r.status_code / 100 != 2) {
            throw ConnectionException("Elastic node not respond with status 2xx.");
        }
        processResult(r.text, bulk.size());
    } catch(const ConnectionException &ex) {
        LOG(LogLevel::ERROR, "Elastic cluster while indexing bulk: %s", ex.what());
        errCount += bulk.size();
    }
}


std::size_t Bulk::perform(const IBulkData &bulk) {
    if (bulk.empty()) { return 0; }

    LOG(LogLevel::INFO, "Going to index %lu elements.", bulk.size());
    impl->errCount = 0;
    impl->run(bulk);
    return impl->errCount;
}


void Bulk::Implementation::processResult(
        const std::string &result, std::size_t size)
{
    rapidjson::Document root;
    rapidjson::ParseResult ok = root.Parse(result.c_str());
    if (!ok || !root.IsObject()) {
        // probably whole bulk has failed
        errCount += size;
        return;
    }

    // Expected response:
    // {"took": int,
    //  "errors": false,
    //  "items": [
    //      {"index": {
    //          "_index": string,
    //          "_type": string,
    //          "_id": string,
    //          "_version": int,
    //          "_shards": {"total": int, "successful": int, "failed": int},
    //          "status": 201}}
    //  ]}

    // check errors flag, if it is false, do not parse single responses
    if (root.HasMember("errors")) {
        const rapidjson::Value &errors = root["errors"];
        if (errors.IsBool() && !errors.GetBool()) {
            // everything is alright, errors==false.
            return;
        }
    }

    // check presence of the bulk items results
    if (!root.HasMember("items")) {
        LOG(LogLevel::WARNING, "Bulk ran with errors, but no items are present "
                               "at the response! Err count is inaccurate now!");
        return;
    }
    const rapidjson::Value &items = root["items"];
    // check correct type of the items
    if (!items.IsArray()) {
        LOG(LogLevel::WARNING, "Failed to read elastic response field 'items', because "
                               "it is not an array! Err count is inaccurate now!");
        return;
    }

    // process items responses
    for (const rapidjson::Value &item: items.GetArray()) {
        if (!item.IsObject()) {
            LOG(LogLevel::WARNING, "Bulk items responses have to be objects!");
            errCount++;
            continue;
        }

        // check index action response
        if (item.HasMember("index")) {
            const rapidjson::Value &res = item["index"];
            if (!res.IsObject()) {
                LOG(LogLevel::WARNING, "Bulk response has unexpected format, "
                                       "object was expected.");
                errCount++;
                continue;
            }
            // read status code
            if (!res.HasMember("status")) {
                LOG(LogLevel::WARNING, "Bulk response item with missing status.");
                errCount++;
                continue;
            };
            const rapidjson::Value &status = res["status"];
            if (!status.IsInt()) {
                LOG(LogLevel::WARNING, "Bulk response was expected to have numeric status. "
                                       "Skipping this response checking.");
                errCount++;
                continue;
            }

            // if status code is not 2xx family, consider it as error
            if (status.GetInt() / 100 != 2) {
                errCount++;
            }
        } else {
            LOG(LogLevel::WARNING, "Unsupported 'action' found at bulk response.");
        }
    }

    // complain if not all items of the bulk were covered by responses
    if (items.Size() < size) {
        LOG(LogLevel::INFO, "Bulk has more items than responses received. Cannot tell "
                            "whether %lu items succeeded...", size - items.Size());
    }
}


std::size_t Bulk::getErrorCount() const {
    return impl->errCount;
}


const std::shared_ptr<Client> &Bulk::getClient() const {
    return impl->client;
}


}  // namespace elasticlient

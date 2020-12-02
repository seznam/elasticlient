/**
 * \file
 * Implementation of the Elastic Scroll API.
 */

#include "scroll-impl.h"

#include <sstream>
#include <memory>
#include <cpr/cpr.h>
#include "logging-impl.h"
#include "elasticlient/scroll-parser.h"


namespace elasticlient {


Scroll::Scroll(const std::shared_ptr<Client> &client,
               std::size_t scrollSize,
               const std::string &scrollTimeout)
  : impl(new Implementation(client, scrollSize, scrollTimeout))
{}


Scroll::Scroll(const std::vector<std::string> &hostUrlList,
               std::size_t scrollSize,
               const std::string &scrollTimeout,
               std::int32_t connectionTimeout)
  : impl(new Implementation(
      std::make_shared<Client>(hostUrlList, connectionTimeout), scrollSize, scrollTimeout))
{}


Scroll::Scroll(Scroll &&) = default;


Scroll::~Scroll() {
    if (impl) {
        try {
            clear();
        } catch(const std::exception &ex) {
            try {
                LOG(LogLevel::ERROR, "Scroll destructor called but exception "
                                     "was raised: '%s'.", ex.what());
            }
            catch(const std::exception &) {}
        }
    }
}


void Scroll::Implementation::ScrollParams::clear() {
    scrollId.clear();
    searchBody.clear();
    docType.clear();
    indexName.clear();
}


bool Scroll::Implementation::run(
        const std::string &commonUrlPart, const std::string &body,
        elasticlient::JsonResult &parsedResult)
{
    try {
        const cpr::Response r = client->performRequest(Client::HTTPMethod::POST,
                                                       commonUrlPart, body);
        if (r.status_code / 100 == 2 or r.status_code == 404) {
            return parseScrollResult(r.text, parsedResult, scrollParameters.scrollId);
        }

    } catch(const ConnectionException &ex) {
        LOG(LogLevel::ERROR, "Elastic cluster failed while scrolling: %s", ex.what());
    }
    return false;
}


void Scroll::init(
        const std::string &indexName, const std::string &docType, const std::string &searchBody)
{
    // clear current scroll if exist
    if (impl->isInitialized() || impl->isScrollStarted()) {
        clear();
    }
    Implementation::ScrollParams &scrollParameters = impl->scrollParameters;
    scrollParameters.indexName = indexName;
    scrollParameters.docType = docType;
    scrollParameters.searchBody = searchBody;
}


bool Scroll::createScroll(elasticlient::JsonResult &parsedResult) {
    Implementation::ScrollParams &scrollParameters = impl->scrollParameters;
    std::ostringstream urlPart;
    urlPart << scrollParameters.indexName << "/" << scrollParameters.docType << "/_search?scroll="
            << impl->scrollTimeout << "&size=" << impl->scrollSize;
    LOG(LogLevel::INFO, "Scroll (create) on %s.", urlPart.str().c_str());
    LOG(LogLevel::INFO, "Scroll (create) body %s.", scrollParameters.searchBody.c_str());

    if (impl->run(urlPart.str(), scrollParameters.searchBody, parsedResult)) {
        return true;
    }

    // if here all hosts failed
    LOG(LogLevel::ERROR, "Elastic cluster failed while creating scroll.");
    return false;
}


bool Scroll::next(elasticlient::JsonResult &parsedResult) {
    const Implementation::ScrollParams &scrollParameters = impl->scrollParameters;

    if (!impl->isInitialized()) {
        LOG(LogLevel::WARNING, "There is no scroll initialized (call init() at first).");
        return false;
    }
    if (!impl->isScrollStarted()) {
        return createScroll(parsedResult);
    } else {
        std::ostringstream urlPart;
        urlPart << "_search/scroll?scroll=" << impl->scrollTimeout;
        LOG(LogLevel::INFO, "Scroll (next) on %s.", urlPart.str().c_str());
        const std::string requestBody{"{\"scroll_id\": \"" + scrollParameters.scrollId + "\"}"};
        if (impl->run(urlPart.str(), requestBody, parsedResult)) {
            return true;
        }
    }

    // if here all hosts failed
    LOG(LogLevel::ERROR, "Elastic cluster failed while scrolling (next).");
    return false;
}


void Scroll::clear() {
    LOG(LogLevel::INFO, "Scroll (clear) called.");
    Implementation::ScrollParams &scrollParameters = impl->scrollParameters;
    if (!impl->isScrollStarted()) {
        LOG(LogLevel::INFO, "There is no scroll started (scrollId is empty).");
    } else {
        const std::string requestBody{"{\"scroll_id\": [\"" + scrollParameters.scrollId + "\"]}"};
        try {
            const cpr::Response r = impl->client->performRequest(
                Client::HTTPMethod::DELETE, "_search/scroll/", requestBody);
            if (r.status_code / 100 != 2) {
                LOG(LogLevel::WARNING, "Scroll delete failed response text: %s", r.text.c_str());
            }
        } catch(const ConnectionException &ex) {
            LOG(LogLevel::ERROR, "Elastic cluster failed while clearing scroll: %s", ex.what());
        }
    }

    // clear scroll parameters
    scrollParameters.clear();
}


const std::shared_ptr<Client> &Scroll::getClient() const {
    return impl->client;
}


ScrollByScan::ScrollByScan(const std::shared_ptr<Client> &client,
                           std::size_t scrollSize,
                           const std::string &scrollTimeout,
                           int primaryShardsCount)
  : Scroll(client, scrollSize, scrollTimeout)
{
    if (primaryShardsCount != 0) {
        impl->scrollSize = (std::size_t) scrollSize / primaryShardsCount;
    }
}


ScrollByScan::ScrollByScan(const std::vector<std::string> &hostUrlList,
                           std::size_t scrollSize,
                           const std::string &scrollTimeout,
                           int primaryShardsCount,
                           std::int32_t connectionTimeout)
  : Scroll(hostUrlList, scrollSize, scrollTimeout, connectionTimeout)
{
    if (primaryShardsCount != 0) {
        impl->scrollSize = (std::size_t) scrollSize / primaryShardsCount;
    }
}


ScrollByScan::ScrollByScan(ScrollByScan &&) = default;


bool ScrollByScan::createScroll(elasticlient::JsonResult &parsedResult) {
    Implementation::ScrollParams &scrollParameters = impl->scrollParameters;

    std::ostringstream urlPart;
    urlPart << scrollParameters.indexName << "/" << scrollParameters.docType << "/_search?scroll="
            << impl->scrollTimeout << "&size=" << impl->scrollSize << "&search_type=scan";

    LOG(LogLevel::INFO, "Scroll (create) on %s.", urlPart.str().c_str());
    LOG(LogLevel::INFO, "Scroll (create) body %s.",scrollParameters.searchBody.c_str());

    if (impl->run(urlPart.str(), scrollParameters.searchBody, parsedResult)) {
        // call next() again to obtain results (scan not giving results on first request)
        return next(parsedResult);
    }

    // if here all hosts failed
    LOG(LogLevel::ERROR, "Elastic cluster failed while creating scroll.");
    return false;
}


}  // namespace elasticlient

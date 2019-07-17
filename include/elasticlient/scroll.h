/**
 * \file
 * Module for using of Elasticsearch scroll API.
 */

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <cstdint>
#include <rapidjson/document.h>


/// The elasticlient namespace
namespace elasticlient {


/**
 * Scroll result.
 */
struct JsonResult {
    /// Parsed document. On successful scroll it contains ["hits"]["hits"] array.
    rapidjson::Document document;
};


// Forward Client class existence.
class Client;


/// Class for use of Elasticsearch Scroll API
class Scroll {
  protected:
    class Implementation;
    /// Hidden implementation and data holder.
    std::unique_ptr<Implementation> impl;

  public:
    /**
     * Initialize class for usage of Elasticsearch scroll API.
     * \param client initialized Client object.
     * \param scrollSize number of results per one scroll "page".
     * \param scrollTimeout time during scroll search context remaining alive. Defined by
     *        Elastic Time Units (i.e. 1m = 1 minute).
     */
    explicit Scroll(const std::shared_ptr<Client> &client,
                    std::size_t scrollSize = 100,
                    const std::string &scrollTimeout = "1m");

    /**
     * Initialize class for usage of Elasticsearch scroll API and create Client instance
     * for specified hostUrlList and timeout.
     * \param hostUrlList list of URLs of Elastic nodes in one cluster.
     * \param scrollSize number of results per one scroll "page".
     * \param scrollTimeout time during scroll search context remaining alive. Defined by
     *        Elastic Time Units (i.e. 1m = 1 minute).
     * \param timeout Elasticsearch node connection timeout.
     */
    explicit Scroll(const std::vector<std::string> &hostUrlList,
                    std::size_t scrollSize = 100,
                    const std::string &scrollTimeout = "1m",
                    std::int32_t connectionTimeout = 6000);

    Scroll(Scroll &&);

    virtual ~Scroll();

    /// Clear scroll for next usage. Should be called immediately after last next() was called.
    void clear();

    /**
     * Initialize new Scroll search.
     * \param indexName specification of an Elasticsearch index.
     * \param docType specification of an Elasticsearch document type.
     * \param searchBody Elasticsearch query body.
     */
    void init(const std::string &indexName,
              const std::string &docType,
              const std::string &searchBody);

    /**
     * Scroll next (get next bulk of results) and parse it into \p parsedResult.
     * \return true if okay
     * \return false on error
     */
    bool next(JsonResult &parsedResult);

    /// Return Client class with current config.
    const std::shared_ptr<Client> &getClient() const;

  protected:
    /// Creates new scroll - obtain scrollId and parsedResult
    virtual bool createScroll(JsonResult &parsedResult);
};


/// Class for use of Elasticsearch Scroll API with deprecated scan option
class ScrollByScan : public Scroll {
  public:
    ScrollByScan(ScrollByScan &&);

    /// \see Scroll
    explicit ScrollByScan(const std::shared_ptr<Client> &client,
                          std::size_t scrollSize = 100,
                          const std::string &scrollTimeout = "1m",
                          int primaryShardsCount = 0);

    /// \see Scroll
    explicit ScrollByScan(const std::vector<std::string> &hostUrlList,
                          std::size_t scrollSize = 100,
                          const std::string &scrollTimeout = "1m",
                          int primaryShardsCount = 0,
                          std::int32_t connectionTimeout = 6000);

  protected:
    /**
     * Creates new scroll - obtain scrollId and parsedResult
     * Different from parrent Scroll::createScroll(), for scan it must
     * make two Elasticsearch calls one for obtain scrollId and second for obtain
     * first bulk of results.
     */
    virtual bool createScroll(JsonResult &parsedResult) override;
};


} // namespace elasticlient

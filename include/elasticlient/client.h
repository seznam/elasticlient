/**
 * \file
 * Module of Elasticsearch Client. Responsible for performing requests on Elasticsearch cluster.
 */

#pragma once

#include <string>
#include <memory>
#include <cstdint>
#include <stdexcept>
#include <vector>


// Forward cpr::Response existence.
namespace cpr {
    class Response;
}


/// The elasticlient namespace
namespace elasticlient {


class ConnectionException: public std::runtime_error {
  public:
    explicit ConnectionException(const std::string &message) : std::runtime_error(message) {}
};


/// Class for managing Elasticsearch connection in one Elasticsearch cluster
class Client {
    class Implementation;
    /// Hidden implementation and data holder.
    std::unique_ptr<Implementation> impl;

  public:
    /// Enum for kinds of supported HTTP methods in performRequest.
    enum class HTTPMethod {
        GET     = 0,
        POST    = 1,
        PUT     = 2,
        DELETE  = 3,
        HEAD    = 4
    };

    /**
     * Initialize the Client.
     * \param hostUrlList  Vector of URLs of Elasticsearch nodes in one Elasticsearch cluster.
     *  Each URL in vector should ends by "/".
     * \param timeout      Elastic node connection timeout.
     */
    explicit Client(const std::vector<std::string> &hostUrlList,
                    std::int32_t timeout = 6000);

    /**
     * Initialize the Client.
     * \param hostUrlList   Vector of URLs of Elasticsearch nodes in one Elasticsearch cluster.
     *  Each URL in vector should end by "/".
     * \param timeout       Elastic node connection timeout.
     * \param proxyUrlList  List of used HTTP proxies.
     */
    explicit Client(
        const std::vector<std::string> &hostUrlList,
        std::int32_t timeout,
        const std::initializer_list<std::pair<const std::string, std::string>>& proxyUrlList);

    Client(Client &&);

    ~Client();

    /**
     * Perform request on nodes until it is successful. Throws exception if all nodes
     * has failed to respond.
     * \param method one of Client::HTTPMethod.
     * \param urlPath part of URL immediately behind "scheme://host/".
     * \param body Elasticsearch request body.
     *
     * \return cpr::Response if any of node responds to request.
     * \throws ConnectionException if all hosts in cluster failed to respond.
     */
    cpr::Response performRequest(HTTPMethod method,
                                 const std::string &urlPath,
                                 const std::string &body);

    /**
     * Perform search on nodes until it is successful. Throws exception if all nodes
     * has failed to respond.
     * \param indexName specification of an Elasticsearch index.
     * \param docType specification of an Elasticsearch document type.
     * \param body Elasticsearch request body.
     * \param routing Elasticsearch routing. If empty, no routing has been used.
     *
     * \return cpr::Response if any of node responds to request.
     * \throws ConnectionException if all hosts in cluster failed to respond.
     */
    cpr::Response search(const std::string &indexName,
                         const std::string &docType,
                         const std::string &body,
                         const std::string &routing = std::string());

    /**
     * Get document with specified id from cluster. Throws exception if all nodes
     * has failed to respond.
     * \param indexName specification of an Elasticsearch index.
     * \param docType specification of an Elasticsearch document type.
     * \param id Id of document which should be retrieved.
     * \param routing Elasticsearch routing. If empty, no routing has been used.
     *
     * \return cpr::Response if any of node responds to request.
     * \throws ConnectionException if all hosts in cluster failed to respond.
     */
    cpr::Response get(const std::string &indexName,
                      const std::string &docType,
                      const std::string &id = std::string(),
                      const std::string &routing = std::string());

    /**
     * Index new document to cluster. Throws exception if all nodes has failed to respond.
     * \param indexName specification of an Elasticsearch index.
     * \param docType specification of an Elasticsearch document type.
     * \param body Elasticsearch request body.
     * \param id Id of document which should be indexed. If empty, id will be generated
     *           automatically by Elasticsearch cluster.
     * \param routing Elasticsearch routing. If empty, no routing has been used.
     *
     * \return cpr::Response if any of node responds to request.
     * \throws ConnectionException if all hosts in cluster failed to respond.
     */
    cpr::Response index(const std::string &indexName,
                        const std::string &docType,
                        const std::string &id,
                        const std::string &body,
                        const std::string &routing = std::string());

    /**
     * Delete document with specified id from cluster. Throws exception if all nodes
     * has failed to respond.
     * \param indexName specification of an Elasticsearch index.
     * \param docType specification of an Elasticsearch document type.
     * \param id Id of document which should be deleted.
     * \param routing Elasticsearch routing. If empty, no routing has been used.
     *
     * \return cpr::Response if any of node responds to request.
     * \throws ConnectionException if all hosts in cluster failed to respond.
     */
    cpr::Response remove(const std::string &indexName,
                         const std::string &docType,
                         const std::string &id,
                         const std::string &routing = std::string());
};


} // namespace elasticlient

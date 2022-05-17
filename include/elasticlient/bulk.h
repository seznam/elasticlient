/**
 * \file
 * Bulk indexer for Elasticsearch.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cstdint>


/// The elasticlient namespace
namespace elasticlient {


// Forward Client class existence.
class Client;


/// Interface for Bulk data collector classes.
class IBulkData {
  public:
    virtual ~IBulkData();

    /**
     * Return index name bulk data belongs to.
     * Empty string if data could have different index names.
     */
    virtual std::string indexName() const = 0;

    /// Return true if bulk has no data inside.
    virtual bool empty() const = 0;

    /// Return number of documents inside the bulk.
    virtual std::size_t size() const = 0;

    /// Return elasticsearch bulk request data.
    virtual std::string body() const = 0;
};


/**
 * Data collector for the bulk operation. All bulk data must be
 * determined to be send to same index.
 * Class produces document body for elasticsearch /_bulk request.
 */
class SameIndexBulkData: public IBulkData {
    class Implementation;
    std::unique_ptr<Implementation> impl;

  public:
    /**
     * Create Bulk data collector with desired size.
     * Size of the bulk is not limiting the client to insert more elements
     * into bulk.
     * \param indexName  Name of the index all data will be send to.
     */
    explicit SameIndexBulkData(const std::string &indexName, std::size_t size = 100);
    ~SameIndexBulkData();

    /// Return index name set during contruction.
    virtual std::string indexName() const override;

    /**
     * Add index document request to the bulk.
     * \param docType document type (as specified in mapping).
     * \param id document ID, for auto-generated ID use empty string.
     * \param doc Json document to index. Must not contain newline char.
     * \return true if bulk has reached its desired capacity.
     */
    bool indexDocument(const std::string &docType,
                       const std::string &id,
                       const std::string &doc);

    /**
     * Add index document request to the bulk.
     * \param docType document type (as specified in mapping).
     * \param id document ID, for auto-generated ID use empty string.
     * \param doc Json document to index. Must not contain newline char.
     * \param validate checks whether string contains a newline.
     * \return true if bulk has reached its desired capacity.
     * \todo Merge with indexDocument() above with validate=true when ABI breaks.
     */
    bool indexDocument(const std::string &docType,
                       const std::string &id,
                       const std::string &doc,
                       bool validate);

    /**
     * Add create document request to the bulk.
     * \param docType document type (as specified in mapping).
     * \param id document ID, for auto-generated ID use empty string.
     * \param doc Json document to index. Must not contain newline char.
     * \return true if bulk has reached its desired capacity.
     */
    bool createDocument(const std::string &docType,
                        const std::string &id,
                        const std::string &doc);

    /**
     * Add create document request to the bulk.
     * \param docType document type (as specified in mapping).
     * \param id document ID, for auto-generated ID use empty string.
     * \param doc Json document to index. Must not contain newline char.
     * \param validate checks whether string contains a newline.
     * \return true if bulk has reached its desired capacity.
     * \todo Merge with createDocument() above with validate=true when ABI breaks.
     */
    bool createDocument(const std::string &docType,
                        const std::string &id,
                        const std::string &doc,
                        bool validate);

    /**
     * Add update document request to the bulk.
     * \param docType document type (as specified in mapping).
     * \param id document ID, for auto-generated ID use empty string.
     * \param doc Json document to index. Must not contain newline char.
     * \return true if bulk has reached its desired capacity.
     */
    bool updateDocument(
            const std::string &docType,
            const std::string &id,
            const std::string &doc);

    /**
     * Add update document request to the bulk.
     * \param docType document type (as specified in mapping).
     * \param id document ID, for auto-generated ID use empty string.
     * \param doc Json document to index. Must not contain newline char.
     * \param validate checks whether string contains a newline.
     * \return true if bulk has reached its desired capacity.
     * \todo Merge with updateDocument() above with validate=true when ABI breaks.
     */
    bool updateDocument(
            const std::string &docType,
            const std::string &id,
            const std::string &doc,
            bool validate);

    /// Clear bulk (size() == 0 after this).
    void clear();

    /// Return true if bulk has no data inside.
    virtual bool empty() const override;

    /// Return number of documents inside the bulk.
    virtual std::size_t size() const override;

    /// Return elasticsearch bulk request data.
    virtual std::string body() const override;
};


/// Class for bulk document indexing.
class Bulk {
    class Implementation;
    std::unique_ptr<Implementation> impl;

  public:
    /**
     * Initialize bulk indexer, using already configured Client class.
     * \param client initialized Client object.
     */
    Bulk(const std::shared_ptr<Client> &client);

    /**
     * Initialize bulk indexer and creates Client instance for specified
     * hostUrlList and connectionTimeout.
     * \param hostUrlList list of URLs of Elastic nodes in one cluster.
     * \param connectionTimeout Elasticsearch node connection timeout.
     */
    Bulk(const std::vector<std::string> &hostUrlList,
         std::int32_t connectionTimeout);
    /**
     * Destroy bulk indexer, writing documents not yet indexed, discarding
     * possible errors. If you are interested in errors, call flush() and
     * getErrorCount() methods before destruction is made.
     */
    ~Bulk();

    Bulk(Bulk &&other);

    /**
     * Run the bulk.
     * \return Number of errors occured.
     */
    std::size_t perform(const IBulkData &bulk);

    /// Return number of errors in last bulk being ran.
    std::size_t getErrorCount() const;

    /// Return Client class with current config.
    const std::shared_ptr<Client> &getClient() const;

};


}  // namespace elasticlient

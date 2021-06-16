/**
 * \file
 * Implementation of bulk indexer.
 */

#pragma once

#include "elasticlient/bulk.h"

#include <string>
#include <vector>
#include <ostream>
#include <stdexcept>


namespace elasticlient {


/**
 * Create control field for one bulk item.
 *
 * If docId is empty, the Id will be automatically generated.
 *
 * Something like that is produced for call:
 * \code
 *   createControl("index", "type1", "1");
 *   {"index": {"_type": "type1", "_id": "1"}}
 * \endcode
 */
std::string createControl(const std::string &action,
                          const std::string &docType,
                          const std::string &docId = "");


struct BulkItem {
    /// Control field record.
    std::string control;
    /// Data (_source) field. Not needed for "delete" action.
    std::string source;

    explicit BulkItem(const std::string &control = "",
                      const std::string &source = "")
      : control(control), source(source)
    {}

    friend std::ostream &operator<<(std::ostream &os, const BulkItem &item) {
        if (item.control.empty()) {
            return os;
        }
        os << item.control << "\n";
        if (not item.source.empty()) {
            os << item.source << "\n";
        }
        return os;
    }
};


class SameIndexBulkData::Implementation {
    /// Index to which all data belongs to.
    std::string indexName;
    /// Desired bulk size
    std::size_t size;
    /// Bulk documents
    std::vector<BulkItem> data;

  public:
    explicit Implementation(const std::string &indexName, std::size_t size)
      : indexName(indexName), size(size), data()
    {
        if (indexName.empty()) {
            throw std::runtime_error("Index name is mandatory argument");
        }
        if (size) {
            data.reserve(size);
        }
    }

    friend class SameIndexBulkData;
};


class Bulk::Implementation {
    /// Client holder
    std::shared_ptr<Client> client;
    /// Number of errors occured (failed to index).
    std::size_t errCount;

    // allow Bulk to access private members
    friend class Bulk;

  public:
    Implementation(std::shared_ptr<Client> elasticClient)
      : client(std::move(elasticClient)), errCount(0)
    {
        if (!client) {
            throw std::runtime_error("Valid Client instance is required.");
        }
    }

    /**
     * Send bulk body on Client
     * Request errors are counted to the bulk counters.
     */
    void run(const IBulkData &bulk);

  private:
    /// Check correctness of bulk result and update error counters.
    void processResult(const std::string &result, std::size_t size);
};


}  // namespace elasticlient

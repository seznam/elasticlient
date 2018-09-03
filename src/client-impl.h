/**
 * \file
 * Elasticsearch Client implementation.
 */

#pragma once

#include "elasticlient/client.h"

#include <string>
#include <vector>
#include <stdexcept>
#include <cstdint>
#include <random>
#include <thread>
#include <time.h>
#include <cpr/session.h>
#include "logging-impl.h"


namespace elasticlient {


/// Helper class for generating pseudorandom uint.
class RandomUIntGenerator {
     std::mt19937 generator;

  public:
    RandomUIntGenerator(): generator(std::random_device()()) {}

    // Generates pseudorandom unit in interval [a, b] unit using uniform distribution.
    std::uint32_t getRandom(std::uint32_t a, std::uint32_t b) {
        std::uniform_int_distribution<std::uint32_t> distribution(a, b);
        return distribution(generator);
    }
};


class Client::Implementation {
    const std::vector<std::string> hostUrlList;
    cpr::Session session;
    cpr::Authentication auth;
    uint32_t currentHostIndex, failCounter;
    RandomUIntGenerator uintGenerator;

    friend class Client;

  public:
    Implementation(const std::vector<std::string> &hostUrlList,
                    const std::string &user,
                    const std::string &password,
                    std::int32_t timeout)
      : hostUrlList(hostUrlList), session(), auth(user, password), currentHostIndex(0), failCounter(0), uintGenerator()
    {
        if (hostUrlList.empty()) {
            throw std::runtime_error("Hosts URL list can not be empty.");
        }
        session.SetTimeout(timeout);
        session.SetAuth(auth);
        resetCurrentHostInfo();
    }

  private:
    /// Reset currentHostIndex and failCounter.
    void resetCurrentHostInfo() {
        currentHostIndex = uintGenerator.getRandom(0, hostUrlList.size()-1);
        failCounter = 0;
    }

    /// If it is possible to increment currentHostIndex - return true, return false otherwise.
    bool failCurrentHostAndIterateNext() {
        if (++failCounter >= hostUrlList.size()) {
            resetCurrentHostInfo();
            return false;
        }
        if (++currentHostIndex >= hostUrlList.size()) {
            currentHostIndex = 0;
        }
        return true;
    }

    /**
     * Perform request on current Elastic node.
     * \param method  One of Client::HTTPMethod.
     * \param urlPath Part of URL imidiately behind "scheme://host/".
     * \param body    Request body.
     * \param response cpr::Response& to be response store there.
     *
     * \return true if request was sucessfully performed.
     * \return false if host failed for this request.
     */
    bool performRequestOnCurrentHost(Client::HTTPMethod method,
                                     const std::string &urlPath,
                                     const std::string &body,
                                     cpr::Response &response);

    /// \see Client::performRequest
    cpr::Response performRequest(Client::HTTPMethod method,
                                 const std::string &urlPath,
                                 const std::string &body = std::string());
};


}  // namespace elasticlient

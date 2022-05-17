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
#include <cpr/proxies.h>
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
    uint32_t currentHostIndex, failCounter;
    RandomUIntGenerator uintGenerator;

    friend class Client;

  public:
    Implementation(const std::vector<std::string> &hostUrlList,
            std::int32_t timeout,
            const std::initializer_list<std::pair<const std::string, std::string>>& proxyUrlList = {})
      : hostUrlList(hostUrlList), session(), currentHostIndex(0), failCounter(0), uintGenerator()
    {
        if (hostUrlList.empty()) {
            throw std::runtime_error("Hosts URL list can not be empty.");
        }

        if (proxyUrlList.size()) {
            session.SetProxies(cpr::Proxies(proxyUrlList));
        }
        session.SetTimeout(timeout);
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

    /// Set client option from ClientOption derived classes.
    void setClientOption(const ClientOption &opt) {
        // invoke opt virtual method that will call visit() with specific type.
        opt.accept(*this);
    }

    /// Set timeout from given instance.
    void visit(const TimeoutOption &);
    /// Set connection timeout from fiven instance.
    void visit(const ConnectTimeoutOption &);
    /// Set proxies from given instance.
    void visit(const ProxiesOption &);
    /// Set SSL options from given instance.
    void visit(const SSLOption &);
};


class Client::SSLOption::SSLOptionImplementation {
    /// SSL Options to set up.
    cpr::SslOptions sslOptions;
  public:
    SSLOptionImplementation(): sslOptions() {}

    const cpr::SslOptions &getOptions() const {
        return sslOptions;
    }

    /// Set single option - derived from class SSLOptionType.
    void setSslOption(const SSLOptionType &opt) {
        // invoke opt virtual method that will call visit() with specific type.
        opt.accept(*this);
    }

    /// Set certfile from given instance.
    void visit(const CertFile &certFile);
    /// Set keyfile from given instance.
    void visit(const KeyFile &keyFile);
    /// Set CA info from given instance.
    void visit(const CaInfo &caBundle);
    /// Set verify host flag from given option instance.
    void visit(const VerifyHost &opt);
    /// Set verify peer flag from given option instance.
    void visit(const VerifyPeer &opt);
};


}  // namespace elasticlient

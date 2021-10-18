/**
 * \file
 * Implementation of the theElasticsearch Client. Responsible for performing
 * requests on Elasticsearch cluster.
 */

#include "client-impl.h"

#include <sstream>
#include <memory>
#include <cpr/cpr.h>
#include "logging-impl.h"

#ifndef __GNUC__ 
#undef DELETE
#endif

namespace {


/**
 * Extends urlPath with arguments indexName and docType like indexName/docType/
 * \param indexName specification of an Elasticsearch index.
 * \param indexNameNotEmpty if true raises std::runtime_error on \p indexName is empty
 * \param docType specification of an Elasticsearch document type.
 * \param docTypeNotEmpty if true raises std::runtime_error on \p docType is empty
 * \param urlPath fill mentioned arguments there. *
 *
 * \throws std::runtime_error from above mentioned reasons
 */
void fillIndexAndTypeInUrlPath(
        const std::string &indexName, bool indexNameNotEmpty,
        const std::string &docType, bool docTypeNotEmpty,
        std::ostringstream &urlPath)
{
    if (indexNameNotEmpty && indexName.empty()) {
        throw std::runtime_error("Argument indexName can not be empty.");
    } else if (!indexName.empty()) {
        urlPath << indexName << "/";
    }
    if (docTypeNotEmpty && docType.empty()) {
        throw std::runtime_error("Argument docType can not be empty.");
    } else if (!docType.empty()) {
        urlPath << docType << "/";
    }

}


/// Extendes \p urlPath with routing argument if not empty
void fillRoutingInUrlPath(const std::string &routing, std::ostringstream &urlPath) {
    if (!routing.empty()) {
        urlPath << "?routing=" << routing;
    }
}


} // anonymous namespace


namespace elasticlient {

void Client::TimeoutOption::accept(Implementation &impl) const {
    impl.visit(*this);
}

void Client::ConnectTimeoutOption::accept(Implementation &impl) const {
    impl.visit(*this);
}


class Client::ProxiesOption::ProxiesOptionImplementation {
    cpr::Proxies proxies;
  public:
    ProxiesOptionImplementation(
            const std::initializer_list<std::pair<const std::string, std::string>> &proxies)
        : proxies(proxies)
    {}

    const cpr::Proxies &getProxies() const {
        return proxies;
    }
};

Client::ProxiesOption::ProxiesOption(
        const std::initializer_list<std::pair<const std::string, std::string>> &proxies)
    : impl(new Client::ProxiesOption::ProxiesOptionImplementation(proxies))
{}

Client::ProxiesOption::~ProxiesOption() = default;

void Client::ProxiesOption::accept(Implementation &impl) const {
    impl.visit(*this);
}


Client::SSLOption::SSLOption(): impl(new SSLOptionImplementation()) {}
Client::SSLOption::SSLOption(SSLOption &&) = default;
Client::SSLOption::~SSLOption() = default;

void Client::SSLOption::setSslOption(const SSLOptionType &opt) {
    impl->setSslOption(opt);
}

void Client::SSLOption::accept(Implementation &impl) const {
    impl.visit(*this);
}

void Client::SSLOption::CertFile::accept(SSLOptionImplementation &impl) const {
    impl.visit(*this);
}

void Client::SSLOption::KeyFile::accept(SSLOptionImplementation &impl) const {
    impl.visit(*this);
}

void Client::SSLOption::CaInfo::accept(SSLOptionImplementation &impl) const {
    impl.visit(*this);
}

void Client::SSLOption::VerifyHost::accept(SSLOptionImplementation &impl) const {
    impl.visit(*this);
}

void Client::SSLOption::VerifyPeer::accept(SSLOptionImplementation &impl) const {
    impl.visit(*this);
}


Client::Client(const std::vector<std::string> &hostUrlList,
               std::int32_t timeout)
  : impl(new Implementation(hostUrlList, timeout))
{}

Client::Client(const std::vector<std::string> &hostUrlList,
               std::int32_t timeout,
               const std::initializer_list<std::pair<const std::string, std::string>>& proxyUrlList)
  : impl(new Implementation(hostUrlList, timeout, proxyUrlList))
{}

Client::Client(Client &&) = default;

Client::~Client() {}


void Client::setClientOption(const ClientOption &opt) {
    impl->setClientOption(opt);
}


cpr::Response Client::performRequest(
        HTTPMethod method, const std::string &urlPath, const std::string &body)
{
   return impl->performRequest(method, urlPath, body);
}


bool Client::Implementation::performRequestOnCurrentHost(Client::HTTPMethod method,
                                                         const std::string &urlPath,
                                                         const std::string &body,
                                                         cpr::Response &response)
{
    const std::string entireUrl = hostUrlList[currentHostIndex] + urlPath;
    session.SetUrl(cpr::Url(entireUrl));
    cpr::Header header;
    if (!body.empty()) {
        header["Content-Type"] = "application/json; charset=utf-8";
    }
    session.SetHeader(header);
    session.SetBody(cpr::Body(body));

    switch (method) {
        case Client::HTTPMethod::GET:
            LOG(LogLevel::DEBUG, "Called GET: %s", urlPath.c_str());
            response = session.Get();
            break;
        case Client::HTTPMethod::POST:
            LOG(LogLevel::DEBUG, "Called POST: %s", urlPath.c_str());
            response = session.Post();
            break;
        case Client::HTTPMethod::PUT:
            LOG(LogLevel::DEBUG, "Called PUT: %s", urlPath.c_str());
            response = session.Put();
            break;
        case Client::HTTPMethod::DELETE:
            LOG(LogLevel::DEBUG, "Called DELETE: %s", urlPath.c_str());
            response = session.Delete();
            break;
        case Client::HTTPMethod::HEAD:
            LOG(LogLevel::DEBUG, "Called HEAD: %s", urlPath.c_str());
            response = session.Head();
            break;
        default:
            throw std::runtime_error("This HTTP method is not implemented yet.");
    }

    LOG(LogLevel::INFO, "Host returned %ld in %lf s for %s.", response.status_code,
        response.elapsed, entireUrl.c_str());

    LOG(LogLevel::DEBUG, "Host response text: %s", response.text.c_str());

    LOG(LogLevel::INFO, "Host response size: %lu", response.text.size());


    if (response.error) {
        LOG(LogLevel::WARNING, "Request error: %s", response.error.message.c_str());
    }
    // Return false if current node failed for request from following reasons.
    // Status code = 0 means, that it is not possible to connect to Elastic node.
    // Status code = 503 means that Elastic node is temporarily unavailable, maybe because of queue
    // capacity of node is full filled.
    if (response.status_code == 0 || response.status_code == 503) {
        LOG(LogLevel::WARNING, "Host on URL '%s' is unavailable.", entireUrl.c_str());
        return false;
    }
    return true;
}


cpr::Response Client::Implementation::performRequest(
        Client::HTTPMethod method, const std::string &urlPath, const std::string &body)
{
    bool isSuccessful = false;
    cpr::Response response;
    while (!isSuccessful) {
        isSuccessful = performRequestOnCurrentHost(method, urlPath, body, response);
        if (!isSuccessful && !failCurrentHostAndIterateNext()) {
            throw ConnectionException("All hosts failed for request.");
        }
    }
    // Reset failCounter if any host successfuly responds.
    failCounter = 0;
    return response;
}


cpr::Response Client::search(const std::string &indexName,
                             const std::string &docType,
                             const std::string &body,
                             const std::string &routing)
{
    std::ostringstream urlPath;
    fillIndexAndTypeInUrlPath(indexName, false, docType, false, urlPath);
    urlPath << "_search";
    fillRoutingInUrlPath(routing, urlPath);
    return impl->performRequest(HTTPMethod::POST, urlPath.str(), body);
}


cpr::Response Client::get(const std::string &indexName,
                          const std::string &docType,
                          const std::string &id,
                          const std::string &routing)
{
    std::ostringstream urlPath;
    fillIndexAndTypeInUrlPath(indexName, true, docType, true, urlPath);
    if (id.empty()) {
        throw std::runtime_error("Argument id can not be empty.");
    }
    urlPath << id;
    fillRoutingInUrlPath(routing, urlPath);
    return impl->performRequest(HTTPMethod::GET, urlPath.str());
}


cpr::Response Client::index(const std::string &indexName,
                            const std::string &docType,
                            const std::string &id,
                            const std::string &body,
                            const std::string &routing)
{
    std::ostringstream urlPath;
    fillIndexAndTypeInUrlPath(indexName, true, docType, true, urlPath);
    if (!id.empty()) {
        urlPath << id;
    }
    fillRoutingInUrlPath(routing, urlPath);
    return impl->performRequest(HTTPMethod::POST, urlPath.str(), body);
}


cpr::Response Client::remove(const std::string &indexName,
                             const std::string &docType,
                             const std::string &id,
                             const std::string &routing)
{
    std::ostringstream urlPath;
    fillIndexAndTypeInUrlPath(indexName, true, docType, true, urlPath);
    if (id.empty()) {
        throw std::runtime_error("Argument id can not be empty.");
    }
    urlPath << id;
    fillRoutingInUrlPath(routing, urlPath);
    return impl->performRequest(HTTPMethod::DELETE, urlPath.str());
}


void Client::Implementation::visit(const TimeoutOption &opt) {
    session.SetTimeout(cpr::Timeout{opt.getValue()});
}

void Client::Implementation::visit(const ConnectTimeoutOption &opt) {
    session.SetConnectTimeout(cpr::ConnectTimeout{opt.getValue()});
}

void Client::Implementation::visit(const ProxiesOption &opt) {
    session.SetProxies(opt.impl->getProxies());
}

void Client::Implementation::visit(const SSLOption &opt) {
    session.SetSslOptions(opt.impl->getOptions());
}

void Client::SSLOption::SSLOptionImplementation::visit(const CertFile &certFile) {
    sslOptions.SetOption(cpr::ssl::CertFile{std::string{certFile.path}});
}

void Client::SSLOption::SSLOptionImplementation::visit(const KeyFile &keyFile) {
    sslOptions.SetOption(cpr::ssl::KeyFile(keyFile.path, keyFile.password));
}

void Client::SSLOption::SSLOptionImplementation::visit(const CaInfo &caBundle) {
    sslOptions.SetOption(cpr::ssl::CaInfo(std::string{caBundle.path}));
}

void Client::SSLOption::SSLOptionImplementation::visit(const VerifyHost &opt) {
    sslOptions.SetOption(cpr::ssl::VerifyHost{opt.verify});
}

void Client::SSLOption::SSLOptionImplementation::visit(const VerifyPeer &opt) {
    sslOptions.SetOption(cpr::ssl::VerifyPeer{opt.verify});
}


}  // namespace elasticlient

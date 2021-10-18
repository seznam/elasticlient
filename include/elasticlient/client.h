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
#include <utility>
#include <initializer_list>
#include <type_traits>

#ifndef __GNUC__ 
#undef DELETE
#endif

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

    /// Abstract class for various options passed to Client constructor.
    struct ClientOption {
        virtual ~ClientOption() {}
        /**
         * Method to set option to the Client (internal impl).
         * Set (derived) option specific settings to the implementation.
         */
        virtual void accept(Implementation &) const = 0;
    };

    /// Helper class holding single value for ClientOption implementations.
    template <typename T>
    struct ClientOptionValue: public ClientOption {
        T value;

        explicit ClientOptionValue(const T& value): value(value) {}
        explicit ClientOptionValue(T&& value): value(std::move(value)) {}
        ClientOptionValue(ClientOptionValue &&) = default;
        virtual ~ClientOptionValue() = default;

        T getValue() const {
            return value;
        }
    };

    /// The timeout [ms] setting for client connection.
    struct TimeoutOption: public ClientOptionValue<std::int32_t> {
        explicit TimeoutOption(std::int32_t timeoutMs)
            : ClientOptionValue(timeoutMs) {}
      protected:
        void accept(Implementation &) const override;
    };

    /// The connection timeout [ms] for client.
    struct ConnectTimeoutOption: public ClientOptionValue<std::int32_t> {
        explicit ConnectTimeoutOption(std::int32_t timeoutMs)
            : ClientOptionValue(timeoutMs) {}
      protected:
        void accept(Implementation &) const override;
    };

    /// Proxies option for client connection.
    struct ProxiesOption: public ClientOption {
        /// Implementation hidden from public interface.
        class ProxiesOptionImplementation;
        std::unique_ptr<ProxiesOptionImplementation> impl;

        /**
         * Set proxies to the client connection.
         * \param proxies List of proxies consisting of the pair <protocol, proxy_url>.
         */
        explicit ProxiesOption(
                const std::initializer_list<std::pair<const std::string, std::string>> &proxies);
        ~ProxiesOption();
      protected:
        void accept(Implementation &) const override;
    };

    /// Options to setup SSL for client connection.
    struct SSLOption: public ClientOption {
        /// Implementation hidden from public interface.
        class SSLOptionImplementation;
        std::unique_ptr<SSLOptionImplementation> impl;

        SSLOption();
        SSLOption(SSLOption &&);
        ~SSLOption();

        /**
         * Construct SSLOptions with various SSLOptionType options.
         * All arguments passed to it must be base of SSLOptionType.
         * If same types are passed, the last one overwrites preceeding one.
         */
        template <typename... T>
        SSLOption(T... args): SSLOption() {
            optSetter(std::move(args)...);
        }

        /// Abstract base class for specific SSL options.
        struct SSLOptionType {
            virtual ~SSLOptionType() {}
            /// Visitor initiation to setup specific option by derived classes.
            virtual void accept(SSLOptionImplementation &) const = 0;
        };

        /// Path to the certificate.
        struct CertFile: public SSLOptionType {
            explicit CertFile(std::string path): path(std::move(path)) {}
            void accept(SSLOptionImplementation &) const override;
            std::string path;
        };

        /// Path to the certificate key file.
        struct KeyFile: public SSLOptionType {
            explicit KeyFile(std::string path, std::string password = {})
                : path(std::move(path)), password(std::move(password))
            {}
            void accept(SSLOptionImplementation &) const override;
            std::string path;
            std::string password;
        };

        /// Path to the custom CA bundle.
        struct CaInfo: public SSLOptionType {
            explicit CaInfo(std::string path): path(std::move(path)) {}
            void accept(SSLOptionImplementation &) const override;
            std::string path;
        };

        /// Flag whether to verify host.
        struct VerifyHost: public SSLOptionType {
            explicit VerifyHost(bool verify): verify(verify) {}
            void accept(SSLOptionImplementation &) const override;
            bool verify;
        };

        /// Flag whether to verify peer.
        struct VerifyPeer: public SSLOptionType {
            explicit VerifyPeer(bool verify): verify(verify) {}
            void accept(SSLOptionImplementation &) const override;
            bool verify;
        };

        /// Set single option - see SSLOptionType derived structs above.
        void setSslOption(const SSLOptionType &);

      protected:
        void accept(Implementation &) const override;

        /// Helper method to setup ssl options with SSLOptionType instances.
        template <typename T>
        void optSetter(T&& t) {
            static_assert(std::is_base_of<SSLOptionType, T>::value,
                          "Record must be derived from SSLOptionType");
            setSslOption(std::forward<T>(t));
        }

        /// Helper method to setup ssl options with SSLOptionType instances.
        template<typename T, typename... TRest>
        void optSetter(T&& t, TRest&&... ts) {
            static_assert(std::is_base_of<SSLOptionType, T>::value,
                          "Record must be derived from SSLOptionType");
            optSetter(std::forward<T>(t));
            optSetter(std::move(ts)...);
        }
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

    /**
     * Initialize the Client.
     *
     * Variadic arguments derived from ClientOption can be passed to it.
     * The later option overwrites the preceeding one for same object types passed into it.
     * \param hostUrlList   Vector of URLs of Elasticsearch nodes in one Elasticsearch cluster.
     *                      Each URL in vector should end by "/".
     */
    template <typename... Opts>
    Client(const std::vector<std::string> &hostUrlList,
           Opts&&... opts)
        : Client(hostUrlList)
    {
        optionConstructHelper(std::move(opts)...);
    }

    Client(Client &&);

    ~Client();

    /**
     * Set single option derived from ClientOption to the client.
     * \param opt Option to set. Has to be base of ClientOption.
     */
    void setClientOption(const ClientOption &opt);

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
  private:
    /// Helper method to setup client with ClientOption options.
    template <typename T>
    void optionConstructHelper(T&& opt) {
        static_assert(std::is_base_of<ClientOption, T>::value,
                      "Record must be derived from ClientOption");
        setClientOption(std::forward<T>(opt));
    }

    /// Helper method to setup client with ClientOption options.
    template <typename T, typename... TRest>
    void optionConstructHelper(T&& opt, TRest&&... rest) {
        static_assert(std::is_base_of<ClientOption, T>::value,
                      "Record must be derived from ClientOption");
        optionConstructHelper(std::forward<T>(opt));
        optionConstructHelper(std::move(rest)...);
    }
};


} // namespace elasticlient

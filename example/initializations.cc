/**
 * \file
 * Example elasticlient initialization.
 */

#include <string>
#include <vector>
#include <iostream>
#include <cpr/response.h>
#include <elasticlient/client.h>
#include <elasticlient/logging.h>


int main() {
    // Setup logging from the elasticlient library if you want to know what is happening
    elasticlient::setLogFunction([](elasticlient::LogLevel, const std::string &msg) {
        std::cerr << msg << "\n";
    });

    elasticlient::Client::SSLOption sslOptions {
            elasticlient::Client::SSLOption::VerifyHost{false},
            elasticlient::Client::SSLOption::VerifyPeer{false},
            elasticlient::Client::SSLOption::CaInfo{"myca.pem"},
            elasticlient::Client::SSLOption::CertFile{"mycert.pem"},
            elasticlient::Client::SSLOption::KeyFile{"mycert-key.pem"}};

    // Prepare Client for nodes of one Elasticsearch cluster
    // various options could be passed into it
    elasticlient::Client client(
            {"http://elastic1.host:9200/"},
            elasticlient::Client::TimeoutOption{30000},
            elasticlient::Client::ConnectTimeoutOption{1000},
            sslOptions,
            elasticlient::Client::ProxiesOption(
                    {{"http", "http://proxy.host:8080"},
                     {"https", "https://proxy.host:8080"}})
    );

    // or you can set options later one by one
    client.setClientOption(elasticlient::Client::TimeoutOption{30000});

    // and you can use client same as shown in hello-world.cc example...
    cpr::Response retrievedDocument = client.get("testindex", "docType", "docId");
}

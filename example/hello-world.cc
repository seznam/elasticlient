/**
 * \file
 * Example program using elasticlient library.
 */

#include <string>
#include <vector>
#include <iostream>
#include <cpr/response.h>
#include <elasticlient/client.h>


int main() {
    // Prepare Client for nodes of one Elasticsearch cluster
    elasticlient::Client client({"http://elastic1.host:9200/"}); // last / is mandatory

    // Or if you'd like to use proxy
    //elasticlient::Client client({"http://elastic1.host:9200/"}, 6000,
    //        {{"http", "http://proxy.host:8080"},{"https", "https://proxy.host:8080"}});

    std::string document {"{\"message\": \"Hello world!\"}"};

    // Index the document, index "testindex" must be created before
    cpr::Response indexResponse = client.index("testindex", "docType", "docId", document);
    // 200
    std::cout << indexResponse.status_code << std::endl;
    // application/json; charset=UTF-8
    std::cout << indexResponse.header["content-type"] << std::endl;
    // Elasticsearch response (JSON text string)
    std::cout << indexResponse.text << std::endl;

    // Retrieve the document
    cpr::Response retrievedDocument = client.get("testindex", "docType", "docId");
    // 200
    std::cout << retrievedDocument.status_code << std::endl;
    // application/json; charset=UTF-8
    std::cout << retrievedDocument.header["content-type"] << std::endl;
    // Elasticsearch response (JSON text string) where key "_source" contain:
    // {"message": "Hello world!"}
    std::cout << retrievedDocument.text << std::endl;

    // Remove the document
    cpr::Response removedDocument = client.remove("testindex", "docType", "docId");
    // 200
    std::cout << removedDocument.status_code << std::endl;
    // application/json; charset=UTF-8
    std::cout << removedDocument.header["content-type"] << std::endl;
    // Elasticsearch response (JSON text string)
    std::cout << removedDocument.text << std::endl;

    return 0;
}

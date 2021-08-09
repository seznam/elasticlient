# C++ elasticlient
C++ elasticlient library is simple library for simplified work with Elasticsearch in C++.
The library is based on [C++ Requests: Curl for People](https://github.com/whoshuu/cpr).

## Features
* Elasticsearch client which work with unlimited nodes in one Elasticsearch cluster. If any node is dead it tries another one.
* Elasticsearch client supports search, index, get, remove methods by default.
* Posibility to perform not implemented method i.e multi GET or indices creation.
* Support for Bulk API requests.
* Support for Scroll API.

## Dependencies
* [C++ Requests: Curl for People](https://github.com/whoshuu/cpr)
* [RapidJSON](http://rapidjson.org/)
* [Google Test](https://github.com/google/googletest)
* Only for tests: [C++ HTTP mock server library](https://github.com/seznam/httpmockserver)

## Requirements
* C++11 compatible compiler such as GCC (tested with version 4.9.2)
* [CMake](http://www.cmake.org/) (tested with version 3.5.2)

## Building and testing on Unix like system

###### Step 1
Clone or download this repository to some directory and go to this directory.
```sh
git clone https://github.com/seznam/elasticlient
cd elasticlient
```

###### Step 2
Now if you want to use all above mentioned dependencies from system, you can skip this step.
```sh
git submodule update --init --recursive
```

###### Step 3
Build the library:
```sh
mkdir build
cd build
cmake ..
make
make test  # Optional, will run elasticlient tests
```
Following CMake configuration variables may be passed right before `..` in `cmake ..` command.
* `-DUSE_ALL_SYSTEM_LIBS=YES`  - use all dependencies from system (default=NO)
* `-DUSE_SYSTEM_CPR=YES`  - use C++ Requests library from system (default=NO)
* `-DUSE_SYSTEM_RAPIDJSON=YES`  - use RapidJson library from system (default=NO)
* `-DUSE_SYSTEM_GTEST=YES`  - use Google Test library from system (default=NO)
* `-DUSE_SYSTEM_HTTPMOCKSERVER=YES`  - use C++ HTTP mock server library from system (default=NO)
* `-DBUILD_ELASTICLIENT_TESTS=YES`  - build elasticlient library tests (default=YES)
* `-DBUILD_ELASTICLIENT_EXAMPLE=YES`  - build elasticlient library example hello-world program (default=YES)
* `-DBUILD_SHARED_LIBS=YES`  - build as a shared library (default=YES)

## How to use
###### Basic Hello world example
If you build the library with `-DBUILD_ELASTICLIENT_EXAMPLE=YES` you can find compiled binary
(hello-world) of this example in `build/bin` directory.

This example will at first index the document into elasticsearch cluster. After that it will
retrieve the same document and on the end it will remove the document.

```cpp
#include <string>
#include <vector>
#include <iostream>
#include <cpr/response.h>
#include <elasticlient/client.h>


int main() {
    // Prepare Client for nodes of one Elasticsearch cluster
    elasticlient::Client client({"http://elastic1.host:9200/"}); // last / is mandatory

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
```

###### Usage of Bulk indexer
```cpp
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <elasticlient/client.h>
#include <elasticlient/bulk.h>


int main() {
    // Prepare Client for nodes of one Elasticsearch cluster
    std::shared_ptr<elasticlient::Client> client = std::make_shared<elasticlient::Client>(
        std::vector<std::string>({"http://elastic1.host:9200/"}));  // last / is mandatory
    elasticlient::Bulk bulkIndexer(client);

    elasticlient::SameIndexBulkData bulk("testindex", 100);
    bulk.indexDocument("docType", "docId0", "{\"data\": \"data0\"}");
    bulk.indexDocument("docType", "docId1", "{\"data\": \"data1\"}");
    bulk.indexDocument("docType", "docId2", "{\"data\": \"data2\"}");
    // another unlimited amount of indexDocument() calls...

    size_t errors = bulkIndexer.perform(bulk);
    std::cout << "When indexing " << bulk.size() << " documents, "
              << errors << " errors occured" << std::endl;
    bulk.clear();
    return 0;
}
```

###### Usage of Scroll API
```cpp
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <json/json.h>
#include <elasticlient/client.h>
#include <elasticlient/scroll.h>


int main() {
    // Prepare Client for nodes of one Elasticsearch cluster
    std::shared_ptr<elasticlient::Client> client = std::make_shared<elasticlient::Client>(
        std::vector<std::string>({"http://elastic1.host:9200/"}));  // last / is mandatory
    elasticlient::Scroll scrollInstance(client);

    std::string searchQuery{"{\"sort\": [\"_doc\"],"
                            " \"query\": {"
                            "   \"prefix\": {"
                            "     \"data\": \"data\"}}}"};

    // Scroll all documents of type: docType from testindex which corresponding searchQuery
    scrollInstance.init("testindex", "docType", searchQuery);

    Json::StyledWriter writer;

    Json::Value res;
    bool isSuccessful = true;
    // Will scroll for all suitable documents
    while ((isSuccessful = scrollInstance.next(res))) {
        if (res["hits"].empty()) {
            // last scroll, no more results
            break;
        }
        std::cout << writer.write(res["hits"]) << std::endl;
    }

    scrollInstance.clear();
}

```

## How to use logging feature (debugging)
```cpp
#include <iostream>
#include <elasticlient/client.h>
#include <elasticlient/logging.h>


/// Very simple log callback (only print message to stdout)
void logCallback(elasticlient::LogLevel logLevel, const std::string &msg) {
    if (logLevel != elasticlient::LogLevel::DEBUG) {
        std::cout << "LOG " << (unsigned) logLevel << ": " << msg << std::endl;
    }
}


int main() {
    // Set logging function for elasticlient library
    elasticlient::setLogFunction(logCallback);

    // Elasticlient will now print all debug messages on stdout.
    // It is necessary to set logging function for elasticlient for each thread where you want
    // use this logging feature!
}
```

## Elasticlient connection settings

To setup various settings for the connection, option arguments can be passed into constructor.
```cpp
    // Prepare Client for nodes of one Elasticsearch cluster.
    // Various options could be passed into it - vector of the cluster nodes must be first
    // but the rest of the arguments is order independent, and each is optional.
    elasticlient::Client client(
            {"http://elastic1.host:9200/"},
            elasticlient::Client::TimeoutOption{30000},
            elasticlient::Client::ConnectTimeoutOption{1000},
            elasticlient::Client::ProxiesOption(
                    {{"http", "http://proxy.host:8080"},
                     {"https", "https://proxy.host:8080"}})
    );

    // each of these options can be set later as well
    elasticlient::Client::SSLOption sslOptions {
            elasticlient::Client::SSLOption::VerifyHost{true},
            elasticlient::Client::SSLOption::VerifyPeer{true},
            elasticlient::Client::SSLOption::CaInfo{"myca.pem"},
            elasticlient::Client::SSLOption::CertFile{"mycert.pem"},
            elasticlient::Client::SSLOption::KeyFile{"mycert-key.pem"}};
    client.setClientOption(std::move(sslOptions));
    client.setClientOption(elasticlient::Client::TimeoutOption{300000});
```

Currently supported options:
* `Client::TimeoutOption` - HTTP request timeout in ms.
* `Client::ConnectTimeoutOption` - Connect timeout in ms.
* `Client::ProxiesOption` - Proxy server settings.
* `Client::SSLOption`
  * `Client::SSLOption::CertFile` - path to the SSL certificate file.
  * `Client::SSLOption::KeyFile` - path to the SSL certificate key file.
  * `Client::SSLOption::CaInfo` - path to the CA bundle if custom CA is used.
  * `Client::SSLOption::VerifyHost` - verify the certificate's name against host.
  * `Client::SSLOption::VerifyPeer` - verify the peer's SSL certificate.

## License
Elasticlient is licensed under the MIT License (MIT).

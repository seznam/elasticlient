/**
 * \file
 * \brief Scroll parser implemented using rapidjson library.
 *
 * Parsing of JSON result exposed due to header-only rapidjson library.
 * Result parsing could be completely hidden inside elasticlient implementation
 * thus resulting in necessity to parse result twice - for error detection and
 * scrollId retrieval, and later by the client code to get the data.
 * Or it can be (and it is) exposed, so the client code compiles exactly same
 * rapidjson::Document class for result parsing and for result hits processing.
 */

#pragma once

#include <string>
#include <rapidjson/document.h>
#include <elasticlient/scroll.h>


namespace elasticlient {


/**
 * Parse scroll result and set it into \p parsedResult.
 *
 * \note Function implemented in public header to assure binary compatibility
 * of parsed rapidjson document. Otherwise there would be a risk of using binary
 * incompatible versions inside elasticlient.so and the client's side...
 *
 * \param result        Scroll result (json).
 * \param parsedResult  Parsed json result.
 * \param scrollId      ScrollId read from the result.
 * \return true on success.
 */
bool parseScrollResult(
        const std::string &result, JsonResult &parsedResult,
        std::string &scrollId)
{
    rapidjson::Document &document = parsedResult.document;
    rapidjson::ParseResult ok = document.Parse(result.c_str());
    if (!ok || !document.IsObject()) {
        // something is weird, only report error
        return false;
    }

    if (document.HasMember("error")) {
        const rapidjson::Value &err = document["error"];
        if (!err.IsBool() || err.GetBool()) {
            // an error was reported
            return false;
        }
    }

    if (document.HasMember("timed_out")) {
        const rapidjson::Value &doc = document["timed_out"];
        if (!doc.IsBool() || doc.GetBool()) {
            // request timed out
            return false;
        }
    }

    if (document.HasMember("_shards") && document["_shards"].IsObject()) {
        const rapidjson::Value &shards = document["_shards"];
        if (shards.HasMember("failed") && shards["failed"].IsInt()) {
            const int failed = shards["failed"].GetInt();
            if (failed > 0) {
                // failed shards, do not trust the data
                return false;
            }
        } else {
            // no information about failed shards, but it has to be here
            return false;
        }
    } else {
        // no information about shards, but it has to be here
        return false;
    }

    if (document.HasMember("hits")) {
        const rapidjson::Value &hits = document["hits"];
        // just make sure the hits array is present
        if (hits.HasMember("hits") && hits["hits"].IsArray()) {
            if (document.HasMember("_scroll_id") && document["_scroll_id"].IsString()) {
                scrollId = document["_scroll_id"].GetString();
                return true;
            }
            // missing _scroll_id, is that scroll response?
            return false;
        }
    }

    // scroll response is not ok
    return false;
}


}  // namespace elasticlient

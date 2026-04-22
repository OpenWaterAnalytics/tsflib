//
//  NexusClient.hpp
//  tsflib
//
//  oatpp ApiClient for the Nexus Integra public REST API v1.
//

#ifndef NexusClient_h
#define NexusClient_h

#include "oatpp/web/client/ApiClient.hpp"
#include "oatpp/core/macro/codegen.hpp"

#include OATPP_CODEGEN_BEGIN(ApiClient)

class NexusClient : public oatpp::web::client::ApiClient {

private:
  constexpr static const char* HTTP_GET  = "GET";
  constexpr static const char* HTTP_POST = "POST";

public:
  API_CLIENT_INIT(NexusClient)

  // ── Tag discovery ──────────────────────────────────────────────
  // GET /api/Tags  →  [ {installation, driver, name, uid} ]
  API_CALL(HTTP_GET, "{nexusApi}/Tags", getTags,
           PATH(String, nexusApi, "nexusApi"),
           HEADER(String, nexustoken, "nexustoken"),
           HEADER(String, graviteeApiKey, "X-Gravitee-Api-Key"))

  // ── Tag creation ───────────────────────────────────────────────
  // POST /api/Tags/insert  body: ["tagname"]  →  [ {installation, driver, name, uid} ]
  API_CALL(HTTP_POST, "{nexusApi}/Tags/insert", insertTags,
           PATH(String, nexusApi, "nexusApi"),
           HEADER(String, nexustoken, "nexustoken"),
           HEADER(String, graviteeApiKey, "X-Gravitee-Api-Key"),
           BODY_STRING(String, body))

  // ── Historic read ──────────────────────────────────────────────
  // POST /api/Tags/historic  body: NexusRequest  →  [ NexusValue ]
  API_CALL(HTTP_POST, "{nexusApi}/Tags/historic", getHistoric,
           PATH(String, nexusApi, "nexusApi"),
           HEADER(String, nexustoken, "nexustoken"),
           HEADER(String, graviteeApiKey, "X-Gravitee-Api-Key"),
           BODY_STRING(String, body))

  // ── Historic write ─────────────────────────────────────────────
  // POST /api/Tags/historic/insert  body: [ NexusValue ]  →  true
  API_CALL(HTTP_POST, "{nexusApi}/Tags/historic/insert", insertHistoric,
           PATH(String, nexusApi, "nexusApi"),
           HEADER(String, nexustoken, "nexustoken"),
           HEADER(String, graviteeApiKey, "X-Gravitee-Api-Key"),
           BODY_STRING(String, body))

  // ── Realtime read ──────────────────────────────────────────────
  // POST /api/Tags/realtime  body: ["uid"]  →  [ NexusValue ]
  API_CALL(HTTP_POST, "{nexusApi}/Tags/realtime", getRealtime,
           PATH(String, nexusApi, "nexusApi"),
           HEADER(String, nexustoken, "nexustoken"),
           HEADER(String, graviteeApiKey, "X-Gravitee-Api-Key"),
           BODY_STRING(String, body))

#include OATPP_CODEGEN_END(ApiClient)
};

#endif /* NexusClient_h */


//
// PTC Tempo Automation API sample client
//
// Copyright © 2023 Bio-Rad Laboratories Inc. All rights Reserved
//
// MIT License
//
// SPDX-License-Identifier: MIT
//

#include "nlohmann/json.hpp"
#include <iostream>

// define CPPHTTPLIB_OPENSSL_SUPPORT is set as an option in the CMakeLists.txt
#include "httplib.h"

using nlohmann::json;

/**
 * @brief The TempoClient class makes calls to PTC-Tempo via the HTTP library.
 *
 * It owns the httplib::Client and httplib::Result. There is only one of each of these objects
 * because the client app makes a single call to PTC-Tempo. If you need to make multiple calls
 * to PTC-Tempo concurrently, you can create multiple instances of TempoClient.
 *
 * @par Blocking Calls
 * All methods that make HTTP calls are blocking calls, meaning that they will not return until
 * either the waitTime has expired or it received a response. If your app cannot allow blocking
 * calls in a thread, you should place that function call into a worker thread.
 */
class TempoClient {

    /// Number of spaces for indenting JSON and text output.
    static const int indent = 2;
    const int32_t versionMajor = 1;
    const int32_t versionMinor = 0;
    const int32_t versionPatch = 0;

    /// Manages all http calls to PTC-Tempo.
    httplib::Client httpClient;

    /// Response from a call to PTC-Tempo.
    httplib::Result httpResult{nullptr, httplib::Error::Unknown, httplib::Headers()};

public:

    /**
     * @brief Creates a client connection to PTC-Tempo.
     * @param host URL for PTC-Tempo.
     * @param password Plaintext password for Automation user on PTC-Tempo.
     * @param waitTime Number of seconds to wait for a response.
     *
     * After the constructor is called, the host object may be used to make HTTP calls; no
     * need to add HTTP headers or set up further authorization.
     */
    TempoClient(const std::string& host, const std::string& password, int32_t waitTime) : httpClient(host) {
        httpClient.set_basic_auth("Automation", password);
        httpClient.set_read_timeout(time_t(waitTime));
#if defined(CPPHTTPLIB_OPENSSL_SUPPORT)
        httpClient.enable_server_certificate_verification(false);
#endif
    }

    /**
     * @brief Makes a get call to the tempo endpoint.
     *
     * This endpoint can be called to check if PTC-Tempo's API is available. If it is available,
     * the response status will be 200, otherwise it may timeout. Response will be stored inside
     * the httpResult data member. This is a blocking call. It will not return until either the
     * waitTime has expired or it received a response.
     */
    void tempo() {
        httpResult = httpClient.Get("/tempo");
    }

    /**
     * @brief Sends request to open PTC Tempo lid.
     *
     * This is a blocking call. It will not return until either the waitTime has expired or it received a response.
     */
    void openLid() {
        httpResult = httpClient.Put("/tempo/lid/open");
    }

    /**
     * @brief Sends request to close PTC Tempo lid.
     *
     * This is a blocking call. It will not return until either the waitTime has expired or it received a response.
     */
    void closeLid() {
        httpResult = httpClient.Put("/tempo/lid/close");
    }

    /**
     * @brief Requests current lid status.
     *
     * This is a blocking call. It will not return until either the waitTime has expired or it received a response.
     */
    void lid() {
        httpResult = httpClient.Get("/tempo/lid");
    }

    /**
     * @brief Requests current instrument status.
     *
     * This is a blocking call. It will not return until either the waitTime has expired or it received a response.
     */
    void status() {
        httpResult = httpClient.Get("/tempo/status");
    }

    /**
     * @brief Either returns list of thermal cycler and lid faults, or clears list
     * of current faults.
     *
     * This is a blocking call. It will not return until either the waitTime has expired or it received a response.
     * @param clearFaults True to clear list of current faults, false to get list of faults.
     */
    void faults(bool clearFaults) {
        httpResult = clearFaults ?
            httpClient.Put("/tempo/errors/clear") :
            httpClient.Get("/tempo/errors");
    }

    /**
     * @brief Retrieve a list of protocols from PTC Tempo.
     *
     * This is a blocking call. It will not return until either the waitTime has expired or it received a response.
     * @param publicProtocols True if caller wants list of public protocols, false to get list of
     *  user's protocols. Response will be stored inside the httpResult data member.
     */
    void protocols(bool publicProtocols) {

        if (publicProtocols) {
            httpResult = httpClient.Get("/tempo/protocols/public");

        } else {
            httpResult = httpClient.Get("/tempo/protocols/user");
        }
    }

    /**
     * @brief Gets a list of run reports from PTC Tempo.
     *
     * This is a blocking call. It will not return until either the waitTime has expired or it received a response.
     * @param limit Number of run reports to retrieve.
     * @param offset Index into number of run reports. Can range from 0 to count, where count is total number of run reports.
     */
    void reports(int64_t limit = 0, int64_t offset = 0) {
        if (limit <= 0 && offset <= 0) {
            httpResult = httpClient.Get("/tempo/run-reports");
        } else if (limit > 0 && offset == 0) {
            httpResult = httpClient.Get("/tempo/run-reports?limit=" + std::to_string(limit));
        } else if (limit == 0) {
            httpResult = httpClient.Get("/tempo/run-reports?offset=" + std::to_string(offset));
        } else {
            httpResult = httpClient.Get("/tempo/run-reports?limit=" + std::to_string(limit) + "&offset=" + std::to_string(offset));
        }
    }

    /**
     *  @brief Prints the versions of the app and the Automation API and checks for compatiblility.
     *
     * @param displayFormat
     * @return False if major versions are not compatible.
     */
    bool version(std::string_view displayFormat = "json") {
        tempo();

        json versionJson;
        int32_t apiVersionMajor = 0;
        int32_t exitCode = 0;

        if (httpResult.error() != httplib::Error::Success) {
            std::cerr << "HTTP client error: " << httplib::to_string(httpResult.error()) << std::endl;
            versionJson["httpCode"] = 504;
            exitCode = static_cast<int32_t>(httpResult.error());

        } else {
            versionJson["httpCode"] = httpResult->status;
            if (httpResult->status == 200) {
                try {
                    json response = json::parse(httpResult->body);
                    auto device = response["device"];
                    auto details = device["details"];
                    std::string apiVersion = details["automationAPI"];

                    if (auto position = apiVersion.find('.'); position > 0) {
                        std::stringstream(apiVersion.substr(0, position)) >> apiVersionMajor;
                    }
                    versionJson["automationAPI"] = apiVersion;
                    if (versionMajor < apiVersionMajor) {
                        versionJson["error"] = "incompatible";
                        exitCode = 1;
                    }

                } catch (json::exception& ex) {
                    std::cerr << ex.what() << std::endl;
                    exitCode = 1;
                }
            }
        }
        std::string version = std::to_string(versionMajor) + '.' + std::to_string(versionMinor) + '.' +std::to_string(versionPatch);
        versionJson["version"] = version;
        std::string responseResult = versionJson.dump(indent);
        if (displayFormat == "text") {
            responseResult = formatResponseForTextDisplay(responseResult);
        }
        std::cout << responseResult << std::endl;

        return exitCode == 0;
    }

    /**
     * @brief Gets total number of run reports.
     *
     * This is a blocking call. It will not return until either the waitTime has expired or it received a response.
     */
    void reportsCount() {
        httpResult = httpClient.Get("/tempo/run-reports/count");
    }

    /**
     * @brief Gets a specific run report by ID.
     * @param runId Which report to obtain. Value should be a GUID obtained from list of run reports.
     */
    void reports(const std::string& runId) {
        httpResult = httpClient.Get("/tempo/run-reports/"+runId);
    }

    /**
     * @brief Gets status of currently active protocol run.
     *
     * This is a blocking call. It will not return until either the waitTime has expired or it received a response.
     */
    void run() {
        httpResult = httpClient.Get("/tempo/protocol-run");
    }

    /**
     * @brief Starts a new protocol run on PTC Tempo.
     *
     * This is a blocking call. It will not return until either the waitTime has expired or it received a response.
     * @param runInfo Contains parameters for new run. Parameters and values are in json format.
     */
    void run(const json& runInfo) {
        std::string body = runInfo.dump();
        const std::string contentType = "application/json";
        httpResult = httpClient.Post("/tempo/protocol-run", body.c_str(), body.length(), contentType);
    }

    /**
     * @brief Sends request to stop current protocol run.
     *
     * This is a blocking call. It will not return until either the waitTime has expired or it received a response.
     */
    void stop() {
        httpResult = httpClient.Put("/tempo/protocol-run/stop");
    }

    /**
     * @brief Sends request to skip current step of protocol run.
     *
     * This is a blocking call. It will not return until either the waitTime has expired or it received a response.
     */
    void skip() {
        httpResult = httpClient.Put("/tempo/protocol-run/skip");
    }

    /**
     * @brief Sends request to pause current protocol run.
     *
     * This is a blocking call. It will not return until either the waitTime has expired or it received a response.
     */
    void pause() {
        httpResult = httpClient.Put("/tempo/protocol-run/pause");
    }

    /**
     * @brief Sends request to resume a paused protocol run.
     *
     * This is a blocking call. It will not return until either the waitTime has expired or it received a response.
     */
    void resume() {
        httpResult = httpClient.Put("/tempo/protocol-run/resume");
    }

    /**
     * @brief This function obtains the status value from a JSON input stirng.
     *
     * It is used to monitor the run status.
     * @return String containing run status.
     */
    std::string getRunStatus() {
        json response = json::parse(httpResult->body);
        return response["status"];
    }

    /**
     * @brief This function obtains the lid status from a JSON input stirng.
     *
     * @return String containing lid status.
     */
    std::string getLidStatus() {
        json response = json::parse(httpResult->body);
        return response["lid"];
    }

    /// Returns true if there are not HTTP result errors and the response status is 200.
    bool statusOK() {
        return httpResult.error() == httplib::Error::Success && (httpResult->status == 200);
    }

    /**
     * @brief Prints response body to terminal output.
     *
     * This prints out a JSON object to stdout. If there are any exceptions in parsing the JSON
     * input string, it sends the exception message to stderr. If the response body is empty, this
     * prints out an empty JSON object.
     * @param responseResult Response body in JSON format.
     * @param displayFormat Output format requested by user; either "text" or "json".
     * @return True for success, false if an exception occurred.
     */
    bool responseString(std::string& responseResult, std::string_view displayFormat = "json") {
        if (httpResult->body.empty()) {
            httpResult->body = "{}";
        }
        try {
            json response = json::parse(httpResult->body);
            response["httpCode"] = 200;
            responseResult = response.dump(indent);
            if (displayFormat == "text") {
                responseResult = formatResponseForTextDisplay(responseResult);
            }
        }  catch (json::exception& ex) {
            std::cerr << ex.what() << std::endl;
            return false;
        }
        return true;
    }

    /**
     * @brief Print response body if response status is 200, otherwise send error message to stderr.
     * @param displayFormat Output format requested by user; either "text" or "json".
     * @return True for success, false if response status is not 200 or an error occurred.
     */
    bool print(const std::string& displayFormat = "json") {

        if (httpResult.error() != httplib::Error::Success) {
            std::cerr << "HTTP client error: " << httplib::to_string(httpResult.error()) << std::endl;
            exit(static_cast<int>(httpResult.error()));

        } else if (httpResult->status == 200) {
            std::string responseResult;

            if (!responseString(responseResult, displayFormat)) {
                return false;
            }
            std::cout << responseResult << std::endl;
        } else {
            std::cerr << "HTTP error: " << httpResult->status << std::endl;
            exit(httpResult->status);
        }
        return true;
    }

    static std::string& formatResponseForTextDisplay(std::string& res) {
        std::istringstream stream(res);
        std::ostringstream result;
        std::string line;
        bool lastBlank = false;
        int16_t lastIndent = 0;
        while (std::getline(stream, line)) {
            auto pos = line.find_first_not_of(' ');
            int isBlank = (line[pos] == '{' || line[pos] == '}' || line[pos] == ']');

            if (!isBlank) {
                const std::string notPrint = "[{\",";
                line.erase(remove_if(line.begin(), line.end(), [&notPrint](const char& c) {
                    return notPrint.find(c) != std::string::npos;
                }), line.end());

                if (lastBlank && lastIndent == static_cast<int16_t>(pos)) {
                    // blank line between each entity in a list
                    result << std::endl;
                }
                lastIndent = static_cast<int16_t>(pos);
                result << line << std::endl;
            }
            lastBlank = isBlank;
        }
        res = result.str();
        return res;
    }
};

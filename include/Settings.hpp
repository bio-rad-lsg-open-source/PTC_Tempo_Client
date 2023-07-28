//
// PTC Tempo Automation API sample client
//
// Copyright © 2023 Bio-Rad Laboratories Inc. All rights Reserved
//
// MIT License
//
// SPDX-License-Identifier: MIT
//

#include <string>

/**
 * @struct Settings
 * @brief Stores values set by command line args.
 *
 * Values provided on command line override those in config file. This struct does not perform any
 * validation to check if values are valid.
 */
struct Settings {
    // instrument
    std::string host;                ///< URL for PTC Tempo.
    std::string password;            ///< Password for Automation user on PTC Tempo.
    int64_t waitTime = 10;           ///< Number of seconds to wait for response.

    // faults
    bool clearFaults = false;        ///< True to clear all cycler and lid faults.

    // reports
    std::string runId;               ///< ID of single run report to retrieve.
    int64_t limit = 0;               ///< Number of run reports to retrieve.
    int64_t offset = 0;              ///< Offset into list of run reports. Can range from 0 to count.
    bool countReports = false;       ///< True to get count of run reports.

    // run
    std::string protocol;            ///< Name of protocol.
    std::string runName;             ///< Name of run.
    int64_t volume = 0;              ///< Overrides volume within protocol.
    int64_t lidTemp = 0;             ///< Overrides lid temperature within protocol.
    std::string plateID;             ///< User provided plate-ID for run.
    bool publicProtocols = false;    ///< True to load protocol from public folder.
    bool templateProtocol = false;   ///< True to use template instead of protocol.

    // monitoring and displaying
    bool monitor = false;            ///< True to monitor responses from PTC Tempo.
    int64_t interval = 1;            ///< Number of seconds for polling interval when monitoring.
    std::string displayType;         ///< Output format: either json or text.
};

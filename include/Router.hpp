//
// PTC Tempo Automation API sample client
//
// Copyright © 2023 Bio-Rad Laboratories Inc. All rights Reserved
//
// MIT License
//
// SPDX-License-Identifier: MIT
//

#include "Config.hpp"
#include "Monitor.hpp"

using nlohmann::json;

/** @class Router
 *  @brief Manages most other objects in client app.
 *
 *  The Router
 *  - determines which PTC Tempo endpoints to call,
 *  - prepares requests for those endpoints based on command line options,
 *  - checks which command line options require which other options,
 *  - checks which command line options exclude other options,
 *  - manages the Config object to store/retrieve configuration values,
 *  - and calls Monitor object for repeatedly checking status of PTC Tempo.
 */
class Router {
    CLI::App* lidCommand;       ///< Contains subcommand to get lid status.
    CLI::App* openCommand;      ///< Contains subcommand to open lid.
    CLI::App* closeCommand;     ///< Contains subcommand to close lid.
    CLI::App* statusCommand;    ///< Contains status subcommand and options.
    CLI::App* faultCommand;     ///< Contains errors subcommand and options.
    CLI::App* reportsCommand;   ///< Contains reports subcommand and options.
    CLI::App* protocolsCommand; ///< Contains protocols subcommand and options.
    CLI::App* runCommand;       ///< Contains subcommand and options to get run status or run protocol.
    CLI::App* configCommand;    ///< Contains subcommand to update/print config file.
    CLI::App* licenseCommand;   ///< Contains subcommand to print license info.
    CLI::App* versionCommand;   ///< Contains subcommand to print version info.

    CLI::App* stopCommand;      ///< Contains subcommand to stop currently active protocol run.
    CLI::App* skipCommand;      ///< Contains subcommand to skip currently active step.
    CLI::App* pauseCommand;     ///< Contains subcommand to pause currently active run.
    CLI::App* resumeCommand;    ///< Contains subcommand to resume a protocol run.

    Settings settings;          ///< Stores values from config file and command line options.
    Config tempoConfig;         ///< Manages config file.

    /// Root command line arg handler for client app.
    CLI::App tempo = CLI::App("PTC Tempo command line interface to Automation API");

    /**
     * @brief Handles all commands related to run reports.
     *
     * This determines which TempoClient function to call for what run report info the user wants.
     * - If the user provides a specific run report ID, this calls the method to get a single report.
     * - If the user requests a count, this calls the method to get run report count.
     * - If the user specifies a limit and offset, this requests a list of (limit) run reports where
     *   the first entry is offset from the total number of reports.
     * - If the user does not specify a count or limit, this requests a list of all run reports.
     *
     * This function also checks whether the user provided invalid command line options. If the options
     * are invalid, this emits a message to stderr and returns false.
     * - countReports is mutually exclusive with all other report options.
     * - runId is mutually exclusive with all other report options.
     *
     * @param tempoClient Object that manages HTTP calls to PTC Tempo.
     * @return True if command line options are valid for reports command, false if any are invalid.
     */
    bool handleReports(TempoClient& tempoClient) const {
        if (!settings.runId.empty()) {
            if (settings.countReports || ( settings.limit != 0 ) || ( settings.offset != 0 ) ) {
                std::cerr << "Error. The --id option is not used with any other option." << std::endl;
                return false;
            }
            tempoClient.reports(settings.runId);
        } else if (settings.countReports) {
            if ( !settings.runId.empty() || ( settings.limit != 0 ) || ( settings.offset != 0 ) ) {
                std::cerr << "Error. The --count option is not used with any other option." << std::endl;
                return false;
            }
            tempoClient.reportsCount();
        } else {
            if ( !settings.runId.empty() || settings.countReports ) {
                std::cerr << "Error. The --count and --id options are not used with --limit or --offset options." << std::endl;
                return false;
            }
            tempoClient.reports(settings.limit, settings.offset);
        }
        return true;
    }

    /**
     * @brief Handles requests for run status or to start a run.
     *
     * If the user provides a protocol name on the command line, this will start a run. If the
     * user does not provide a protocol name, this will get current run status.
     *
     * This checks for these invalid conditions for command line options.
     * - When getting run status, this checks if the user set any other command line options.
     * - When starting a run, this checks if the user requested both a public protocol and a template.
     *
     * @param tempoClient Object that manages HTTP calls to PTC Tempo.
     * @return True if command line options are valid for run command, false if any are invalid.
     */
    bool handleRun(TempoClient& tempoClient) {
        if (settings.protocol.empty()) {
            if ( settings.publicProtocols || settings.templateProtocol
                || !settings.plateID.empty() || !settings.runName.empty()
                || (settings.volume > 0) || (settings.lidTemp > 0) ) {
                std::cerr << "Error. The --volume, --plate, --name, --public, --templates, --monitor, and --temp options require the --protocol option." << std::endl;
                return false;
            }
            // get the run status
            tempoClient.run();
            return true;
        }

        if ( settings.publicProtocols && settings.templateProtocol ) {
            std::cerr << "Error. The --public and --templates options are mutually exclusive." << std::endl;
            return false;
        }

        json run;
        run["protocolName"] = settings.protocol;
        run["location"] = settings.publicProtocols ? "public" : settings.templateProtocol ? "templates" : "user";
        // apply a default plateID and runName one isn't provide
        // consider moving this to the config for defaults
        if (settings.plateID.empty() || settings.runName.empty()) {
            auto timestamp = std::chrono::system_clock::now();
            auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()).count();
            std::string suffix = std::to_string(millis % 10000);
            if (settings.plateID.empty()) {
                settings.plateID = "plate" + suffix;
            }
            if (settings.runName.empty()) {
                settings.runName = "run" + settings.protocol + suffix;
            }
        }
        run["runName"] = settings.runName;
        run["plateID"] = settings.plateID;
        if (settings.volume > 0) {
            run["volume"] = settings.volume;
        }
        if (settings.lidTemp > 0) {
            run["lidTemp"] = settings.lidTemp;
        }
        tempoClient.run(run);
        return true;
    }

public:

    /**
     * @brief Constructor sets up all the command line options used to run the client app.
     */
    Router() {
        tempo.require_subcommand(0, 1);
        tempo.add_option("--host", settings.host, "Sets the host string of the instrument. example: http://10.10.2.51");
        tempo.add_option("--password", settings.password, "Provides password for the Automation user.");
        tempo.add_option("--waitTime", settings.waitTime, "Sets how long to wait for a response in seconds.");
        tempo.add_option("--interval", settings.interval, "Sets polling interval in seconds when monitoring.");
        tempo.add_option("--display", settings.displayType, "Sets output display format - options: text or json.");

        lidCommand = tempo.add_subcommand("lid", "Gets the instrument lid status.");
        lidCommand->add_flag("--monitor", settings.monitor, "Monitor lid status.");
        lidCommand->add_option("--interval", settings.interval, "Set interval for polling lid status. Requires --monitor flag.");

        openCommand = tempo.add_subcommand("open", "Opens the instrument lid.");
        closeCommand = tempo.add_subcommand("close", "Closes the instrument lid.");
        
        statusCommand = tempo.add_subcommand("status", "Gets a brief status of the instrument and currently running protocol.");
        statusCommand->add_flag("--monitor", settings.monitor, "Monitor instrument status.");
        statusCommand->add_option("--interval", settings.interval, "Set interval for instrument status refresh. Requires --monitor flag.");

        faultCommand = tempo.add_subcommand("errors", "Gets a list of device faults.");
        faultCommand->add_flag("--clear", settings.clearFaults, "Clear device faults.");

        reportsCommand = tempo.add_subcommand("reports", "Gets a list of run reports for the Automation user or retrieves the details of a specific run report.");
        reportsCommand->add_option("--id", settings.runId, "Run id of the report to retrieve. Not used with any other options.");
        reportsCommand->add_option("--limit", settings.limit, "Number of reports to retrieve. Not used with --id or --count options.");
        reportsCommand->add_option("--offset", settings.offset, "Offset at which to start retrieving the list of reports. Not used with --id or --count options.");
        reportsCommand->add_flag("--count", settings.countReports, "Returns the total count of reports. Not used with any other options.");

        protocolsCommand = tempo.add_subcommand("protocols", "Lists all protocols present in the Automation user's My Files folder.");
        protocolsCommand->add_flag("--public", settings.publicProtocols, "List the Public protocols instead of user protocols.");

        runCommand = tempo.add_subcommand("run", "If used without options, it provides run status. If used with --protocol option, it starts a run.");
        runCommand->add_option("--protocol", settings.protocol, "Name of the protocol to run.");
        runCommand->add_option("--name", settings.runName, "Name for the run. Requires the --protocol option.");
        runCommand->add_option("--plate", settings.plateID, "ID of the plate used in the run. Requires the --protocol option.");
        runCommand->add_option("--volume", settings.volume, "Volume for the run. Requires the --protocol option.");
        runCommand->add_option("--temp", settings.lidTemp, "Lid temperature for the run. Requires the --protocol option.");
        runCommand->add_flag("--public", settings.publicProtocols, "Protocol is in the Public location instead of user location. Requires the --protocol option.");
        runCommand->add_flag("--templates", settings.templateProtocol, "Use a template protocol. Requires the --protocol option.");
        runCommand->add_flag("--monitor", settings.monitor, "Monitor run status. Requires the --protocol option.");
        runCommand->add_option("--interval", settings.interval, "Set interval for run status refresh. Requires --monitor flag.");

        stopCommand = tempo.add_subcommand("stop", "Stops the protocol run.");
        skipCommand = tempo.add_subcommand("skip", "Skips the currently active step in the protocol run.");
        pauseCommand = tempo.add_subcommand("pause", "Pauses the protocol run.");
        resumeCommand = tempo.add_subcommand("resume", "Resumes the protocol run.");

        licenseCommand = tempo.add_subcommand("license", "Prints the copyright licenses.");
        versionCommand = tempo.add_subcommand("version", "Prints the versions and checks the Automation API compatibility.");
        configCommand = tempo.add_subcommand("config", "Sets the default values in config.json.");
        tempoConfig.options(*configCommand);
    }

    /**
     * @brief Initialize the settings using the config file and command line args.
     */
    void initialize() {
        tempoConfig.initialize(settings);
    }

    /// Returns reference to root command line application handler.
    CLI::App& tempoCli() {
        return tempo;
    }

    /**
     * @brief This function routes monitor commands to functions in tempoClient.
     * @param command Which command to process.
     * @param tempoClient Reference to client connection object.
     * @param success Output parameter indicates monitoring completely successfully.
     * @return True if command was processed.
     */
    bool routeMonitorCommands(const CLI::App& command, TempoClient& tempoClient, bool& success) {
        auto monitorInterval = static_cast<int32_t>(settings.interval);
        success = true;
        bool processed = false;
        if (command.get_name() == lidCommand->get_name()) {
            processed = true;
            if (settings.monitor) {
                auto monitor = Monitor(tempoClient, monitorInterval, settings.displayType, [&tempoClient]() {
                    tempoClient.lid();
                    return tempoClient.getLidStatus() == "opening" || tempoClient.getLidStatus() == "closing";
                });
                success = monitor.success();
                return processed;
            }
            tempoClient.lid();

        } else if (command.get_name() == statusCommand->get_name()) {
            processed = true;
            if (settings.monitor) {
                auto monitor = Monitor(tempoClient, monitorInterval, settings.displayType, [&tempoClient]() {
                    tempoClient.status();
                    return tempoClient.getRunStatus() == "running" || tempoClient.getRunStatus() == "paused";
                });
                success = monitor.success();
                return processed;
            }
            tempoClient.status();

        } else if (command.get_name() == runCommand->get_name()) {
            processed = true;
            if (!handleRun(tempoClient)) {
                success = false;
            } else
            if (settings.monitor && (tempoClient.getRunStatus() == "running" || tempoClient.getRunStatus() == "paused")) {
                auto monitor = Monitor(tempoClient, monitorInterval, settings.displayType, [&tempoClient]() {
                    tempoClient.run();
                    return tempoClient.getRunStatus() == "running" || tempoClient.getRunStatus() == "paused";
                });
                success = monitor.success();
                return processed;
            }

        }
        if (processed) {
            return tempoClient.print(settings.displayType);
        }
        return processed;
    }

    /**
     * @brief This function routes a subcommand and options from the command line to the correct
     * function in the tempoClient object.
     *
     * It obtains the subcommands from the tempo object. It performs some validity checks of the
     * command line args. If the subcommand does not require any HTTP connection, such as license
     * and config, it runs those and returns. If the subcommand requires HTTP connection, it makes
     * a tempoClient object and calls the proper method therein.
     *
     * @return True for success, false for failure. If failure occurs, this emits a message
     *  to stderr.
     */
    bool route() {

        auto commands = tempo.get_subcommands();

        // Process config and license command without creating a TempoClient
        if (commands.size() > 1) {
            std::cerr << "No more than one command" << std::endl;
            return false;
        } else if (!commands.empty()) {
            const CLI::App* command = *commands.begin();
            if (command->get_name() == licenseCommand->get_name()) {
                Config::license();
                return true;

            } else if (command->get_name() == configCommand->get_name()) {
                tempoConfig.save();
                tempoConfig.print(settings.displayType);
                return true;
            }
        }

        // process requests to the instrument
        TempoClient tempoClient(settings.host, settings.password, static_cast<int32_t>(settings.waitTime));

        if (commands.empty()) {
            tempoClient.tempo();
            return tempoClient.print(settings.displayType);
        }

        const CLI::App* command = *commands.begin();

        if (bool success; routeMonitorCommands(*command, tempoClient, success)) {
            return success;
        } else if (command->get_name() == reportsCommand->get_name()) {
            if (!handleReports(tempoClient)) {
                return false;
            }

        } else if (command->get_name() == openCommand->get_name()) {
            tempoClient.openLid();

        } else if (command->get_name() == closeCommand->get_name()) {
            tempoClient.closeLid();

       } else if (command->get_name() == protocolsCommand->get_name()) {
            tempoClient.protocols(settings.publicProtocols);

        } else if (command->get_name() == faultCommand->get_name()) {
            tempoClient.faults(settings.clearFaults);
            
        } else if (command->get_name() == stopCommand->get_name()) {
            tempoClient.stop();

        } else if (command->get_name() == skipCommand->get_name()) {
            tempoClient.skip();

        } else if (command->get_name() == pauseCommand->get_name()) {
            tempoClient.pause();

        } else if (command->get_name() == resumeCommand->get_name()) {
            tempoClient.resume();
        } else if (command->get_name() == versionCommand->get_name()) {
            return tempoClient.version(settings.displayType);
        }

        return tempoClient.print(settings.displayType);
    }

};

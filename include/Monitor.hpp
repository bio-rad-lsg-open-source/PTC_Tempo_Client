//
// PTC Tempo Automation API sample client
//
// Copyright © 2023 Bio-Rad Laboratories Inc. All rights Reserved
//
// MIT License
//
// SPDX-License-Identifier: MIT
//

#include "TempoClient.hpp"
#ifdef WIN32
#include <windows.h>
#endif

#include <thread>
#include <chrono>
#include <algorithm>

/**
 * @brief The Monitor class executes a StatusCall function repeatedly until that call returns
 * false or this class cannot update the screen.
 *
 * It is used for HTTP requests that check a status repeatedly (e.g. - lid, run, or status).
 */
class Monitor {

public:

    /**
     * @brief Constructor sets up the monitor and repeatedly calls the status function.
     * StatusCall, template definition of function used for monitoring.
     * @param tempoClient_ Reference to object that makes HTTP requests.
     * @param interval How many seconds to wait between calls to status function.
     * @param displayType_ How to format the output; either json or text.
     * @param statusCall Reference to function that obtains status from instrument. This can be a lambda.
     */
    template<typename StatusCall>
    Monitor(TempoClient& tempoClient_, int32_t interval, const std::string& displayType_, StatusCall statusCall) :
            tempoClient(tempoClient_),
            displayType(displayType_) {
        clearConsole();
        int16_t bottomLine;

        // request status from instrument
        bool monitor = statusCall();
        refreshScreen(bottomLine);
        if (!monitor) {
            clearBottom(bottomLine);
            return;
        }
        bool done = false;
        do {
            std::this_thread::sleep_for(std::chrono::seconds(interval));
            // request status from instrument
            if (!statusCall()) {
                done = true;
            } else if (!refreshScreen(bottomLine)) {
                successValue = false;
                done = true;
            }
        } while (!done);

        clearBottom(bottomLine);
        tempoClient.print(displayType);
    }

    /// Returns true for success, false if unable to upddate screen.
    [[nodiscard]] bool success() const {
        return successValue;
    }

private:

    /// Reference to object that makes HTTP requests.
    TempoClient& tempoClient;
    /// How to format the output; either json or text.
    const std::string& displayType;
    /// False if unable to upddate screen.
    bool successValue = true;

    /**
     * @brief Refreshes output contents on terminal.
     * @param bottomLine Output parameter for number of lines printed, not number of rows in screen.
     * @return True to keep polling status, false to stop.
     */
    bool refreshScreen(int16_t& bottomLine) const {
        int columns;
        int rows;
#ifdef WIN32
        CONSOLE_SCREEN_BUFFER_INFO consoleInfo;

        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &consoleInfo);
        columns = consoleInfo.srWindow.Right - consoleInfo.srWindow.Left;
        rows = consoleInfo.srWindow.Bottom - consoleInfo.srWindow.Top;
#else
        columns = 0;
        rows = 0;
#endif
        int16_t numberOfLines = 0;

        if (!tempoClient.statusOK()) {
            return false;
        }
        std::string responseResult;
        if (!tempoClient.responseString(responseResult, displayType)) {
            return false;
        }
#ifdef WIN32
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), COORD{0, 0});
#endif
        std::istringstream stream(responseResult);
        std::string line;

        while (std::getline(stream, line)) {
            ++numberOfLines;
            int32_t len = columns - static_cast<int32_t>(line.length());
            std::string clear = len > 0 ? std::string(len, ' ') : std::string();
            std::cout << line << clear << std::endl;
        }
        bottomLine = numberOfLines;
        std::string clear(columns, ' ');
        for (int32_t i = numberOfLines; i <= rows; ++i) {
            std::cout << clear;
            if (i < rows) {
                std::cout << std::endl;
            }
        }
        return true;
    }

    /**
     * @brief Clears all output lines on the console.
     */
    static void clearConsole() {
        [[maybe_unused]] int noLinuxWarning;
#ifdef _WIN32
        HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
        COORD coordScreen = {0, 0};
        DWORD charsWritten;
        CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
        DWORD conSize;
        if (!GetConsoleScreenBufferInfo(console, &consoleInfo)) {
            return;
        }
        conSize = consoleInfo.dwSize.X * consoleInfo.dwSize.Y;
        if (!FillConsoleOutputCharacter(console, ' ', conSize, coordScreen, &charsWritten)) {
            return;
        }
        if (!GetConsoleScreenBufferInfo(console, &consoleInfo)) {
            return;
        }
        if (!FillConsoleOutputAttribute(console, consoleInfo.wAttributes, conSize, coordScreen, &charsWritten)) {
            return;
        }
        SetConsoleCursorPosition(console, coordScreen);
#endif
    }

    /**
     * @brief Clears just the bottom line of output.
     * @param bottomLine Number of rows to clear.
     */
    static void clearBottom([[maybe_unused]] int16_t bottomLine) {
        [[maybe_unused]] int16_t rows;
#ifdef _WIN32
        HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

        CONSOLE_SCREEN_BUFFER_INFO consoleInfo;

        GetConsoleScreenBufferInfo(console, &consoleInfo);
        rows = static_cast<int16_t>(consoleInfo.srWindow.Bottom - consoleInfo.srWindow.Top);
        bottomLine = bottomLine < rows ? bottomLine : rows;
        ++bottomLine;
        COORD coordScreen = {0, bottomLine};
        SetConsoleCursorPosition(console, coordScreen);
#endif
    }

};

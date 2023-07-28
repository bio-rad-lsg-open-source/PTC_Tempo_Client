//
// PTC Tempo Automation API sample client
//
// Copyright © 2023 Bio-Rad Laboratories Inc. All rights Reserved
//
//  MIT License
//
// SPDX-License-Identifier: MIT
//

#include "Router.hpp"
#include <exception>
#include <stdexcept>

/**
 * @brief Main function creates the Router object and runs it.
 *
 * @par Exceptions
 * To prevent exceptions from unwinding the stack and crashing the client app, the main
 * function catches all exceptions. It emits a message to stderr and exits.
 *
 * @return 0 for success, 1 for failure, anything greater than 1 is an HTTP status code.
 */
int main(int argc, char **argv) {
    int success = false;
    try {
        Router router;
        router.initialize();

        CLI11_PARSE(router.tempoCli(), argc, argv)
        success = router.route();

    } catch (std::invalid_argument const& ex) {
        std::cerr << std::endl << std::endl << "Exception: " << ex.what() << std::endl << std::endl;
    } catch (...) {
        std::exception_ptr p = std::current_exception();
        std::cerr << std::endl << std::endl << "Exception: " << (p ? typeid(p).name() : "null") << std::endl << std::endl;
    }
    return success ? 0 : 1;
}

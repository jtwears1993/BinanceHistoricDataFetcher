//
// Created by jtwears on 10/26/25.
//

#pragma once

#include <iostream>
#include <string>

namespace common {
    /// Check condition and exit if not true.
    inline auto ASSERT(const bool cond, const std::string &msg) noexcept {
        [[unlikely]] if (!cond) {
            std::cerr << "ASSERT : " << msg << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}
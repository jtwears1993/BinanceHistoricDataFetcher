//
// Created by jtwears on 11/5/25.
//

#pragma once

#include <atomic>

namespace common::sync::producer_consumer {
    struct Context {
        std::atomic<bool> producerDone{false};
        std::atomic<bool> consumerDone{false};
        std::atomic<bool> running{true};
    };
}
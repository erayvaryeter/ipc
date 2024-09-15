#pragma once

#include <iostream>
#include <functional>

struct MessageTemplate { size_t numberOfConsumers = 0; };

// PRODUCER MESSAGES
struct MdfChangeRequest : MessageTemplate {
    uint8_t wantedMdfIndex = 0;
};

// CALLBACK MESSAGES
struct MdfChange : MessageTemplate {
    uint8_t newMdfIndex = 0;
};
using MdfChangeCallback = std::function<void(MdfChange)>;
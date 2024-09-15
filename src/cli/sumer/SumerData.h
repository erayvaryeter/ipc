#pragma once

#include <vector>
#include <memory>
#include <ipc/SharedMemory.h>

#include "SumerMessages.h"

class SumerData {
public:
    static SumerData& getInstance() {
        static SumerData instance;
        return instance;
    }

    bool createAllProducers();
    bool createAllConsumers();
    
    // producer methods
    void changeMdf(uint8_t newMdfIndex);

    // consumer methods
    void callbackMdfChangeRequest(MdfChangeRequest request);

    void startConsumers();

    SumerData(const SumerData&) = delete;
    SumerData& operator=(const SumerData&) = delete;

private:
    SumerData() = default;
    ~SumerData() = default;

    // producers
    std::shared_ptr<ProducerObject<MdfChange>> m_producerMdfChange;

    // consumers
    std::shared_ptr<ConsumerObject<MdfChangeRequest, MdfChangeRequestCallback>> m_consumerMdfChangeRequest;
};
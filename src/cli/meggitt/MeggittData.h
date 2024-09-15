#pragma once

#include <vector>
#include <memory>
#include <ipc/SharedMemory.h>

#include "MeggittMessages.h"

class MeggittData {
public:
    static MeggittData& getInstance() {
        static MeggittData instance;
        return instance;
    }

    bool createAllProducers();
    bool createAllConsumers();

    // producer methods
    void sendMdfChangeRequest(uint8_t wantedMdfIndex);

    // consumer methods
    void callbackMdfChange(MdfChange newMdfIndex);

    void startConsumers();

    MeggittData(const MeggittData&) = delete;
    MeggittData& operator=(const MeggittData&) = delete;

private:
    MeggittData() = default;
    ~MeggittData() = default;

    // producers
    std::shared_ptr<ProducerObject<MdfChangeRequest>> m_producerMdfChangeRequest;

    // consumers
    std::shared_ptr<ConsumerObject<MdfChange, MdfChangeCallback>> m_consumerMdfChange;
};
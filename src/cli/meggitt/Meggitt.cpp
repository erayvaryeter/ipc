#include <iostream>
#include <thread>
#include <chrono>

#include "MeggittData.h"

int main() {
    bool producersCreated = false;
    bool consumersCreated = false;

    while (!producersCreated) {
        producersCreated = MeggittData::getInstance().createAllProducers();
    }

    while (!consumersCreated) {
        consumersCreated = MeggittData::getInstance().createAllConsumers();
    }

    MeggittData::getInstance().startConsumers();

    std::this_thread::sleep_for(std::chrono::seconds(2));

    size_t wantedMdfIndex = 1;
    MeggittData::getInstance().sendMdfChangeRequest(wantedMdfIndex);

    system("pause");
    return 0;
}
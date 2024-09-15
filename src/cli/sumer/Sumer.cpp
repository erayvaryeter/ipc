#include <iostream>
#include <thread>
#include <chrono>

#include "SumerData.h"

int main() {
    bool producersCreated = false;
    bool consumersCreated = false;

    while (!producersCreated) {
        producersCreated = SumerData::getInstance().createAllProducers();
    }

    while (!consumersCreated) {
        consumersCreated = SumerData::getInstance().createAllConsumers();
    }

    SumerData::getInstance().startConsumers();

    system("pause");
    return 0;
}
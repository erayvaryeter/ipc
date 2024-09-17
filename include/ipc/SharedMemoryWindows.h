#pragma once

#include <cstddef>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <windows.h>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <memory>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

using namespace boost::interprocess;

class SharedMemoryObject {
public:
    inline SharedMemoryObject(const std::string& sharedMemoryName);
    inline virtual ~SharedMemoryObject();
    inline void printTime();

protected:
    size_t m_sharedMemorySize;
    std::string m_sharedMemoryName;
    std::string m_semaphoreNewDataName;
    HANDLE m_semaphoreNewData;
};

template <typename DataType>
class ProducerObject : public SharedMemoryObject {
public:
    ProducerObject(const std::string& sharedMemoryName, const std::string& sharedDataName);
    ~ProducerObject() override;
    bool create();
    bool write(DataType& data);

private:
    std::string m_sharedDataName;
    DataType* m_sharedData;
    std::shared_ptr<managed_shared_memory> m_sharedMemory;
};

template <typename DataType, typename CallbackType>
class ConsumerObject : public SharedMemoryObject {
public:
    ConsumerObject(const std::string& sharedMemoryName, const std::string& sharedDataName);
    ~ConsumerObject() override;
    bool open();
    bool read();
    void registerCallback(CallbackType& callback);

private:
    CallbackType m_callback;
    std::string m_sharedDataName;
    DataType* m_sharedData;
    std::shared_ptr<managed_shared_memory> m_sharedMemory;
};

////////////////////// IMPLEMENTATION //////////////////////////

SharedMemoryObject::SharedMemoryObject(const std::string& sharedMemoryName)
    : m_sharedMemoryName(""),
    m_semaphoreNewDataName(""),
    m_sharedMemorySize(0),
    m_semaphoreNewData(nullptr)
{
    m_sharedMemoryName = sharedMemoryName + "_DATA";
    m_semaphoreNewDataName = sharedMemoryName + "_SEM_NEW_DATA";
}

SharedMemoryObject::~SharedMemoryObject() 
{
    if (m_semaphoreNewData != nullptr) {
        CloseHandle(m_semaphoreNewData);
    }
}

void SharedMemoryObject::printTime() 
{
#ifdef LOG_SHARED_MEMORY
    // Get the current time point from the system clock
    auto now = std::chrono::system_clock::now();

    // Convert to time_t to get the time in seconds
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time_t);

    // Get the microseconds part
    auto duration = now.time_since_epoch();
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    auto seconds = microseconds / 1000000;
    auto micros = microseconds % 1000000;

    // Output the time with microseconds precision
    std::cout << "Current time: ";
    std::cout << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");
    std::cout << '.' << std::setw(6) << std::setfill('0') << micros << std::endl;
#endif
}

template <typename DataType>
ProducerObject<DataType>::ProducerObject(const std::string& sharedMemoryName, const std::string& sharedDataName)
    : SharedMemoryObject(sharedMemoryName),
    m_sharedDataName(""),
    m_sharedData(nullptr),
    m_sharedMemory(nullptr)
{
    m_sharedDataName = sharedDataName;
    m_sharedMemorySize = sizeof(DataType);
}

template <typename DataType>
ProducerObject<DataType>::~ProducerObject() 
{
    if (m_sharedMemory != nullptr) {
        m_sharedMemory->destroy<DataType>(m_sharedMemoryName.c_str());
    }
}

template <typename DataType, typename CallbackType>
ConsumerObject<DataType, CallbackType>::ConsumerObject(const std::string& sharedMemoryName, const std::string& sharedDataName)
    : SharedMemoryObject(sharedMemoryName),
    m_sharedDataName(""),
    m_sharedData(nullptr),
    m_sharedMemory(nullptr)
{
    m_sharedDataName = sharedDataName;
    m_sharedMemorySize = sizeof(DataType);
}

template <typename DataType, typename CallbackType>
ConsumerObject<DataType, CallbackType>::~ConsumerObject() 
{
    if (m_sharedMemory != nullptr) {
        m_sharedMemory->destroy<DataType>(m_sharedMemoryName.c_str());
    }
}

template <typename DataType>
bool ProducerObject<DataType>::create() 
{
    try {
        m_sharedMemory = std::make_shared<managed_shared_memory>(open_or_create, m_sharedMemoryName.c_str(), static_cast<int>(m_sharedMemorySize));
        if (m_sharedMemory != nullptr) {
            return false;
        }

        m_sharedData = m_sharedMemory->find_or_construct<DataType>(m_sharedDataName.c_str())();
        if (m_sharedData != nullptr) {
            return false;
        }

        m_semaphoreNewData = CreateSemaphore(NULL, 0, 100, m_semaphoreNewDataName.c_str());
        if (m_semaphoreNewData == nullptr) {
            return false;
        }
    }
    catch (const interprocess_exception& ex) {
        std::cout << "producer error: " << ex.what() << std::endl;
        return false;
    }

    return true;
}

template <typename DataType>
bool ProducerObject<DataType>::write(DataType& data) 
{
    if (sizeof(data) > m_sharedMemorySize) {
        return false;
    }
    printTime();
    size_t previousNumberOfConsumers = data.numberOfConsumers;
    *m_sharedData = data;
    (*m_sharedData).numberOfConsumers = previousNumberOfConsumers;
    // notify consumers - increase the count by number of consumers
    ReleaseSemaphore(m_semaphoreNewData, static_cast<int>(previousNumberOfConsumers), NULL);
    return true;
}

template <typename DataType, typename CallbackType>
bool ConsumerObject<DataType, CallbackType>::open() 
{
    try {
        m_sharedMemory = std::make_shared<managed_shared_memory>(open_only, m_sharedMemoryName.c_str());
        if (m_sharedMemory != nullptr) {
            return false;
        }

        auto result = m_sharedMemory->find<DataType>(m_sharedDataName.c_str());
        if (result.first != nullptr) {
            m_sharedData = result.first;
        }
        else {
            return false;
        }

        m_semaphoreNewData = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, m_semaphoreNewDataName.c_str());
        if (m_semaphoreNewData == nullptr) {
            return false;
        }
    }
    catch (const interprocess_exception& ex) {
        std::cout << "consumer error: " << ex.what() << std::endl;
        return false;
    }

    return true;
}

template <typename DataType, typename CallbackType>
bool ConsumerObject<DataType, CallbackType>::read() 
{
    WaitForSingleObject(m_semaphoreNewData, INFINITE); // if count is already 0 -> wait. if count is more than 0 -> proceed and decrease
    DataType data = *m_sharedData;
    printTime();
    m_callback(data);
    return true;
}

template <typename DataType, typename CallbackType>
void ConsumerObject<DataType, CallbackType>::registerCallback(CallbackType& callback) 
{
    m_callback = callback;
    (*m_sharedData).numberOfConsumers++;
    std::cout << "Number of consumers: " << static_cast<int>((*m_sharedData).numberOfConsumers) << std::endl;
}
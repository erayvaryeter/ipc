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

class SharedMemoryObject {
public:
    inline SharedMemoryObject(const std::string& name);
    inline virtual ~SharedMemoryObject();
    inline void printTime();

protected:
    std::string m_memoryName;
    std::string m_semaphoreReadWriteName;
    std::string m_semaphoreNewDataName;
    size_t m_memorySize;
    HANDLE m_fileMappingHandle;
    void* m_fileMappingAddress;
    HANDLE m_semaphoreReadWrite;
    HANDLE m_semaphoreNewData;
};

template <typename DataType>
class ProducerObject : public SharedMemoryObject {
public:
    ProducerObject(const std::string& name) : SharedMemoryObject(name) { m_memorySize = sizeof(DataType); }
    ~ProducerObject() override {}
    bool create();
    bool write(DataType& data);
};

template <typename DataType, typename CallbackType>
class ConsumerObject : public SharedMemoryObject {
public:
    ConsumerObject(const std::string& name) : SharedMemoryObject(name) { m_memorySize = sizeof(DataType); }
    ~ConsumerObject() override {}
    bool open();
    bool read();
    void registerCallback(CallbackType& callback);
private:
    CallbackType m_callback;
};

////////////////////// IMPLEMENTATION //////////////////////////

SharedMemoryObject::SharedMemoryObject(const std::string& name)
    : m_memoryName(""),
    m_fileMappingHandle(nullptr),
    m_fileMappingAddress(nullptr),
    m_semaphoreReadWrite(nullptr),
    m_semaphoreNewData(nullptr)
{
    m_memoryName = name + "_DATA";
    m_semaphoreReadWriteName = name + "_SEM_READ_WRITE";
    m_semaphoreNewDataName = name + "_SEM_NEW_DATA";
}

SharedMemoryObject::~SharedMemoryObject()
{
    if (m_fileMappingAddress != nullptr) {
        UnmapViewOfFile(m_fileMappingAddress);
    }
    if (m_fileMappingHandle != nullptr) {
        CloseHandle(m_fileMappingHandle);
    }
    if (m_semaphoreReadWrite != nullptr) {
        CloseHandle(m_semaphoreReadWrite);
    }
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
bool ProducerObject<DataType>::create() {
    // create actual data shared memory elements
    m_fileMappingHandle = CreateFileMapping(INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        static_cast<DWORD>(m_memorySize),
        m_memoryName.c_str());

    if (m_fileMappingHandle == nullptr) {
        return false;
    }

    m_fileMappingAddress = MapViewOfFile(m_fileMappingHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (m_fileMappingAddress == nullptr) {
        return false;
    }

    m_semaphoreReadWrite = CreateSemaphore(NULL, 1, 1, m_semaphoreReadWriteName.c_str());
    if (m_semaphoreReadWrite == nullptr) {
        return false;
    }

    m_semaphoreNewData = CreateSemaphore(NULL, 0, 100, m_semaphoreNewDataName.c_str());
    if (m_semaphoreNewData == nullptr) {
        return false;
    }

    return true;
}

template <typename DataType>
bool ProducerObject<DataType>::write(DataType& data) {
    if (sizeof(data) > m_memorySize) {
        return false;
    }
    printTime();
    WaitForSingleObject(m_semaphoreReadWrite, INFINITE);
    DataType tempData; 
    std::memcpy(&tempData, m_fileMappingAddress, sizeof(tempData));
    data.numberOfConsumers = tempData.numberOfConsumers;
    std::memcpy(m_fileMappingAddress, &data, sizeof(data));
    ReleaseSemaphore(m_semaphoreReadWrite, 1, NULL);
    // notify consumers - increase the count by number of consumers
    ReleaseSemaphore(m_semaphoreNewData, static_cast<int>(data.numberOfConsumers), NULL);
    return true;
}

template <typename DataType, typename CallbackType>
bool ConsumerObject<DataType, CallbackType>::open() {
    // open actual data shared memory elements
    m_fileMappingHandle = OpenFileMapping(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        m_memoryName.c_str());

    if (m_fileMappingHandle == nullptr) {
        return false;
    }

    m_fileMappingAddress = MapViewOfFile(m_fileMappingHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (m_fileMappingAddress == nullptr) {
        return false;
    }

    m_semaphoreReadWrite = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, m_semaphoreReadWriteName.c_str());
    if (m_semaphoreReadWrite == nullptr) {
        return false;
    }

    m_semaphoreNewData = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, m_semaphoreNewDataName.c_str());
    if (m_semaphoreNewData == nullptr) {
        return false;
    }

    return true;
}

template <typename DataType, typename CallbackType>
bool ConsumerObject<DataType, CallbackType>::read() {
    WaitForSingleObject(m_semaphoreNewData, INFINITE); // if count is already 0 -> wait. if count is more than 0 -> proceed and decrease
    DataType data;
    WaitForSingleObject(m_semaphoreReadWrite, INFINITE);
    std::memcpy(&data, m_fileMappingAddress, sizeof(data));
    ReleaseSemaphore(m_semaphoreReadWrite, 1, NULL);
    printTime();
    m_callback(data);
    return true;
}

template <typename DataType, typename CallbackType>
void ConsumerObject<DataType, CallbackType>::registerCallback(CallbackType& callback) {
    m_callback = callback;
    DataType currentData;
    WaitForSingleObject(m_semaphoreReadWrite, INFINITE);
    std::memcpy(&currentData, m_fileMappingAddress, sizeof(currentData));
    currentData.numberOfConsumers++;
    std::cout << "Number of consumers: " << static_cast<int>(currentData.numberOfConsumers) << std::endl;
    std::memcpy(m_fileMappingAddress, &currentData, sizeof(currentData));
    ReleaseSemaphore(m_semaphoreReadWrite, 1, NULL);
}
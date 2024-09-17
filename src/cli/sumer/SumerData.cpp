#include "SumerData.h"
#include <thread>

bool SumerData::createAllProducers() {
	bool retval = true;

	m_producerMdfChange = std::make_shared<ProducerObject<MdfChange>>("FEWS_MEMORY", "MDF_CHANGE");
	if (m_producerMdfChange != nullptr) { /*if (!m_producerMdfChange->create()) { retval = false; }*/ }
	else { retval = false; }

	if (retval == false) {
		std::cout << "Producers could not be created, will try again in 1 second ..." << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	return retval;
}

bool SumerData::createAllConsumers() {
	bool retval = true;

	m_consumerMdfChangeRequest = std::make_shared<ConsumerObject<MdfChangeRequest, MdfChangeRequestCallback>>("FEWS_MEMORY", "MDF_CHANGE_REQUEST");
	if (m_consumerMdfChangeRequest != nullptr) { 
		if (m_consumerMdfChangeRequest->open()) {
			MdfChangeRequestCallback func = [](MdfChangeRequest request) { SumerData::getInstance().callbackMdfChangeRequest(request); };
			m_consumerMdfChangeRequest->registerCallback(func);
		} else { retval = false; }
	}
	else { retval = false; }
	
	if (retval == false) {
		std::cout << "Consumers could not be opened, will try again in 1 second ..." << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	return retval;
}

void SumerData::changeMdf(uint8_t newMdfIndex) {
	MdfChange data;
	data.newMdfIndex = newMdfIndex;
	bool retval = m_producerMdfChange->write(data);
	if (retval) {
		std::cout << "SUMER: Mdf Changed - Index: " << static_cast<int>(data.newMdfIndex) << std::endl;
	}
}

void SumerData::callbackMdfChangeRequest(MdfChangeRequest request) {
	std::cout << "SUMER: Mdf Change Requested - Index: " << static_cast<int>(request.wantedMdfIndex) << std::endl;
	std::this_thread::sleep_for(std::chrono::seconds(1));
	changeMdf(request.wantedMdfIndex);
}

void SumerData::startConsumers() {
	std::thread threadCallbackMdfChangeRequest([this]() { while (true) { m_consumerMdfChangeRequest->read(); } });
	threadCallbackMdfChangeRequest.detach();
}
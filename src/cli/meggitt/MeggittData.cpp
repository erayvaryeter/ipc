#include "MeggittData.h"
#include <thread>

bool MeggittData::createAllProducers() {
	bool retval = true;
	
	m_producerMdfChangeRequest = std::make_shared<ProducerObject<MdfChangeRequest>>("FEWS_MEMORY", "MDF_CHANGE_REQUEST");
	if (m_producerMdfChangeRequest != nullptr) { /*if (!m_producerMdfChangeRequest->create()) { retval = false; }*/ }
	else { retval = false; }

	if (retval == false) {
		std::cout << "Producers could not be created, will try again in 1 second ..." << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	return retval;
}

bool MeggittData::createAllConsumers() {
	bool retval = true;

	m_consumerMdfChange = std::make_shared<ConsumerObject<MdfChange, MdfChangeCallback>>("FEWS_MEMORY", "MDF_CHANGE");
	if (m_consumerMdfChange != nullptr) {
		if (m_consumerMdfChange->open()) {
			MdfChangeCallback func = [](MdfChange mdfChange) { MeggittData::getInstance().callbackMdfChange(mdfChange); };
			m_consumerMdfChange->registerCallback(func);
		}
		else { retval = false; }
	}
	else { retval = false; }

	if (retval == false) {
		std::cout << "Consumers could not be opened, will try again in 1 second ..." << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	return retval;
}

void MeggittData::sendMdfChangeRequest(uint8_t wantedMdfIndex) {
	MdfChangeRequest request;
	request.wantedMdfIndex = wantedMdfIndex;
	bool retval = m_producerMdfChangeRequest->write(request);
	if (retval) {
		std::cout << "MEGGITT: Mdf Change Requested - Index: " << static_cast<int>(request.wantedMdfIndex) << std::endl;
	}
}

void MeggittData::callbackMdfChange(MdfChange newMdfIndex) {
	std::cout << "MEGGITT: Mdf Changed - Index: " << static_cast<int>(newMdfIndex.newMdfIndex) << std::endl;
	std::this_thread::sleep_for(std::chrono::seconds(1));
	sendMdfChangeRequest(newMdfIndex.newMdfIndex + 1);
}

void MeggittData::startConsumers() {
	std::thread threadCallbackMdfChange([this]() { while (true) { m_consumerMdfChange->read(); } });
	threadCallbackMdfChange.detach();
}
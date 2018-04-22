/*
 * logger.cpp
 *
 *  Created on: Apr 16, 2018
 *      Author: martin
 */

#include "logger.h"

void Logger::write(const std::string& aMsg, log_level_t aLevel)
{
	log_item_t item = { std::chrono::system_clock::now(), aMsg, aLevel};

	//write to all sinks
	for(auto iSink: m_sinks) {
		iSink(item);
	}

	//write to hisotry buffer
	m_entries.push_back(item);
}

void Logger::attach(logger_sink_f aSink)
{
	for(const auto& iEntry: m_entries) {
		aSink(iEntry);
	}

	m_sinks.push_back(aSink);
}

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

std::string log_level_t_str(const log_level_t& aLevel)
{
	switch(aLevel) {
	case log_level_t::INFO: 	return "INFO";
	case log_level_t::WARNING: 	return "WARNING";
	case log_level_t::ERROR:	return "ERROR";
	}

	return "UNKOWN_LOG_LEVEL";
}

uint32_t log_level_t_color(const log_level_t& aLevel)
{
	switch(aLevel) {
	case log_level_t::INFO: 	return 0xFFEDEDED;
	case log_level_t::WARNING: 	return 0xFFffea4f;
	case log_level_t::ERROR:	return 0xFFf9614d;
	};

	return 0xFFFFFFFF;
}

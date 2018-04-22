/*
 * logger.h
 *
 *  Created on: Apr 16, 2018
 *      Author: martin
 */

#ifndef SRC_LOGGER_H_
#define SRC_LOGGER_H_

#include <memory>
#include <vector>
#include <chrono>
#include <string>
#include <functional>
#include <tuple>

//TODO: rename to _e postfix
enum class log_level_t {
	INFO,
	WARNING,
	ERROR,
};

//TODO: remove static; implement in .cpp
static std::string log_level_t_str(const log_level_t& aLevel)
{
	switch(aLevel) {
	case log_level_t::INFO: 	return "INFO";
	case log_level_t::WARNING: 	return "WARNING";
	case log_level_t::ERROR:	return "ERROR";
	}

	return "UNKOWN_LOG_LEVEL";
}

//TODO: remove static; implement in .cpp
static uint32_t log_level_t_color(const log_level_t& aLevel)
{
	switch(aLevel) {
	case log_level_t::INFO: 	return 0xFFEDEDED;
	case log_level_t::WARNING: 	return 0xFFffea4f;
	case log_level_t::ERROR:	return 0xFFf9614d;
	};

	return 0xFFFFFFFF;
}

struct log_item_t {
	std::chrono::time_point<std::chrono::system_clock> time;
	std::string msg;
	log_level_t level;
};

using logger_sink_f = std::function<void(const log_item_t&)>;

class Logger
{
private:
	Logger()  = default;
	Logger(const Logger&) = delete;
	Logger& operator=(const Logger&) = delete;

	std::vector<log_item_t> m_entries;
	std::vector<logger_sink_f> m_sinks;


public:
	static Logger* Instance()
	{
		static Logger* impl_logger = new Logger();

		return impl_logger;
	}

	void write(const std::string& aMsg, log_level_t aLevel);

	void attach(logger_sink_f aSink);
};

template<log_level_t ELevel>
void LoggerWrite(const std::string& aMsg)
{
	Logger::Instance()->write(aMsg, ELevel);
}

static auto LoggerInfo = [](const std::string& aMsg) -> void { LoggerWrite<log_level_t::INFO>(aMsg); };
static auto LoggerWarning = [](const std::string& aMsg) -> void { LoggerWrite<log_level_t::WARNING>(aMsg); };
static auto LoggerError = [](const std::string& aMsg) -> void { LoggerWrite<log_level_t::ERROR>(aMsg); };



#endif /* SRC_LOGGER_H_ */

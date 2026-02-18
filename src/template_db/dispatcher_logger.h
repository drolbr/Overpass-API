#ifndef DE__OSM3S___TEMPLATE_DB__DISPATCHER_LOGGER
#define DE__OSM3S___TEMPLATE_DB__DISPATCHER_LOGGER

#include <sys/types.h>
#include <cstdint>
#include <string>


class Logger
{
public:
  Logger(time_t now, const std::string& db_dir, const std::string& filename = "");
  void annotated_log(const std::string& message);
  void raw_log(const std::string& message);
  void set_now(time_t now_) { now = now_; }

private:
  std::string logfile_full_name;
  time_t now;
  pid_t pid;
};


struct Dispatcher_Reading_Logger
{
  Dispatcher_Reading_Logger(Logger& logger_) : logger(&logger_) {}

  void request_read_and_idx(pid_t pid, uint32_t max_allowed_time, uint64_t max_allowed_space) const;
  void read_idx_finished(pid_t pid) const;
  void read_finished(pid_t pid) const;
  void read_aborted(pid_t pid) const;
  void arguments_mismatch(pid_t pid, uint num_expected, uint num_provided) const;

private:
  Logger* logger;
};


#endif

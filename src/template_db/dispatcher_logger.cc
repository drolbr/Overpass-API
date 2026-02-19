#include "dispatcher_logger.h"

#include <unistd.h>
#include <fstream>
#include <sstream>


void Dispatcher_Reading_Logger::request_enqueued(
    pid_t pid, uint32_t max_allowed_time, uint64_t max_allowed_space) const
{
  std::ostringstream out;
  out<<"request_enqueued "<<pid<<" timeout "<<max_allowed_time
  <<" space "<<max_allowed_space<<'.';
  logger->annotated_log(out.str());
}


void Dispatcher_Reading_Logger::read_idx_started(pid_t pid) const
{
  std::ostringstream out;
  out<<"read_idx_started "<<pid<<'.';
  logger->annotated_log(out.str());
}


void Dispatcher_Reading_Logger::read_idx_finished(pid_t pid) const
{
  std::ostringstream out;
  out<<"read_idx_finished "<<pid<<'.';
  logger->annotated_log(out.str());
}


void Dispatcher_Reading_Logger::read_finished(pid_t pid) const
{
  std::ostringstream out;
  out<<"read_finished of process "<<pid<<'.';
  logger->annotated_log(out.str());
}


void Dispatcher_Reading_Logger::read_aborted(pid_t pid) const
{
  std::ostringstream out;
  out<<"read_aborted of process "<<pid<<'.';
  logger->annotated_log(out.str());
}


void Dispatcher_Reading_Logger::arguments_mismatch(pid_t pid, uint num_expected, uint num_provided) const
{
  std::ostringstream out;
  out<<"too few arguments from "<<pid<<": expected "<<num_expected<<" but got "<<num_provided;
  logger->annotated_log(out.str());
}


//-----------------------------------------------------------------------------


Logger::Logger(time_t now_, const std::string& db_dir, const std::string& filename)
    : logfile_full_name(db_dir + filename), now(now_), pid(getpid()) {}

void Logger::annotated_log(const std::string& message)
{
  // Collect current time in a user-readable form.
  struct tm* tm_ = gmtime(&now);
  char strftime_buf[21];
  strftime_buf[0] = 0;
  if (tm_)
    strftime(strftime_buf, 21, "%F %H:%M:%S ", tm_);

  std::ofstream out(logfile_full_name.c_str(), std::ios_base::app);
  out<<strftime_buf<<'['<<pid<<"] "<<message<<'\n';
}


void Logger::raw_log(const std::string& message)
{
  std::ofstream out(logfile_full_name.c_str(), std::ios_base::app);
  out<<message<<'\n';
}

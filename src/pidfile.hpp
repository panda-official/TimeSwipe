// -*- C++ -*-

// PANDA Timeswipe Project
// Copyright (C) 2021  PANDA GmbH / Dmitry Igrishin

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef PANDA_TIMESWIPE_PIDFILE_HPP
#define PANDA_TIMESWIPE_PIDFILE_HPP

#include "error.hpp"

#include <cerrno>
#include <string>

#include <sys/file.h> // flock()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace panda::timeswipe::detail {

/// RAII-guarded PID file handle.
class Pid_file final {
public:
  /// The destructor.
  ~Pid_file()
  {
    cleanup();
  }

  /**
   * @brief The constructor.
   *
   * @details Doesn't creates PID file implicitly. The creation is deferred
   * until call of lock().
   *
   * @param stem Filename with stripped extension.
   *
   * @see lock().
   */
  explicit Pid_file(const std::string& stem)
    : name_{std::string{"/var/run/"}.append(stem).append(".pid")}
  {}

  /// Attempts to create PID file and obtain lock on it.
  void lock()
  {
    if (is_locked_) return;

    // Exception protector.
    struct Guard {
      ~Guard() { if (!done) self.cleanup(); }
      Guard(Pid_file& self) : self{self} {}
      bool done{};
      Pid_file& self;
    } guard{*this};

    // Open file.
    if ( (fd_ = ::open(name_.c_str(), O_CREAT | O_RDWR, 0600)) < 0) {
      const auto code = errno;
      throw Sys_exception{code,
        std::string{"cannot open file "}.append(name())};
    }

    // Lock file.
    if (::flock(fd_, LOCK_EX | LOCK_NB)) {
      const auto code = errno;
      throw Sys_exception{code,
        std::string{"cannot obtain exclusive lock on file "}.append(name())};
    }
    is_locked_ = true;

    // Check if process with PID from file exists.
    const auto proc_name = std::string{"/proc/"}.append(read_pid()).append("/exe");
    if (!::access(proc_name.c_str(), F_OK))
      /*
       * Process with PID from file exists, but the corresponding PID file is
       * successfully locked by the current process. Thus, it's okay to truncate
       * it out.
       */
      truncate_pid();

    write_pid();

    guard.done = true;
  }

  /// Attempts to create PID file and obtain lock on it.
  void unlock()
  {
    if (!is_locked_) return;

    close_pid_file();
    unlink_pid_file();
    is_locked_ = false;
  }

  /// @returns `true` if locked.
  bool is_locked() const noexcept
  {
    return is_locked_;
  }

  /// @returns The name of lock file.
  const std::string& name() const noexcept
  {
    return name_;
  }

private:
  std::string name_;
  bool is_locked_{};
  int fd_{-1};

  std::string read_pid()
  {
    std::string result{15, '\0'};
    const auto sz = ::read(fd_, result.data(), result.size());
    if (sz < 0) {
      const auto code = errno;
      throw Sys_exception{code,
        std::string{"cannot read PID from file "}.append(name_)};
    }
    result.resize(sz);
    return result;
  }

  void write_pid()
  {
    PANDA_TIMESWIPE_ASSERT(fd_ != -1);
    const auto pid = std::to_string(::getpid());
    if (::write(fd_, pid.data(), pid.size()) < 0) {
      const auto code = errno;
      throw Sys_exception{code,
        std::string{"cannot write PID to file "}.append(name_)};
    }
  }

  void truncate_pid()
  {
    PANDA_TIMESWIPE_ASSERT(fd_ != -1);
    if (::ftruncate(fd_, 0)) {
      const auto code = errno;
      throw Sys_exception{code,
        std::string{"cannot truncate file "}.append(name_)};
    }
  }

  void close_pid_file()
  {
    if (fd_ < 0) return;

    if (::close(fd_)) {
      const auto code = errno;
      throw Sys_exception{code,
        std::string{"cannot close file "}.append(name_)};
    }
    fd_ = -1;
  }

  void unlink_pid_file()
  {
    PANDA_TIMESWIPE_ASSERT(fd_ == -1);
    if (::unlink(name_.c_str())) {
      const auto code = errno;
      throw Sys_exception{code,
        std::string{"cannot unlink file "}.append(name_)};
    }
  }

  void cleanup() noexcept
  {
    if (fd_ >= 0) {
      ::close(fd_);
      fd_ = -1;
    }

    if (is_locked_) {
      ::unlink(name_.c_str());
      is_locked_ = false;
    }
  }
};

} // namespace panda::timeswipe::detail

#endif  // PANDA_TIMESWIPE_PIDFILE_HPP

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

#include <cstring>
#include <cerrno>
#include <string>

#include <sys/file.h> // flock()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace panda::timeswipe::detail {

class Pid_file final {
public:
  /// The destructor.
  ~Pid_file()
  {
    unlock();
  }

  /**
   * Create PID file.
   *
   * @param name Unique application name.
   */
  Pid_file(const std::string& name)
    : fname_{std::string{"/var/run/"}.append(name).append(".pid")}
  {}

  /**
   * Lock.
   *
   * If first call was successful then subsequence calls will be successful too.
   *
   * @param[out] error Error description when lock is failed.
   *
   * @returns `false` if pidloc failed, and `error` has detailed error description.
   */
  bool lock(std::string& error)
  {
    if (locked_) return true;
    fd_ = open(fname_.c_str(), O_CREAT | O_RDWR, 0600);
    if (fd_ < 0) {
      const auto code = errno;
      error = std::string("lock open failed: ") + std::string(strerror(code));
      return false;
    }
    int rc = flock(fd_, LOCK_EX | LOCK_NB);
    if (rc) {
      const auto code = errno;
      if (EWOULDBLOCK == code) {
        error = "another instance running";
      } else {
        error = std::string("flock failed: ") + std::string(strerror(code));
      }
      unlock();
      return false;
    }
    std::string buf(32, '\0');
    int rd = read(fd_, buf.data(), buf.size());
    buf.resize(rd);

    std::string proc_name{"/proc/"+buf+"/exe"};
    if (access(proc_name.c_str(), F_OK) != -1) {
      error = "process exists with pid " + buf;
      unlock();
      return false;
    }

    buf = std::to_string(getpid());
    rd = write(fd_, buf.data(), buf.size());
    locked_ = true;

    return true;
  }

  /// @returns `true` if locked.
  bool is_locked() const noexcept
  {
    return locked_;
  }

  /// @returns The name of lock file.
  const std::string& name() const noexcept
  {
    return fname_;
  }

private:
  std::string fname_;
  int fd_{-1};
  bool locked_{};

  void unlock()
  {
    if (fd_ != -1) {
      close(fd_);
      fd_ = -1;
    }
    if (locked_) {
      unlink(fname_.c_str());
      locked_ = false;
    }
  }
};

} // namespace panda::timeswipe::detail

#endif  // PANDA_TIMESWIPE_PIDFILE_HPP

// -*- C++ -*-
// Copyright (C) 2021 Dmitry Igrishin
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
// Dmitry Igrishin
// dmitigr@gmail.com

#ifndef DMITIGR_PROGPAR_PROGPAR_HPP
#define DMITIGR_PROGPAR_PROGPAR_HPP

#include "exceptions.hpp"
#include "version.hpp"
#include "../error/assert.hpp"
#include "../filesystem.hpp"

#include <algorithm>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace dmitigr::progpar {

/**
 * @brief Program parameters.
 *
 * Stores the parsed program parameters like the following:
 *   prog [--opt1 --opt2=val] [--] [arg1 arg2]
 *
 * Each option may have a value specified after the "=" character. The sequence
 * of two dashes ("--") indicates "end of options", so the remaining parameters
 * should be treated as arguments rather than as options.
 *
 * @remarks Short options notation (e.g. `-o` or `-o 1`) doesn't supported
 * currently and always treated as arguments.
 */
class Program_parameters final {
public:
  /// The alias to represent a map of program options.
  using Option_map = std::map<std::string, std::optional<std::string>>;

  /// The alias to represent a vector of program arguments.
  using Argument_vector = std::vector<std::string>;

  /**
   * An option reference.
   *
   * @warning The lifetime of the instances of this class is limited by
   * the lifetime of the corresponding instances of type Program_parameters.
   */
  class Optref final {
  public:
    /// @returns `true` if the instance is valid (references an option).
    bool is_valid() const noexcept
    {
      return is_valid_;
    }

    /**
     * @returns `is_valid()`.
     *
     * @par Requires
     * `!value()`.
     */
    bool is_valid_throw_if_value() const
    {
      const auto valid = is_valid();
      if (valid && value())
        throw_requirement("requires no value");

      return valid;
    }

    /**
     * @returns `is_valid()`.
     *
     * @par Requires
     * `value()`.
     */
    bool is_valid_throw_if_no_value() const
    {
      const auto valid = is_valid();
      if (valid && !value())
        throw_requirement("requires a value");

      return valid;
    }

    /// @returns `is_valid()`.
    explicit operator bool() const noexcept
    {
      return is_valid();
    }

    /// @returns The corresponding Program_parameters instance.
    const Program_parameters& program_parameters() const noexcept
    {
      return program_parameters_;
    }

    /// @returns The name of this option.
    const std::string& name() const
    {
      return name_;
    }

    /**
     * @returns The value of this option.
     *
     * @par Requires
     * `is_valid()`.
     */
    const std::optional<std::string>& value() const
    {
      if (!is_valid())
        throw_requirement("must present");

      return value_;
    }

    /**
     * @returns `*value()`.
     *
     * @par Requires
     * `value()`.
     */
    const std::string& not_null_value() const
    {
      const auto& val = value();
      if (!val)
        throw_requirement("requires a value");

      return *val;
    }

    /**
     * @returns `*value()`.
     *
     * @par Requires
     * `!value().empty()`.
     */
    const std::string& not_empty_value() const
    {
      const auto& val = not_null_value();
      if (val.empty())
        throw_requirement("requires a non empty value");

      return val;
    }

    /// @returns `value().value_or(std::move(val))`.
    std::string value_or(std::string val) const
    {
      return value().value_or(std::move(val));
    }

  private:
    friend Program_parameters;

    bool is_valid_{};
    const Program_parameters& program_parameters_;
    std::string name_;
    std::optional<std::string> value_;

    /// The constructor. (Constructs invalid instance.)
    Optref(const Program_parameters& pp, std::string name) noexcept
      : program_parameters_{pp}
      , name_{std::move(name)}
    {
      DMITIGR_ASSERT(!is_valid());
    }

    /// The constructor.
    explicit Optref(const Program_parameters& pp,
      std::string name, std::optional<std::string> value) noexcept
      : is_valid_{true}
      , program_parameters_{pp}
      , name_{std::move(name)}
      , value_{std::move(value)}
    {
      DMITIGR_ASSERT(is_valid());
    }

    /// @throws `Generic_exception`.
    [[noreturn]] void throw_requirement(const std::string_view requirement) const
    {
      throw Generic_exception{std::string{"option --"}
        .append(name_).append(" ").append(requirement)};
    }
  };

  /// The default constructor. (Constructs invalid instance.)
  Program_parameters() noexcept = default;

  /**
   * @brief The constructor.
   *
   * @par Requires
   * `(argc > 0 && argv && argv[0] && std::strlen(argv[0]) > 0)`.
   */
  Program_parameters(const int argc, const char* const* argv)
  {
    if (!(argc > 0))
      throw Generic_exception{"invalid count of program parameters (argc)"};
    else if (!argv)
      throw Generic_exception{"invalid vector of program parameters (argv)"};

    path_ = argv[0];
    if (path_.empty())
      throw Generic_exception{"invalid program name (argv[0])"};

    static const auto opt = [](const std::string_view arg)
      -> std::optional<std::pair<std::string, std::optional<std::string>>>
      {
        if (auto pos = arg.find("--"); pos == 0) {
          if (arg.size() == 2) {
            // Explicit end-of-opts.
            return std::make_pair(std::string{}, std::nullopt);
          } else if (pos = arg.find('=', 2); pos != std::string::npos) {
            // Option with value.
            auto name = arg.substr(2, pos - 2);
            auto value = arg.substr(pos + 1);
            return std::pair<std::string, std::string>(std::move(name),
              std::move(value));
          } else
            // Option without value.
            return std::make_pair(std::string{arg.substr(2)}, std::nullopt);
        }

        // Not an option.
        return std::nullopt;
      };

    if (argc == 1)
      return;

    int argi = 1;

    // Collecting options.
    for (; argi < argc; ++argi) {
      if (auto o = opt(argv[argi])) {
        if (o->first.empty()) {
          // Explicit end-of-opts detected.
          ++argi;
          break;
        } else
          options_[std::move(o->first)] = std::move(o->second);
      } else
        // First argument (implicit end-of-opts) detected.
        break;
    }

    // Collecting arguments.
    for (; argi < argc; ++argi)
      arguments_.emplace_back(argv[argi]);

    DMITIGR_ASSERT(is_valid());
  }

  /**
   * @brief The constructor.
   *
   * @par Requires
   * `!path.empty()`.
   */
  explicit Program_parameters(std::filesystem::path path,
    Option_map options = {}, Argument_vector arguments = {})
    : path_{std::move(path)}
    , options_{std::move(options)}
    , arguments_{std::move(arguments)}
  {
    if (path_.empty())
      throw Generic_exception{"invalid program name (argv[0])"};
    DMITIGR_ASSERT(is_valid());
  }

  /// @returns `false` if this instance is default-constructed.
  bool is_valid() const noexcept
  {
    return !path_.empty();
  }

  /// @returns The executable path.
  const std::filesystem::path& path() const noexcept
  {
    return path_;
  }

  /// @returns The map of options.
  const Option_map& options() const noexcept
  {
    return options_;
  }

  /// @returns The vector of arguments.
  const Argument_vector& arguments() const noexcept
  {
    return arguments_;
  }

  /// @returns The option reference, or invalid instance if no option `name`.
  Optref option(const std::string& name) const noexcept
  {
    const auto i = options_.find(name);
    return i != cend(options_) ? Optref{*this, i->first, i->second} :
      Optref{*this, name};
  }

  /// @returns A value of type `std::tuple<Optref, ...>`.
  template<class ... Types>
  auto options(Types&& ... names) const noexcept
  {
    return std::make_tuple(option(std::forward<Types>(names))...);
  }

  /// @returns `option(option_name)`.
  Optref operator[](const std::string& option_name) const noexcept
  {
    return option(option_name);
  }

  /**
   * @returns `arguments()[argument_index]`.
   *
   * @par Requires
   * `(argument_index < arguments().size())`.
   */
  const std::string& operator[](const std::size_t argument_index) const
  {
    if (!(argument_index < arguments_.size()))
      throw Generic_exception{"invalid program argument index"};
    return arguments_[argument_index];
  }

private:
  std::filesystem::path path_;
  Option_map options_;
  Argument_vector arguments_;
};

} // namespace dmitigr::progpar

#endif // DMITIGR_PROGPAR_PROGPAR_HPP

// -*- C++ -*-

// PANDA Timeswipe Project
// Copyright (C) 2021  PANDA GmbH

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

#ifndef PANDA_TIMESWIPE_FIRMWARE_JSON_HPP
#define PANDA_TIMESWIPE_FIRMWARE_JSON_HPP

#include <cstddef>
#ifndef RAPIDJSON_HAS_STDSTRING
#define RAPIDJSON_HAS_STDSTRING 1
#endif
#ifndef RAPIDJSON_NO_SIZETYPEDEFINE
#define RAPIDJSON_NO_SIZETYPEDEFINE 1
#endif
#ifndef RAPIDJSON_ALLOCATOR_DEFAULT_CHUNK_CAPACITY
#define RAPIDJSON_ALLOCATOR_DEFAULT_CHUNK_CAPACITY 32
#endif

namespace rapidjson {
using SizeType = std::size_t;
} // namespace rapidjson

#include "../3rdparty/dmitigr/3rdparty/rapidjson/document.h"
#include "../3rdparty/dmitigr/3rdparty/rapidjson/stringbuffer.h"
#include "../3rdparty/dmitigr/3rdparty/rapidjson/writer.h"

#include <string>

inline void set_error(rapidjson::Document& resp,
  rapidjson::Value& root,
  std::string edescr,
  const rapidjson::Value& val = {})
{
  using Value = rapidjson::Value;
  auto& alloc = resp.GetAllocator();
  root.SetObject();
  root.AddMember("error", Value{rapidjson::kObjectType}, alloc);
  auto& error = root["error"];
  error.AddMember("edescr", std::move(edescr), alloc);
  if (!val.IsNull())
    error.AddMember("val", std::move(Value{}.CopyFrom(val, alloc, true)), alloc);
}

inline std::string to_text(const rapidjson::Value& value)
{
  rapidjson::StringBuffer buf;
  rapidjson::Writer writer{buf};
  writer.SetMaxDecimalPlaces(3);
  return value.Accept(writer) ? std::string{buf.GetString(), buf.GetSize()} :
    std::string{};
}

#endif  // PANDA_TIMESWIPE_FIRMWARE_JSON_HPP

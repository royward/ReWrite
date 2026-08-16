// ============================================================================
// File: database_elements.hpp
// Description: DataElement definition - all data is worked with in this format
// ============================================================================
// Copyright 2026 Roy Ward
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <cstdint>
#include <vector>
#include <deque>
#include <string>
#include <variant>

inline constexpr uint32_t TYPE_UNBOUND = 0;
inline constexpr uint32_t TYPE_BOOL = 1;
inline constexpr uint32_t TYPE_I64 = 2;
inline constexpr uint32_t TYPE_LIST = 3;
inline constexpr uint32_t TYPE_CHAR = 4;

struct DataElement;
class DataVector;

class DataVector {
public:
    int32_t refcount = 0;
    std::vector<DataElement> list;
    static uint32_t freelist;
    static std::vector<DataVector> data_vectors;

    static uint32_t allocate();
    static void incref(uint32_t index, uint32_t val = 1) {
        if (index == 0) return;
        data_vectors[index].refcount+=val;
    }
    static void decref(uint32_t index);
    static uint32_t count_free();
};

class DataContainer {
public:
    uint32_t offset=0;
    uint32_t pool_index=0;

    DataContainer() = default;
    DataContainer(size_t index, uint32_t off) : offset(off), pool_index(index) {
        DataVector::incref(pool_index,1);
    }
    DataContainer(const DataContainer& other)
        : offset(other.offset), pool_index(other.pool_index) {
        DataVector::incref(pool_index,1);
    }
    DataContainer(DataContainer&& other) noexcept
        : offset(other.offset), pool_index(other.pool_index) {
        other.pool_index = 0; // Steal ownership, safely reset source to root/null
        other.offset = 0;
    }
    ~DataContainer() {
        DataVector::decref(pool_index); // Automatically drops ref when container dies
    }
    // Handles: container1 = container2;
    DataContainer& operator=(const DataContainer& other) {
        if (this != &other) {
            DataVector::incref(other.pool_index,1);
            DataVector::decref(this->pool_index);
            this->pool_index = other.pool_index;
            this->offset = other.offset;
        }
        return *this;
    }
    // Handles: container1 = std::move(container2);
    DataContainer& operator=(DataContainer&& other) noexcept {
        if (this != &other) {
            DataVector::decref(this->pool_index);
            this->pool_index = other.pool_index;
            this->offset = other.offset;
            other.pool_index = 0;
            other.offset = 0;
        }
        return *this;
    }
    uint32_t get_refcount() {
        return DataVector::data_vectors[pool_index].refcount;
    }
};

struct DataUnbound {};
struct DataBool { bool value; };
struct DataInt { int64_t value; };
struct DataList { DataContainer value; };
struct DataChar { char value; };

using DataVariant = std::variant <DataUnbound, DataBool, DataInt, DataList, DataChar>;

struct DataElement {
    DataVariant value;
    uint32_t last_match=0;

    std::string to_string() const;
    DataElement() = default;
    DataElement(DataVariant val) : value(std::move(val)), last_match(0) {}
    DataElement(const DataElement& other) : value(other.value), last_match(other.last_match) {}
    DataElement(DataElement&& other) noexcept : value(std::move(other.value)), last_match(other.last_match) {}
    ~DataElement() = default;
    DataElement& operator=(const DataElement& other) {
        if (this != &other) {
            this->value = other.value; // Handles full cleanup of old type and copying of new type
            this->last_match = other.last_match;
        }
        return *this;
    }
    DataElement& operator=(DataElement&& other) noexcept {
        if (this != &other) {
            this->value = std::move(other.value); // Steals variant resources cleanly
            this->last_match = other.last_match;
        }
        return *this;
    }
    void incref(uint32_t val = 1) const {
        if (std::holds_alternative<DataList>(value)) {
            const auto& list_wrapper = std::get<DataList>(value);
            DataVector::incref(list_wrapper.value.pool_index, val);
        }
    }
    void decref() const {
        if (std::holds_alternative<DataList>(value)) {
            const auto& list_wrapper = std::get<DataList>(value);
            DataVector::decref(list_wrapper.value.pool_index);
        }
    }
};


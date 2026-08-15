// ============================================================================
// File: database_elements.cpp
// Description: DataElement methods - currently only to_string
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

#include "data_element.hpp"
#include <stdexcept>
#include <span>

std::vector<DataVector> DataVector::data_vectors=std::vector<DataVector>(1);
uint32_t DataVector::freelist=0;

uint32_t DataVector::allocate() {
    if (freelist == 0) {
        // No recycled slots available; expand the vector pool
        data_vectors.push_back(DataVector{});
        return data_vectors.size() - 1;
    }

    // Pop from freelist
    uint32_t allocated_index = freelist;
    freelist = data_vectors[allocated_index].refcount; // Next free slot was hidden here

    // Reset node data for its new life
    data_vectors[allocated_index].refcount = 0;
    data_vectors[allocated_index].list.clear();
    return allocated_index;
}

void DataVector::decref(uint32_t index) {
    if (index == 0) return;
    data_vectors[index].refcount--;
    if (data_vectors[index].refcount <= 0) {
        data_vectors[index].list.clear();
        // Minimize memory capacity if an interpreter list grew massively
        data_vectors[index].list.shrink_to_fit();
        data_vectors[index].refcount = freelist;
        freelist = index;
    }
}

uint32_t DataVector::count_free() {
    uint32_t count=0;
    uint32_t free=freelist;
    while(free!=0) {
        count++;
        free=data_vectors[free].refcount;
    }
    return count;
}


// void DataContainer::ensure_unique() {
//     uint32_t old_index = pool_index;
//     if (old_index == 0 || DataVector::data_vectors[old_index].refcount <= 1) {
//         return;
//     }
//     uint32_t new_index = DataVector::allocate();
//
//     const auto& old_list = DataVector::data_vectors[old_index].list;
//     auto& new_list = DataVector::data_vectors[new_index].list;
//     new_list.assign(old_list.begin() + offset, old_list.end());
//     for (const auto& element : DataVector::data_vectors[new_index].list) {
//         element.incref(1); // Uses your new DataElement::incref method
//     }
//     pool_index = new_index;
//     DataVector::decref(old_index);
// }


std::string display_single_char(char ch, char term) {
    if(ch==term) {
        return {'\\',term};
    }
    switch(ch) {
        case '\t':return {'\\','t'};
        case '\r':return {'\\','r'};
        case '\n':return {'\\','n'};
        default: return {ch};
    }
}

namespace detail {
    inline std::string to_string_visit(const DataUnbound&) {
        return "<unbound>";
        //throw std::runtime_error("Unbond variables cannot be displayed");
    }
    inline std::string to_string_visit(const DataBool& b) {
        return b.value ? "true" : "false";
    }
    inline std::string to_string_visit(const DataInt& i) {
        return std::to_string(i.value);
    }
    inline std::string to_string_visit(const DataChar& c) {
        std::string s="\'";
        s+=display_single_char(c.value,'\'');
        s+="\'";
        return s;
    }
    inline std::string to_string_visit(const DataList& l) {
        const std::vector<DataElement>& fulllist=DataVector::data_vectors[l.value.pool_index].list;
        uint32_t offset=l.value.offset;
        const std::span<const DataElement> list=std::span<const DataElement>(fulllist.data()+offset,fulllist.size()-offset);
        // first check if the whole list is a string
        if(list.size()==0) {
            return "{}";
        }
        bool is_string=true;
        for(const DataElement& e:list) {
            if(!std::holds_alternative<DataChar>(e.value)) {
                is_string=false;
                break;
            }
        }
        if(is_string) {
            // whole list is a string, display as such
            std::string out = "\"";
            for (const DataElement& e:list) {
                out += display_single_char(std::get<DataChar>(e.value).value,'\"');
            }
            out += "\"";
            return out;
        } else {
            std::string out = "{";
            for (std::size_t i = 0; i < list.size(); i++) {
                if (i != 0) out += ",";
                out += list[i].to_string();
            }
            out += "}";
            return out;
        }
    }
}

std::string DataElement::to_string() const {
    return std::visit([](const auto& alt) { return detail::to_string_visit(alt); }, value);
}

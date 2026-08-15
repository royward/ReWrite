// ============================================================================
// File: program_call_internal.cpp
// Description: The internal "library functions"
// In a separate file because it is separate from the main execution logic and might get long
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
#include "program.hpp"
#include <fstream>
#include <iterator>
#include <print>

// https://en.cppreference.com/cpp/utility/functional for all the cool types stuff

template<typename InType, typename OutType, typename Op>
DataElement binary_op(const char* op_name, const DataElement& a, const DataElement& b, Op op) {
    if (!std::holds_alternative<InType>(a.value) || !std::holds_alternative<InType>(b.value)) {
        throw std::runtime_error(std::format("wrong types binary {}: {},{}", op_name, a.value.index(), b.value.index()));
    }
    return DataElement{OutType{op(std::get<InType>(a.value).value, std::get<InType>(b.value).value)}};
}

template<typename Op>
DataElement compare_op(const char* op_name, const DataElement& a, const DataElement& b, Op op) {
    if (std::holds_alternative<DataInt>(a.value) && std::holds_alternative<DataInt>(b.value)) {
        return DataElement{DataBool{op(std::get<DataInt>(a.value).value, std::get<DataInt>(b.value).value)}};
    } else if (std::holds_alternative<DataChar>(a.value) && std::holds_alternative<DataChar>(b.value)) {
        return DataElement{DataBool{op(std::get<DataChar>(a.value).value, std::get<DataChar>(b.value).value)}};
    } else {
        throw std::runtime_error(std::format("wrong types binary {}: {},{}", op_name, a.value.index(), b.value.index()));
    }
}

void check_arg_count(std::string_view function, const VecDataElement& args, std::size_t count) {
    if(args.data.size()-args.offset!=count) {
        throw std::runtime_error(std::format("wrong arg count for {}: Found {}, expected {}",function,args.data.size()-args.offset,count));
    }
}

template<typename Type> void check_type(std::string_view function, const DataElement&a) {
    if(!std::holds_alternative<Type>(a.value)) {
        throw std::runtime_error(std::format("wrong type for {}: {}",function,a.value.index()));
    }
}

void do_call_library(TokenKind op, const VecDataElement& args, VecDataElement& sofar) {
    switch(op) {
        case CountTrailingZeros: {
            check_arg_count("count_trailing_zeros",args,1);
            check_type<DataInt>("count_trailing_zeros",args.data[args.offset]);
            sofar.data.push_back(DataElement{DataInt{std::countr_zero(static_cast<uint64_t>(std::get<DataInt>(args.data[args.offset].value).value))}});
        } break;
        case CountLeadingZeros: {
            check_arg_count("count_leading_zeros",args,1);
            check_type<DataInt>("count_leading_zeros",args.data[args.offset]);
            sofar.data.push_back(DataElement{DataInt{std::countl_zero(static_cast<uint64_t>(std::get<DataInt>(args.data[args.offset].value).value))}});
        } break;
        case PopCount: {
            check_arg_count("pop_count",args,1);
            check_type<DataInt>("pop_count",args.data[args.offset]);
            sofar.data.push_back(DataElement{DataInt{std::popcount(static_cast<uint64_t>(std::get<DataInt>(args.data[args.offset].value).value))}});
         } break;
        case CharToInt: {
            check_arg_count("char_to_int",args,1);
            check_type<DataChar>("char_to_int",args.data[args.offset]);
            sofar.data.push_back(DataElement{DataInt{static_cast<int64_t>(std::get<DataChar>(args.data[args.offset].value).value)}});
        } break;
        case IntToChar: {
            check_arg_count("int_to_char",args,1);
            check_type<DataInt>("int_to_char", args.data[args.offset]);
            sofar.data.push_back(DataElement{DataChar{static_cast<char>(std::get<DataInt>(args.data[args.offset].value).value)}});
        } break;
        case Print: {
            check_arg_count("print",args,1);
            check_type<DataList>("print",args.data[args.offset]);
            const DataContainer& d0 = std::get<DataList>(args.data[args.offset].value).value;
            const std::vector<DataElement>& x0=DataVector::data_vectors[d0.pool_index].list;
            for(uint32_t i=d0.offset;i<x0.size();i++) {
                const DataElement& e=x0[i];
                check_type<DataChar>("print",e);
                putchar(std::get<DataChar>(e.value).value);
            }
        } break;
        case PrintLn: {
            check_arg_count("println",args,1);
            check_type<DataList>("println",args.data[args.offset]);
            const DataContainer& d0 = std::get<DataList>(args.data[args.offset].value).value;
            const std::vector<DataElement>& x0=DataVector::data_vectors[d0.pool_index].list;
            for(uint32_t i=d0.offset;i<x0.size();i++) {
                const DataElement& e=x0[i];
                check_type<DataChar>("println",e);
                putchar(std::get<DataChar>(e.value).value);
            }
            putchar('\n');
        } break;
        case PrintLnDebug: {
            std::print("Debug:");
            // prints all args and returns them unchanged - can be inserted anywhere for debugging
            for(std::size_t i=args.offset;i<args.data.size();i++) {
                if(i!=0)putchar(',');
                std::print("{}",args.data[i].to_string());
                sofar.data.push_back(args.data[i]);
            }
            putchar('\n');
        } break;
       case PrintLnAny: {
            for(std::size_t i=args.offset;i<args.data.size();i++) {
                if(i!=0)putchar(',');
                std::print("{}",args.data[i].to_string());
            }
            putchar('\n');
        } break;
        case LoadTextFile: {
            check_arg_count("load_text_file",args,1);
            // get filename from char list
            check_type<DataList>("load_text_file",args.data[args.offset]);
            const DataContainer& filename_list = std::get<DataList>(args.data[args.offset].value).value;
            const std::vector<DataElement>& filenamev=DataVector::data_vectors[filename_list.pool_index].list;
            std::string filename;
            for(uint32_t i=filename_list.offset;i<filenamev.size();i++) {
                const DataElement& e=filenamev[i];
                check_type<DataChar>("load_text_file",e);
                filename += std::get<DataChar>(e.value).value;
            }
            std::ifstream file(filename, std::ios::in | std::ios::binary);
            if(!file.is_open()) {
                throw std::runtime_error(std::format("load_text_file: could not open file: {}", filename));
            }
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            std::vector<DataElement> chars;
            chars.reserve(content.size());
            for(char c : content) {
                chars.push_back(DataElement{DataChar{c}});
            }
            uint32_t newvec=DataVector::allocate();
            DataVector::data_vectors[newvec].list=std::move(chars);
            DataContainer dc(newvec,0);
            sofar.data.push_back(DataElement{DataList{std::move(dc)}});
        } break;
        case LoadTextFileLines: {
            check_arg_count("load_text_file",args,1);
            // get filename from char list
            check_type<DataList>("load_text_file",args.data[args.offset]);
            const DataContainer& filename_list = std::get<DataList>(args.data[args.offset].value).value;
            const std::vector<DataElement>& filenamev=DataVector::data_vectors[filename_list.pool_index].list;
            std::string filename;
            for(uint32_t i=filename_list.offset;i<filenamev.size();i++) {
                const DataElement& e=filenamev[i];
                check_type<DataChar>("load_text_file",e);
                filename += std::get<DataChar>(e.value).value;
            }
            std::ifstream file(filename, std::ios::in | std::ios::binary);
            if(!file.is_open()) {
                throw std::runtime_error(std::format("load_text_file: could not open file: {}", filename));
            }
            std::vector<DataElement> lines;
            std::string line;
            while (std::getline(file, line)) {
                std::vector<DataElement> chars;
                chars.reserve(line.size());
                for(char c : line) {
                    chars.push_back(DataElement{DataChar{c}});
                }
                uint32_t newvec=DataVector::allocate();
                DataVector::data_vectors[newvec].list=std::move(chars);
                DataContainer dc(newvec,0);
                lines.push_back(DataElement{DataList{std::move(dc)}});
            }
            uint32_t newvec=DataVector::allocate();
            DataVector::data_vectors[newvec].list=std::move(lines);
            DataContainer dc(newvec,0);
            sofar.data.push_back(DataElement{DataList{std::move(dc)}});
        } break;
        case SaveTextFile: {
            check_arg_count("save_text_file",args,2);
             // get filename
            check_type<DataList>("save_text_file",args.data[args.offset]);
            check_type<DataList>("save_text_file",args.data[args.offset+1]);
            const DataContainer& filename_list = std::get<DataList>(args.data[args.offset].value).value;
            const std::vector<DataElement>& filenamev=DataVector::data_vectors[filename_list.pool_index].list;
            std::string filename;
            for(uint32_t i=filename_list.offset;i<filenamev.size();i++) {
                const DataElement& e=filenamev[i];
                check_type<DataChar>("save_text_file",e);
                filename += std::get<DataChar>(e.value).value;
            }
            // get content
            const DataContainer& content_list = std::get<DataList>(args.data[args.offset+1].value).value;
            const std::vector<DataElement>& content_listv=DataVector::data_vectors[content_list.pool_index].list;
            std::ofstream file(filename, std::ios::out | std::ios::binary);
            if(!file.is_open()) {
                throw std::runtime_error(std::format("save_text_file: could not open file: {}", filename));
            }
            for(uint32_t i=content_list.offset;i<content_listv.size();i++) {
                const DataElement& e=content_listv[i];
                check_type<DataChar>("save_text_file",e);
                file << std::get<DataChar>(e.value).value;
            }
        } break;
        case SaveBinaryFile: {
            check_arg_count("save_binary_file", args, 2);
            check_type<DataList>("save_text_file",args.data[args.offset]);
            check_type<DataList>("save_text_file",args.data[args.offset+1]);
            const DataContainer& filename_list = std::get<DataList>(args.data[args.offset].value).value;
            const std::vector<DataElement>& filenamev=DataVector::data_vectors[filename_list.pool_index].list;
            std::string filename;
            for(uint32_t i=filename_list.offset;i<filenamev.size();i++) {
                const DataElement& e=filenamev[i];
                check_type<DataChar>("save_text_file",e);
                filename += std::get<DataChar>(e.value).value;
            }
            const DataContainer& content = std::get<DataList>(args.data[args.offset+1].value).value;
            const std::vector<DataElement>& contentv=DataVector::data_vectors[content.pool_index].list;
            std::ofstream file(filename, std::ios::out | std::ios::binary);
            if(!file.is_open()) {
                throw std::runtime_error(std::format("save_binary_file: could not open file: {}", filename));
            }
            for(uint32_t i=content.offset;i<contentv.size();i++) {
                const DataElement& e=contentv[i];
                check_type<DataInt>("save_binary_file", e);
                uint8_t byte = static_cast<uint8_t>(std::get<DataInt>(e.value).value);
                file.write(reinterpret_cast<const char*>(&byte), 1);
            }
        } break;
        default: {
            throw std::runtime_error(std::format("unknown library function: {}",static_cast<int>(op)));
        }
    }
}

DataElement do_call_internal(TokenKind op, const VecDataElement& args) {
    switch(args.data.size()-args.offset) {
        case 1: {
            const DataElement& arg=args.data[args.offset];
            std::size_t argtype=arg.value.index();
            switch(op) {
                case Minus: {
                    if(argtype==TYPE_I64) {
                        return DataElement{DataInt{-std::get<DataInt>(arg.value).value}};
                    } else {
                        throw std::runtime_error(std::format("wrong type unary minus: {}",argtype));
                    }
                }
                 case Not: {
                    if(argtype==TYPE_BOOL) {
                        return DataElement{DataBool{!std::get<DataBool>(arg.value).value}};
                    } else {
                        throw std::runtime_error(std::format("wrong type not: {}",argtype));
                    }
                }
                 case Tilda: {
                    if(argtype==TYPE_I64) {
                        return DataElement{DataInt{~std::get<DataInt>(arg.value).value}};
                    } else {
                        throw std::runtime_error(std::format("wrong type not: {}",argtype));
                    }
                }
                default: throw std::runtime_error(std::format("unknown unary op: {}",(int)op));
            }
        } break;
        case 2: {
            const DataElement& arg0=args.data[args.offset];
            const DataElement& arg1=args.data[args.offset+1];
            std::size_t argtype0=arg0.value.index();
            std::size_t argtype1=arg1.value.index();
            switch(op) {
                case EqualEqual: { // works for all types
                    return DataElement{DataBool{compare_equal(arg0,arg1)}};
                }
                case NotEqual: { // works for all types
                    return DataElement{DataBool{!compare_equal(arg0,arg1)}};
                }
                case Plus: {
                   if(argtype0==TYPE_I64 && argtype1==TYPE_I64) {
                        return DataElement{DataInt{std::get<DataInt>(arg0.value).value+std::get<DataInt>(arg1.value).value}};
                    } else if(argtype0==TYPE_CHAR && argtype1==TYPE_I64) {
                        return DataElement{DataChar{static_cast<char>(std::get<DataChar>(arg0.value).value+std::get<DataInt>(arg1.value).value)}};
                   } else {
                        throw std::runtime_error(std::format("wrong types plus: {},{}",argtype0,argtype1));
                    }
                }
               case Minus: {
                    if(argtype0==TYPE_I64&& argtype1==TYPE_I64) {
                        return DataElement{DataInt{std::get<DataInt>(arg0.value).value-std::get<DataInt>(arg1.value).value}};
                    } else if(argtype0==TYPE_CHAR&& argtype1==TYPE_I64) {
                        return DataElement{DataChar{static_cast<char>(std::get<DataChar>(arg0.value).value-std::get<DataInt>(arg1.value).value)}};
                   } else if(argtype0==TYPE_CHAR&& argtype1==TYPE_CHAR) {
                        return DataElement{DataInt{static_cast<int>(std::get<DataChar>(arg0.value).value-std::get<DataChar>(arg1.value).value)}};
                    } else {
                        throw std::runtime_error(std::format("wrong types minus: {},{}",argtype0,argtype1));
                    }
                }
                case Star: return binary_op<DataInt, DataInt>("times", arg0, arg1, std::multiplies<>{});
                case Greater: return compare_op<>("greater", arg0, arg1, std::greater<>{});
                case Less: return compare_op<>("less", arg0, arg1, std::less<>{});
                case GreaterEqual: return compare_op<>("greater_equal", arg0, arg1, std::greater_equal<>{});
                case LessEqual: return compare_op<>("less_equal", arg0, arg1, std::less_equal<>{});
                case AndAnd: return binary_op<DataBool, DataBool>("logical_and", arg0, arg1, std::logical_and<>{});
                case OrOr: return binary_op<DataBool, DataBool>("logical_or", arg0, arg1, std::logical_or<>{});
                case And: return binary_op<DataInt, DataInt>("bit_and", arg0, arg1, std::bit_and<>{});
                case Or: return binary_op<DataInt, DataInt>("bit_or", arg0, arg1, std::bit_or<>{});
                case Xor: return binary_op<DataInt, DataInt>("bit_xor", arg0, arg1, std::bit_xor<>{});
                 case ShiftLeft: {
                    if(argtype0==TYPE_I64 && argtype1==TYPE_I64) {
                        if (std::get<DataInt>(arg1.value).value < 0) {
                            throw std::runtime_error("left shift by negative");
                        }
                        return DataElement{DataInt{std::get<DataInt>(arg0.value).value<<std::get<DataInt>(arg1.value).value}};
                    } else {
                        throw std::runtime_error(std::format("wrong types left shift: {},{}",argtype0,argtype1));
                    }
                }
                 case ShiftRight: {
                    if(argtype0==TYPE_I64 && argtype1==TYPE_I64) {
                        if (std::get<DataInt>(arg1.value).value < 0) {
                            throw std::runtime_error("right shift by negative");
                        }
                        return DataElement{DataInt{std::get<DataInt>(arg0.value).value>>std::get<DataInt>(arg1.value).value}};
                    } else {
                        throw std::runtime_error(std::format("wrong types right shift: {},{}",argtype0,argtype1));
                    }
                }
                 case Divide: {
                    if(argtype0==TYPE_I64 && argtype1==TYPE_I64) {
                        if (std::get<DataInt>(arg1.value).value == 0) {
                            throw std::runtime_error("division by zero");
                        }
                        return DataElement{DataInt{std::get<DataInt>(arg0.value).value/std::get<DataInt>(arg1.value).value}};
                    } else {
                        throw std::runtime_error(std::format("wrong types binary divide: {},{}",argtype0,argtype1));
                    }
                }
                case Modulus: {
                    if(argtype0==TYPE_I64 && argtype1==TYPE_I64) {
                        if (std::get<DataInt>(arg1.value).value == 0) {
                            throw std::runtime_error("modulus by zero");
                        }
                        return DataElement{DataInt{std::get<DataInt>(arg0.value).value%std::get<DataInt>(arg1.value).value}};
                    } else {
                        throw std::runtime_error(std::format("wrong types binary modulus: {},{}",argtype0,argtype1));
                    }
                }
                default: throw std::runtime_error(std::format("unknown binary op: {}",(int)op));
            }
        } break;
        default: throw std::runtime_error(std::format("do_call_internal only works with 1 or 2 args, found {}",args.data.size()-args.offset));
    }
    return DataElement{DataUnbound{}}; // keep compiler happy
}

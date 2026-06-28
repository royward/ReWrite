#include "program.hpp"

DataElement do_call_internal(uint32_t op, const std::vector<DataElement>& args) {
    switch(args.size()) {
        case 1: {
            const DataElement& arg=args[0];
            std::size_t argtype=arg.value.index();
            switch(op) {
                case OP_UNARY_MINUS: {
                    if(argtype==TYPE_I64) {
                        return DataElement{DataInt{-std::get<DataInt>(arg.value).value}};
                    } else {
                        throw std::runtime_error(std::format("wrong type unary minus: {}",argtype));
                    }
                }
                default: throw std::runtime_error(std::format("unknown unary op: {}",op));
            }
        } break;
        case 2: {
            const DataElement& arg0=args[0];
            const DataElement& arg1=args[1];
            std::size_t argtype0=arg0.value.index();
            std::size_t argtype1=arg1.value.index();
            switch(op) {
                case OP_PLUS: {
                    if(argtype0==TYPE_I64 && argtype1==TYPE_I64) {
                        return DataElement{DataInt{std::get<DataInt>(arg0.value).value+std::get<DataInt>(arg1.value).value}};
                    } else {
                        throw std::runtime_error(std::format("wrong types binary plus: {},{}",argtype0,argtype1));
                    }
                }
                case OP_MINUS: {
                    if(argtype0==TYPE_I64 && argtype1==TYPE_I64) {
                        return DataElement{DataInt{std::get<DataInt>(arg0.value).value-std::get<DataInt>(arg1.value).value}};
                    } else {
                        throw std::runtime_error(std::format("wrong types binary minus: {},{}",argtype0,argtype1));
                    }
                }
                case OP_TIMES: {
                    if(argtype0==TYPE_I64 && argtype1==TYPE_I64) {
                        return DataElement{DataInt{std::get<DataInt>(arg0.value).value*std::get<DataInt>(arg1.value).value}};
                    } else {
                        throw std::runtime_error(std::format("wrong types binary times: {},{}",argtype0,argtype1));
                    }
                }
                case OP_DIVIDE: {
                    if(argtype0==TYPE_I64 && argtype1==TYPE_I64) {
                        if (std::get<DataInt>(arg1.value).value == 0) {
                            throw std::runtime_error("division by zero");
                        }
                        return DataElement{DataInt{std::get<DataInt>(arg0.value).value/std::get<DataInt>(arg1.value).value}};
                    } else {
                        throw std::runtime_error(std::format("wrong types binary divide: {},{}",argtype0,argtype1));
                    }
                }
                case OP_MODULUS: {
                    if(argtype0==TYPE_I64 && argtype1==TYPE_I64) {
                        if (std::get<DataInt>(arg1.value).value == 0) {
                            throw std::runtime_error("modulus by zero");
                        }
                        return DataElement{DataInt{std::get<DataInt>(arg0.value).value%std::get<DataInt>(arg1.value).value}};
                    } else {
                        throw std::runtime_error(std::format("wrong types binary modulus: {},{}",argtype0,argtype1));
                    }
                }
                default: throw std::runtime_error(std::format("unknown binary op: {}",op));
            }
        } break;
        default: throw std::runtime_error(std::format("do_call_internal only works with 1 or 2 args, found {}",args.size()));
    }
    return DataElement{DataUnbound{}}; // keep compiler happy
}

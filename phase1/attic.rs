const TypeList=1;
const TypeI8=4;
const TypeU8=5;
const TypeI16=6;
const TypeU16=7;
const TypeI32=8;
const TypeU32=9;
const TypeU64=11;
const TypeI128=12;
const TypeU128=13;

base_type("i8") -> true,TypeI8;
base_type("u8") -> true,TypeU8;
base_type("i16") -> true,TypeI16;
base_type("u16") -> true,TypeU16;
base_type("i32") -> true,TypeI32;
base_type("u32") -> true,TypeU32;
base_type("u64") -> true,TypeU64;

get_type_size_pri(TypeI8) -> {0,1};
get_type_size_pri(TypeU8) -> {0,1};
get_type_size_pri(TypeI16) -> {0,2};
get_type_size_pri(TypeU16) -> {0,2};
get_type_size_pri(TypeI32) -> {0,4};
get_type_size_pri(TypeU32) -> {0,4};
get_type_size_pri(TypeU64) -> {0,8};

// LLVM

intsize(0) -> "i8";
intsize(1) -> "i16";
intsize(2) -> "i32";

llvm_tp(TypeI8) -> "i8";
llvm_tp(TypeU8) -> "i8";
llvm_tp(TypeI16) -> "i16";
llvm_tp(TypeU16) -> "i16";
llvm_tp(TypeI32) -> "i32";
llvm_tp(TypeU32) -> "i32";
llvm_tp(TypeU64) -> "i64";

is_unsigned(TypeU8) -> true;
is_unsigned(TypeU16) -> true;
is_unsigned(TypeU32) -> true;
is_unsigned(TypeI64) -> true;

compile_rhs_final_binding(_,sofar,{}) -> sofar;
compile_rhs_final_binding(ctx,sofar,{{tp,src,dst},*rest}) -> compile_rhs_final_binding(ctx,{*sofar,{ctx_fnr(ctx),OpMove+get_scalar_type_size(tp),0,BindReg,dst,BindReg,src}},rest);



#pragma once

#include "defs.hpp"
#include "profiling.hpp"

#if __clang__ || __GNUC__ 
#define BENCHMARK_CONSUME(var_) asm volatile ("" :: "x"(var_))
#define BENCHMARK_CLOBBER_INT(var_) asm volatile ("xor %0, %0" : "=x"(var_))
#define BENCHMARK_SET_INT(var_, val_) asm volatile ("mov %0, %1" : "=x"(var_) : "g"(val_))
#define BENCHMARK_CLOBBER_F64(var_) asm volatile ("vpxor %0, %0, %0" : "=x"(var_))
#define BENCHMARK_CLOBBER_F32(var_) asm volatile ("vpxor %0, %0, %0" : "=x"(var_))
#else
#define BENCHMARK_CONSUME(var_) (void)(var_)
#define BENCHMARK_CLOBBER_INT(var_) (void)(var_)
#define BENCHMARK_SET_INT(var_, val_) (var_) = (val_)
#define BENCHMARK_CLOBBER_F64(var_) (void)(var_)
#define BENCHMARK_CLOBBER_F32(var_) (void)(var_)
#endif

#if __clang__
#define BENCHMARK_SET_F64(var_, val_) asm volatile ("vmovsd %0, %1" : "=x"(var_) : "g"(val_))
#define BENCHMARK_SET_F32(var_, val_) asm volatile ("vmovss %0, %1" : "=x"(var_) : "g"(val_))
#else
#define BENCHMARK_SET_F64(var_, val_) (var_) = (val_)
#define BENCHMARK_SET_F32(var_, val_) (var_) = (val_)
#endif

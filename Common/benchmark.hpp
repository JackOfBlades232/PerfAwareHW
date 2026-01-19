#pragma once

#include "defs.hpp"
#include "profiling.hpp"

#if __clang__ || __GNUC__ 
#define BENCHMARK_CONSUME(var_) asm volatile ("" :: "x"(var_))
#else
#define BENCHMARK_CONSUME(var_) (void)(var_)
#endif

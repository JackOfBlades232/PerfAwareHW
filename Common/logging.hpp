#pragma once

#include "defs.hpp"

#define LOGNORMAL(fmt_, ...) printf(fmt_ "\n", ##__VA_ARGS__)
#define LOGDBG(fmt_, ...) fprintf(stderr, "[DBG] " fmt_ "\n", ##__VA_ARGS__)
#define LOGERR(fmt_, ...) fprintf(stderr, "[ERR] " fmt_ "\n", ##__VA_ARGS__)


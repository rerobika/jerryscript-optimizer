/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#ifndef COMMON_H
#define COMMON_H

#include <algorithm>
#include <assert.h>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifndef DEBUG_DUMP
#define DEBUG_DUMP 0
#endif

#if DEBUG_DUMP
#define LOG(x)                                                                 \
  do {                                                                         \
    std::cout << x << std::endl;                                               \
  } while (0)
#else
#define LOG(x)                                                                 \
  do {                                                                         \
    if (0) {                                                                   \
      std::cout << x << std::endl;                                             \
    }                                                                          \
  } while (0)
#endif

#define unreachable() assert(0)

#endif // COMMON_H

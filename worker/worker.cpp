#include <iostream>

#include "../compiler/compile.hpp"
#include "kafka.hpp"

#include "kafka.cpp"

auto main() -> int {
  return kafka::run_worker();
}

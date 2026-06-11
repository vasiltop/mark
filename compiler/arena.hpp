#pragma once

#include "core.hpp"
#include <cstddef>
#include <cstring>
#include <unistd.h>

namespace arena {

constexpr auto mb(u64 bytes) -> u64 { return bytes << 20; }
constexpr auto kb(u64 bytes) -> u64 { return bytes << 10; }

constexpr auto align_pow_2(u64 value, u64 alignment) -> u64 {
  auto mask{~(alignment - 1)};
  return (value + alignment - 1) & mask;
}

struct Arena {
  u64 res;
  u64 cmt;
  u64 pos;
  u64 cmt_size;
};

inline constexpr u64 HEADER_SIZE{align_pow_2(sizeof(Arena), 8)};

static_assert(sizeof(Arena) <= HEADER_SIZE,
              "Arena struct exceeds the defined header size.");

auto alloc(u64 reserve_size = mb(64), u64 commit_size = kb(64)) -> Arena *;
auto release(Arena *arena) -> void;
auto push_raw(Arena *arena, u64 size, u64 align, bool zero = true) -> void *;
auto pop(Arena *arena, u64 size) -> void;
auto pop_to(Arena *arena, u64 pos) -> void;
auto clear(Arena *arena) -> void;

template <typename T>
auto push(Arena *arena, u64 count = 1, bool zero = true) -> T * {
  return static_cast<T *>(push_raw(arena, sizeof(T) * count, alignof(T), zero));
}

template <typename T> auto pop(Arena *arena, u64 count = 1) -> void {
  pop(arena, sizeof(T) * count);
}

auto push_str(Arena *arena, const char *start, s32 len) -> char *;

struct Temp {
  Arena *arena;
  u64 pos;
};

auto temp_begin(Arena *arena) -> Temp;
auto temp_end(Temp temp) -> void;

auto os_reserve(u64 size) -> void *;
auto os_commit(void *ptr, u64 size) -> bool;
auto os_release(void *ptr, u64 size) -> bool;

} // namespace arena

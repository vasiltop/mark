namespace arena {

auto push_str(Arena *arena, const char *start, s32 len) -> char * {
  auto *dst = static_cast<char *>(push_raw(arena, len + 1, 1, false));
  if (!dst) return nullptr;
  std::memcpy(dst, start, len);
  dst[len] = '\0';
  return dst;
}

auto alloc(u64 reserve_size, u64 commit_size) -> Arena * {
  auto reserve_size_a = align_pow_2(reserve_size, getpagesize());
  auto commit_size_a = align_pow_2(commit_size, getpagesize());

  auto *base = os_reserve(reserve_size_a);
  if (!base)
    return nullptr;
  if (!os_commit(base, commit_size_a)) {
    os_release(base, reserve_size_a);
    return nullptr;
  }

  auto *arena = static_cast<Arena *>(base);
  arena->res = reserve_size_a;
  arena->cmt = commit_size_a;
  arena->pos = HEADER_SIZE;
  arena->cmt_size = commit_size_a;

  return arena;
}

auto release(Arena *arena) -> void {
  if (!arena)
    return;
  os_release(arena, arena->res);
}

auto push_raw(Arena *arena, u64 size, u64 align, bool zero) -> void * {
  if (!arena)
    return nullptr;
  if (align == 0 || (align & (align - 1)) != 0)
    return nullptr;

  auto pos_pre = align_pow_2(arena->pos, align);
  auto pos_pst = pos_pre + size;
  if (pos_pst < pos_pre)
    return nullptr;
  if (arena->res < pos_pst)
    return nullptr;

  std::byte *base = reinterpret_cast<std::byte *>(arena);

  while (arena->cmt < pos_pst) {
    u64 remaining = arena->res - arena->cmt;
    if (remaining == 0)
      return nullptr;
    u64 commit_len = arena->cmt_size < remaining ? arena->cmt_size : remaining;
    if (!os_commit(base + arena->cmt, commit_len))
      return nullptr;
    arena->cmt += commit_len;
  }

  if (zero) {
    u64 size_to_zero = pos_pst - pos_pre;
    std::memset(base + pos_pre, 0, size_to_zero);
  }

  arena->pos = pos_pst;

  return base + pos_pre;
}

auto pop(Arena *arena, u64 size) -> void {
  if (!arena || arena->pos <= HEADER_SIZE || size == 0)
    return;
  auto used = arena->pos - HEADER_SIZE;
  if (size >= used)
    arena->pos = HEADER_SIZE;
  else
    arena->pos -= size;
}

auto pop_to(Arena *arena, u64 pos) -> void {
  if (!arena || pos < HEADER_SIZE)
    return;

  arena->pos = pos;
}

auto clear(Arena *arena) -> void {
  if (!arena)
    return;
  arena->pos = HEADER_SIZE;
}

auto temp_begin(Arena *arena) -> Temp {
  return Temp{.arena = arena, .pos = arena->pos};
}

auto temp_end(Temp temp) -> void { pop_to(temp.arena, temp.pos); }

#if defined(__linux__) || defined(__APPLE__)
#include <sys/mman.h>
auto os_reserve(u64 size) -> void * {
  auto *result = mmap(0, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (result == MAP_FAILED) {
    return nullptr;
  }

  return result;
}

auto os_commit(void *ptr, u64 size) -> bool {
  return mprotect(ptr, size, PROT_READ | PROT_WRITE) == 0;
}

auto os_release(void *ptr, u64 size) -> bool {
  if (!ptr)
    return false;
  return munmap(ptr, size) == 0;
}
#elif defined(_WIN32)
#include <windows.h>

auto os_reserve(u64 size) -> void * {
  return VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
}

auto os_commit(void *ptr, u64 size) -> bool {
  return VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE) != nullptr;
}

auto os_release(void *ptr, u64 size) -> bool {
  return VirtualFree(ptr, 0, MEM_RELEASE) != 0;
}
#endif
} // namespace arena

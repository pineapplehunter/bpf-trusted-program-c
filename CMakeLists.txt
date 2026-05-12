cmake_minimum_required(VERSION 4.1.2)
project(trust-filter C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- userspace binary (links libbpf) ----
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBBPF REQUIRED IMPORTED_TARGET libbpf)

add_executable(trust-filter trust-filter.c)
target_link_libraries(trust-filter PRIVATE PkgConfig::LIBBPF)
target_compile_options(trust-filter PRIVATE -g -O2 -Wall)

# ---- BPF program (cross-compiled with clang -target bpf) ----
# Nix-wrapped clang injects host flags; use CLANG env var if set.
if(DEFINED ENV{CLANG})
  set(CLANG_BPF "$ENV{CLANG}")
else()
  find_program(CLANG_BPF clang REQUIRED)
endif()

set(BPF_FLAGS
  -g -O2
  -target bpf
  -Wno-missing-declarations
)
foreach(_dir ${LIBBPF_INCLUDE_DIRS})
  list(APPEND BPF_FLAGS "-I${_dir}")
endforeach()

set(BPF_SRC  "${CMAKE_CURRENT_SOURCE_DIR}/trust-filter.bpf.c")
set(BPF_OUT  "${CMAKE_CURRENT_BINARY_DIR}/trust-filter.bpf.o")

add_custom_command(
  OUTPUT  "${BPF_OUT}"
  COMMAND "${CLANG_BPF}" ${BPF_FLAGS} -c "${BPF_SRC}" -o "${BPF_OUT}"
  DEPENDS "${BPF_SRC}" "${CMAKE_CURRENT_SOURCE_DIR}/vmlinux.h"
  COMMENT "Compiling BPF program (clang -target bpf)…"
  VERBATIM
)
add_custom_target(bpf ALL DEPENDS "${BPF_OUT}")

add_dependencies(trust-filter bpf)

# ---- convenience targets ----
add_custom_target(run
  COMMAND sudo "$<TARGET_FILE:trust-filter>" "${BPF_OUT}"
  DEPENDS trust-filter bpf
  USES_TERMINAL
)

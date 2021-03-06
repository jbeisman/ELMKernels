
#pragma once

#ifndef ENABLE_KOKKOS
#define ACCELERATE
constexpr bool KOKKOS_ENABLED = false;
namespace NS = ELM;
#else
#define ACCELERATE KOKKOS_INLINE_FUNCTION
constexpr bool KOKKOS_ENABLED = true;
#include "Kokkos_Core.hpp"
namespace NS = Kokkos;
#endif

#include "kokkos_types.hh"

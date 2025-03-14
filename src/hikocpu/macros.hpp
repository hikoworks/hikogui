// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#if defined(_MSC_BUILD)
#include <intrin.h>
#endif
#include <version>
#include <exception>

// All the HI_HAS_* macros tell us if the compiler will generate code with these instructions.
// Therefor we can use intrinsics for these instructions without checking the cpu-id.
// Which intrinsics are available is handled by a different macro.

#if defined(HI_GENERIC)
// Nothing defined when generic.

// MSVC platform detection.
//  - _M_AMD64 determines if the processor is 64 bit, both x86-64 and arm64.
//  - _M_IX86 is not defined on x86-64, but _M_IX86_FP is.
#elif defined(_M_IX86_FP)
#define HI_HAS_X86 1
#if defined(_M_AMD64)
#define HI_HAS_X86_64 1
#endif
#elif defined(_M_ARM_FP)
#define HI_HAS_ARM 1
#if defined(_M_AMD64)
#define HI_HAS_ARM64 1
#endif

// clang/gcc platform detection
#elif defined(__amd64__) or defined(__amd64) or defined(__x86_64__) or defined(__x86_64) or defined(_M_AMD64) or defined(_M_X64)
#define HI_HAS_X86 1
#define HI_HAS_X86_64 1
#elif defined(__aarch64__) or defined(_M_ARM64)
#define HI_HAS_ARM 1
#define HI_HAS_ARM64 1
#elif defined(__i386__) or defined(_M_IX86)
#define HI_HAS_X86 1
#elif defined(__arm__) or defined(__arm) or defined(_ARM) or defined(_M_ARM)
#define HI_HAS_ARM 1
#endif

#if defined(HI_GENERIC)
// Don't add any cpu features.

// Detect MSVC /arch option for x86 features.
#elif defined(_M_IX86_FP)
// MSVC /arch:SSE (x86)
#if _M_IX86_FP >= 1 or defined(HI_HAS_X86_64)
#define HI_HAS_SSE 1
#define HI_HAS_MMX 1
#define HI_HAS_SCE 1
#define HI_HAS_OSFXSR 1
#define HI_HAS_FXSR 1
#define HI_HAS_FPU 1
#define HI_HAS_CX8 1
#define HI_HAS_CMOV 1
#endif

// MSVC /arch:SSE2 (x86)
#if _M_IX86_FP >= 2 or defined(HI_HAS_X86_64)
#define HI_HAS_SSE2 1
#endif

// MSVC /arch:AVX (x86-64-v2)
#if defined(__AVX__)
#define HI_HAS_SSSE3 1
#define HI_HAS_SSE4_1 1
#define HI_HAS_SSE4_2 1
#define HI_HAS_SSE3 1
#define HI_HAS_POPCNT 1
#define HI_HAS_LAHF 1
#define HI_HAS_CX16 1
#define HI_HAS_AVX 1
#endif

// MSVC /arch:AVX2 (x86-64-v3)
#if defined(__AVX2__)
#define HI_HAS_AVX2 1
#define HI_HAS_BMI1 1
#define HI_HAS_BMI2 1
#define HI_HAS_F16C 1
#define HI_HAS_FMA 1
#define HI_HAS_LZCNT 1
#define HI_HAS_MOVBE 1
#define HI_HAS_OSXSAVE 1
#endif

// MSVC /arch:AVX512 (x86-64-v4)
#if defined(__AVX512F__) and defined(__AVX512BW__) and defined(__AVX512CD__) and defined(__AVX512DQ__) and defined(__AVX512VL__)
#define HI_HAS_AVX512F 1
#define HI_HAS_AVX512BW 1
#define HI_HAS_AVX512CD 1
#define HI_HAS_AVX512DQ 1
#define HI_HAS_AVX512VL 1
#endif

// Detect MSVC /arch option for arm features.
#elif defined(_M_ARM_FP)
#if _M_ARM_FP >= 30 and _M_ARM_FP < 40
#define HI_HAS_VFP3
#endif
#if _M_ARM_FP >= 40 and _M_ARM_FP < 50
#define HI_HAS_VFP4
#endif

// Check for other CPU features from individual macro definition on compilers
// other than MSVC.
#else
#if defined(__MMX__)
#define HI_HAS_MMX 1
#endif
#if defined(__SSE__)
#define HI_HAS_SSE 1
#endif
#if defined(__SSE2__)
#define HI_HAS_SSE2 1
#endif
#if defined(__SSE3__)
#define HI_HAS_SSE3 1
#endif
#if defined(__SSSE3__)
#define HI_HAS_SSSE3 1
#endif
#if defined(__SSE4_1__)
#define HI_HAS_SSE4_1 1
#endif
#if defined(__SSE4_2__)
#define HI_HAS_SSE4_2 1
#endif
#if defined(__POPCNT__)
#define HI_HAS_POPCNT 1
#endif
#if defined(__LAHF_SAHF__)
#define HI_HAS_LAHF 1
#endif
#if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16)
#define HI_HAS_CX16 1
#endif
#if defined(__AVX__)
#define HI_HAS_AVX 1
#endif
#if defined(__AVX2__)
#define HI_HAS_AVX2 1
#endif
#if defined(__BMI__)
#define HI_HAS_BMI1 1
#endif
#if defined(__BMI2__)
#define HI_HAS_BMI2 1
#endif
#if defined(__F16C__)
#define HI_HAS_F16C 1
#endif
#if defined(__FMA__)
#define HI_HAS_FMA 1
#endif
#if defined(__LZCNT__)
#define HI_HAS_LZCNT 1
#endif
#if defined(__MOVBE__)
#define HI_HAS_MOVBE 1
#endif
#if defined(__XSAVE__)
#define HI_HAS_XSAVE 1
#endif
#if defined(__AVX512BW__)
#define HI_HAS_AVX512BW 1
#endif
#if defined(__AVX512CD__)
#define HI_HAS_AVX512CD 1
#endif
#if defined(__AVX512DQ__)
#define HI_HAS_AVX512DQ 1
#endif
#if defined(__AVX512F__)
#define HI_HAS_AVX512F 1
#endif
#if defined(__AVX512VL__)
#define HI_HAS_AVX512VL 1
#endif
#if defined(__SHA__)
#define HI_HAS_SHA 1
#endif
#if defined(__AES__)
#define HI_HAS_AES 1
#endif
#if defined(__PCLMUL__)
#define HI_HAS_PCLMUL 1
#endif
#if defined(__RDRND__)
#define HI_HAS_RDRND 1
#endif
#if defined(__RDSEED__)
#define HI_HAS_RDSEED 1
#endif
#endif

// clang-format off
#if defined(HI_HAS_SSE2) and defined(HI_HAS_SSE) and defined(HI_HAS_SCE) and \
    defined(HI_HAS_OSFXSR) and defined(HI_HAS_MMX) and defined(HI_HAS_FXSR) and \
    defined(HI_HAS_FPU) and defined(HI_HAS_CX8) and defined(HI_HAS_CMOV)
#define HI_HAS_X86_64_V1 1
#endif

#if defined(HI_HAS_X86_64_V1) and defined(HI_HAS_SSSE3) and defined(HI_HAS_SSE4_1) and \
    defined(HI_HAS_SSE4_2) and defined(HI_HAS_SSE3) and defined(HI_HAS_POPCNT) and \
    defined(HI_HAS_LAHF) and defined(HI_HAS_CX16)
#define HI_HAS_X86_64_V2 1
#endif

#if defined(HI_HAS_X86_64_V2) and defined(HI_HAS_AVX) and defined(HI_HAS_AVX2) and \
    defined(HI_HAS_BMI1) and defined(HI_HAS_BMI2) and defined(HI_HAS_F16C) and \
    defined(HI_HAS_FMA) and defined(HI_HAS_LZCNT) and defined(HI_HAS_MOVBE) and \
    defined(HI_HAS_OSXSAVE)
#define HI_HAS_X86_64_V3 1
#endif

#if defined(HI_HAS_X86_64_V3) and defined(HI_HAS_AVX512F) and defined(HI_HAS_AVX512BW) and \
    defined(HI_HAS_AVX512CD) and defined(HI_HAS_AVX512DQ) and defined(HI_HAS_AVX512VL)
#define HI_HAS_X86_64_V4 1
#endif
// clang-format on

#ifndef hi_stringify
#define hi_stringify_(x) #x
#define hi_stringify(x) hi_stringify_(x)
#endif

#ifndef hi_keywords
#define hi_keywords
#define hi_export
#define hi_export_module(...)
#if defined(__clang__) or defined(__GNUC__)
#define hi_target(...) [[gnu::target(__VA_ARGS__)]]
#define hi_no_inline [[gnu::noinline]]
#define hi_force_inline [[gnu::always_inline]]
#define hi_restrict __restrict__
#define hi_warning_push() _Pragma("warning(push)")
#define hi_warning_pop() _Pragma("warning(push)")
#define hi_warning_ignore_msvc(...)
#define hi_warning_ignore_clang(...) _Pragma(hi_stringify(clang diagnostic ignored __VA_ARGS__))
#define hi_warning_ignore_gcc(...)
#define hi_no_sanitize_address
#define hi_assume(...) __builtin_assume(not not(__VA_ARGS__))
#define hi_assert_break() __builtin_trap()
#define hi_debug_break() __builtin_debugtrap()
#elif defined(_MSC_BUILD)
#define hi_target(...)
#define hi_no_inline [[msvc::noinline]]
#define hi_force_inline [[msvc::forceinline]]
#define hi_restrict __restrict
#define hi_warning_push() _Pragma("warning( push )")
#define hi_warning_pop() _Pragma("warning( pop )")
#define hi_warning_ignore_msvc(...) _Pragma(hi_stringify(warning(disable : __VA_ARGS__)))
#define hi_warning_ignore_clang(...)
#define hi_warning_ignore_gcc(...)
#define hi_no_sanitize_address __declspec(no_sanitize_address)
#define hi_assume(...) __assume(not not(__VA_ARGS__))
#define hi_assert_break() __int2c()
#define hi_debug_break() __debugbreak()
#else
#define hi_target(...)
#define hi_no_inline
#define hi_force_inline
#define hi_restrict
#define hi_warning_push()
#define hi_warning_pop()
#define hi_warning_ignore_msvc(...)
#define hi_warning_ignore_clang(...)
#define hi_warning_ignore_gcc(...)
#define hi_no_sanitize_address
#define hi_assume(...) static_assert(std::convertible_to<decltype(not not(__VA_ARGS__)), bool>)
#define hi_assert_break() std::terminate()
#define hi_debug_break() std::terminate()
#endif
#endif

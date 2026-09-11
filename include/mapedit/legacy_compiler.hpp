#pragma once

// Compatibility surface for the original MapEdit function-codegen lane.
// The exact retail PE has a HYBRID Rich header: most project C++ objects are
// Utc12_CPP build 8799 (VC6), while only one C and one C++ linked input carry
// Utc13 build 8830.  Tools that collapse Rich metadata to one label may thus
// display "13.00.8830" for the whole EXE even though that is not the compiler
// identity of the reconstructed engine owners.  For per-function byte/CFG
// auditing, the native lane is validated by emitted PDB/CodeView 12.00.8799.0.
// Keep this header intentionally tiny: it only
// maps syntax introduced after VC6 to constructs that preserve the same
// x86 ABI.  It must not alter runtime semantics.

#if (defined(_MSC_VER) && _MSC_VER < 1300) || defined(MAPEDIT_LEGACY_CPP98)
#  define MAPEDIT_JOIN_IMPL(a,b) a##b
#  define MAPEDIT_JOIN(a,b) MAPEDIT_JOIN_IMPL(a,b)
#  define typedef char MAPEDIT_JOIN(mapedit_static_assert_,__LINE__)[(expr) ? 1 : -1]
#  define constexpr const
#  define nullptr 0
#  define override
#  define final
#  define noexcept
#endif
// Small CRT spelling bridge used by the VC6 lane.  Keep call sites explicit
// instead of teaching the reconstructed runtime different error semantics.
#if defined(_MSC_VER) && _MSC_VER < 1300
#  define MAPEDIT_SNPRINTF _snprintf
#else
#  define MAPEDIT_SNPRINTF ::snprintf
#endif


# libbitcoin-system C++ Coding Style Guide

Derived from analysis of the existing codebase. All new contributions should follow these conventions.

---

## 1. Naming Conventions

### Files & Directories
- All lowercase with underscores: `transaction.hpp`, `byte_reader.cpp`
- Headers in `include/bitcoin/system/<subsystem>/`, sources in `src/<subsystem>/`
- Subsystem directories: `chain/`, `machine/`, `crypto/`, `math/`, `unicode/`, `stream/`

### Classes & Types
- `snake_case` for class/struct/enum names: `binary`, `transaction`, `byte_reader`, `opcode`
- Enum classes use `snake_case` throughout: `script_version`, `coverage`
- Template parameters: descriptive `PascalCase` — `Integer`, `Value`, `Size`, `Bits`, `Bytes`, `Type`

### Functions & Methods
- `snake_case`: `is_valid()`, `to_data()`, `read_byte()`, `decompose_one()`
- Trailing underscore suffix marks *unsafe* variants (caller must pre-validate):
  ```cpp
  INLINE chunk_xptr pop_chunk_() NOEXCEPT;
  INLINE bool pop_strict_bool_() NOEXCEPT;
  ```

### Variables
- Local variables: `snake_case` — `witness_size`, `message_size`
- Member variables: `snake_case_` with **trailing underscore** — `bits_`, `bytes_`, `arena_`
- Constants: `snake_case` (constexpr) — `hangul_sbase`, `max_uint32`, `ec_even_sign`

### Macros
| Prefix | Purpose | Example |
|--------|---------|---------|
| `BC_` | Library-level utilities | `BC_API`, `BC_ASSERT`, `BC_PUSH_WARNING` |
| `HAVE_` | Platform/feature detection | `HAVE_LINUX`, `HAVE_GNUC`, `HAVE_ICU` |
| `WITH_` | Build-time options | `WITH_SHA`, `WITH_512` |
| `NO_` | Disabled-feature/warning labels | `NO_THROW_IN_NOEXCEPT` |

### Namespaces
- Hierarchy: `libbitcoin::system::chain`, `libbitcoin::system::machine`, etc.
- Public convenience alias: `namespace bc = libbitcoin;`
- No `using namespace` in headers — ever.

---

## 2. File & Header Organization

### Include Guards
```cpp
#ifndef LIBBITCOIN_SYSTEM_CHAIN_TRANSACTION_HPP
#define LIBBITCOIN_SYSTEM_CHAIN_TRANSACTION_HPP
// ...
#endif
```

### Include Order
1. Standard library headers (`<memory>`, `<optional>`, `<algorithm>`)
2. Project headers (`<bitcoin/system/...>`)

No blank lines between groups; order is strict.

### Header Structure
```cpp
// Copyright/license block
// Include guards
// Standard includes
// Project includes

namespace libbitcoin {
namespace system {
namespace chain {

// Forward declarations
// Type aliases (typedef / using)
// Class declaration

} // namespace chain
} // namespace system
} // namespace libbitcoin
#endif
```

### Closing Namespace Comments
Every closing brace at namespace scope gets a trailing comment:
```cpp
} // namespace chain
} // namespace system
} // namespace libbitcoin
```

---

## 3. Code Formatting

### Indentation
- **4 spaces** — no tabs.

### Brace Style (Allman)
Opening brace always on its own line for classes, functions, and control blocks:
```cpp
class transaction
{
public:
    bool is_valid() const NOEXCEPT;

private:
    size_t bits_;
};

bool transaction::is_valid() const NOEXCEPT
{
    return !inputs_.empty();
}
```

Exception: compact single-statement bodies may appear inline:
```cpp
if (condition) { return value; }
```

### Whitespace
- Space after keywords: `if (`, `for (`, `while (`, `template <`
- No space before `(` in function calls: `to_data()` not `to_data ()`
- Spaces around binary operators: `value = 5`, `left && right`
- Pointer/reference declarators attach to the type: `const data_chunk&`, `shared_ptr<T>`

---

## 4. Class & Struct Design

### Section Order
```cpp
class BC_API transaction
{
public:
    // 1. Type aliases
    typedef std::shared_ptr<const transaction> cptr;

    // 2. Static methods
    static bool is_relative_locktime_applied(...) NOEXCEPT;

    // 3. Constructors / destructor (via macro or explicit)
    transaction() NOEXCEPT;

    // 4. Operators
    bool operator==(const transaction& other) const NOEXCEPT;

    // 5. Serialization
    data_chunk to_data(bool witness) const NOEXCEPT;

    // 6. Properties / accessors
    bool is_valid() const NOEXCEPT;
    size_t inputs() const NOEXCEPT;

protected:
    // ...

private:
    // Members last, each with trailing underscore
    size_t bits_;
    data_chunk bytes_;
};
```

### Visual Section Dividers
```cpp
/// Constructors.
/// -----------------------------------------------------------------------

/// Properties.
/// -----------------------------------------------------------------------
```

### Copy/Move/Destruct Macros
```cpp
// Permit all compiler-generated specials:
DEFAULT_COPY_MOVE_DESTRUCT(transaction)

// Delete them:
DELETE_COPY_MOVE_DESTRUCT(transaction)
```

Prefer macros over manual declarations to avoid omissions.

### Const Correctness
- Getter methods are always `const NOEXCEPT`
- Parameters passed by `const&` to avoid copies
- No public data members — strict encapsulation

---

## 5. Memory Management

### Shared Pointer Naming Convention
```
T::ptr      → shared_ptr<T>
T::cptr     → shared_ptr<const T>
[name]_ptr  → shared_ptr<T>
[name]_cptr → shared_ptr<const T>
[name]s_ptr → shared_ptr<collection<T>>
[name]s_cptr→ shared_ptr<const collection<T>>
[name]s     → collection<T>
[name]_ptrs → collection<shared_ptr<T>>
[name]_cptrs→ collection<shared_ptr<const T>>
[name]_xptr → external_ptr<T>  (non-owning)
```

### Allocators
`std_vector<T>` is a project alias using a custom allocator:
```cpp
template <typename Type>
using std_vector = std::vector<Type, allocator<Type>>;
```
Use `std_vector` instead of `std::vector` in project code.

### No Raw Pointer Ownership
Raw pointers appear only for non-owning references inside implementation details. Prefer `shared_ptr`, references, or `external_ptr`.

---

## 6. Type System

### Strong Typedefs for Domain Types
```cpp
static constexpr size_t ec_secret_size = 32;
typedef data_array<ec_secret_size> ec_secret;
typedef data_array<33>             ec_compressed;
typedef data_array<65>             ec_uncompressed;
typedef data_array<hash_size>      hash_digest;
```

### Collection Typedefs
```cpp
typedef std_vector<ec_secret>     secret_list;
typedef std_vector<ec_compressed> compressed_list;
```

### Type Selector Templates
```cpp
template <size_t Bytes = 0u>
using signed_type = /* conditional by size */;

template <bool Condition, typename IfTrue, typename IfFalse>
using iif = std::conditional_t<Condition, IfTrue, IfFalse>;
```

### Constexpr Literals
```cpp
constexpr ec_compressed ec_compressed_generator = base16_array(
    "0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798");
```

---

## 7. Templates & Constraints

### SFINAE Constraint Aliases (preferred over `requires`)
```cpp
template <typename Type>
using if_one_byte = bool_if<is_same_size<Type, uint8_t>>;

template <uint64_t Value>
using if_nonzero = bool_if<is_nonzero(Value)>;
```

### Template Function Signature Pattern
```cpp
template <typename Integer, size_t Size = sizeof(Integer),
    if_integer<Integer> = true,
    if_not_greater<Size, sizeof(Integer)> = true>
Integer read_big_endian() NOEXCEPT;
```

### Type Traits
Use project-defined traits where they exist:
- `is_same_type<L, R>` over `std::is_same_v` + `std::decay_t`
- `is_integral<Type>` for Bitcoin integer semantics (excludes `bool`)
- `is_same_size<A, B>` for size equality

---

## 8. Error Handling

### No Exceptions for Normal Control Flow
Exceptions are reserved for unrecoverable errors (stream failure, OOM, math overflow):
```cpp
// In math:
throw overflow_exception{ "literal overflow" };

// In streams:
throw istream_exception{};
```

### Return Codes
Fallible operations return `bool` or `code` (`std::error_code` alias):
```cpp
using code = std::error_code;
bool is_valid() const NOEXCEPT;
```

### NOEXCEPT Macro
```cpp
// Release builds: strict noexcept
// Debug builds: no specifier (allows assert/throw to surface)
#if defined(NDEBUG)
    #define NOEXCEPT noexcept
    #define THROWS  noexcept(false)
#else
    #define NOEXCEPT
    #define THROWS
#endif
```

Apply `NOEXCEPT` to all methods that do not intentionally throw. Apply `THROWS` to wrappers around throwing code.

---

## 9. Comments & Documentation

### File Header
```cpp
/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: ...
 */
```

### Method/Function Comments
Triple-slash Doxygen-style on declarations:
```cpp
/// Determine if the flag is enabled in the active flags set.
static constexpr bool is_enabled(uint32_t active_flags, flags flag) NOEXCEPT;
```

### Section Dividers
```cpp
/// Constructors.
/// -----------------------------------------------------------------------

// Local helpers.
// --------------------------------------------------------------------------
```

Triple-slash (`///`) for API-visible sections; double-slash (`//`) for implementation-internal sections.

### Algorithm References
When implementing a non-obvious algorithm, cite the specification:
```cpp
// unicode.org/reports/tr15/#Hangul
```

### Philosophy
- Comments explain *why*, not *what*
- Self-documenting code is preferred; no redundant inline paraphrasing
- No `@param` / `@return` Doxygen tags — prose summaries only

---

## 10. Serialization Pattern

Every serializable type exposes parallel constructor/method overloads for all stream types:

```cpp
// Deserialization constructors
transaction(const data_slice& data, bool witness) NOEXCEPT;
transaction(stream::in::fast& stream, bool witness) NOEXCEPT;
transaction(std::istream& stream, bool witness) NOEXCEPT;
transaction(reader& source, bool witness) NOEXCEPT;

// Serialization methods
data_chunk to_data(bool witness) const NOEXCEPT;
void to_data(std::ostream& stream, bool witness) const NOEXCEPT;
void to_data(writer& sink, bool witness) const NOEXCEPT;
```

---

## 11. Platform Macros

### Compiler / Platform Detection
```cpp
#if defined(HAVE_GNUC)   // GCC
#if defined(HAVE_MSC)    // MSVC
#if defined(HAVE_CLANG)  // Clang
#if defined(HAVE_LINUX)
#if defined(HAVE_APPLE)
#if defined(HAVE_FREEBSD)
```

### Compiler Attribute Macros
```cpp
INLINE     // always_inline / __forceinline
NODISCARD  // [[nodiscard]]
FALLTHROUGH// [[fallthrough]]
DEPRECATED // [[deprecated]]
```

### Warning Suppression
```cpp
BC_PUSH_WARNING(NO_DYNAMIC_ARRAY_INDEXING)
// ... code that triggers the warning ...
BC_POP_WARNING()
```

Use targeted push/pop pairs — never file-wide suppression.

### Debug Assertions
```cpp
BC_ASSERT(expression);        // active in debug, noop in release
BC_DEBUG_ONLY(expression);    // expression only compiled in debug
```

---

## 12. Key Patterns Summary

| Pattern | Rule |
|---------|------|
| Member variables | `name_` (trailing underscore) |
| Unsafe helper methods | `method_()` (trailing underscore) |
| Const methods | Always mark `const NOEXCEPT` |
| NOEXCEPT | Apply to all non-throwing methods |
| Smart pointers | `cptr` = `shared_ptr<const T>`; `ptr` = `shared_ptr<T>` |
| Collections | `std_vector<T>` (project alias with custom allocator) |
| Brace style | Allman — opening brace on own line |
| Indentation | 4 spaces, no tabs |
| Namespace closing | `} // namespace name` |
| Macros | ALL_CAPS; `BC_` / `HAVE_` / `WITH_` / `NO_` prefixes |
| Error handling | `bool` / `code` return; exceptions only for unrecoverable errors |
| Serialization | Parallel reader/writer overloads on every serializable type |

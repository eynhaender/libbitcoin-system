# Interpreter Unit Tests — Implementation Guide

## Overview

This document describes the unit-test suite added under `test/machine/interpreter_op_*.cpp`.
The suite tests each `interpreter<Stack>` opcode handler individually, at the C++ function
level, without involving script parsing, transaction validation, or any network-layer code.

The design follows the Hoare-triple formal model described in
`doc/formal-model-explanation.md` and the testing strategy outlined in
`doc/unit-test-concept-interpreter.md`.

---

## Why Per-Opcode Unit Tests?

The upstream libbitcoin maintainer explicitly rejected acceptance-style tests (feeding
full script vectors from Bitcoin Core and asserting pass/fail) as "not unit tests".
Acceptance tests:

- Hide which individual handler is responsible for a failure.
- Re-test the script dispatch engine and transaction infrastructure alongside the op logic.
- Produce low diagnostic signal: a single JSON vector covers dozens of opcodes.

Per-opcode unit tests address all three concerns.  Each test case specifies exactly one
opcode handler, exactly one input configuration (the precondition), and exactly one
expected result (the postcondition).

---

## Formal Model

Every test case is a **Hoare triple**:

```
{P}  C  {Q}
```

| Symbol | Meaning |
|--------|---------|
| `P`    | Precondition — the stack contents and active flags before the call |
| `C`    | Command — exactly one call to one `op_*()` method |
| `Q`    | Postcondition — the return code and the stack contents after the call |

### Example

```
{P: stack = [{0x01}, {0x02}]}
  op_swap()
{Q: return op_success, stack = [{0x02}, {0x01}]}
```

Translated into a test:

```cpp
BOOST_AUTO_TEST_CASE(interpreter__op_swap__two_items__swapped)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });   // bottom
    f.push(data_chunk{ 0x02 });   // top
    BOOST_REQUIRE_EQUAL(f.op_swap(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x02 }));  // was bottom
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));  // was top
}
```

Wait — `op_swap` swaps, so after the swap top becomes `{0x01}` and bottom becomes `{0x02}`.
The postcondition is what matters, not the initial push order.

---

## Test Naming Convention

```
<suite>__<opcode_name>__<condition>__<expected_outcome>
```

Examples:

| Test name | Meaning |
|-----------|---------|
| `interpreter__op_swap__empty_stack__op_swap` | `op_swap` on empty stack returns `error::op_swap` |
| `interpreter__op_add__two_positives__sum_on_stack` | `op_add` on two positive numbers leaves the sum |
| `interpreter__op_return__nops_flag__op_reserved` | `op_return` with `nops_rule` active returns `error::op_reserved` |

The suite name matches the file: `interpreter_op_stack_tests`, `interpreter_op_arithmetic_tests`, etc.

---

## Test Fixture

### Problem

`interpreter<Stack>` stores a `const chain::transaction&` (a reference, not a value).
The constructor takes `(const transaction&, const input_iterator&, uint32_t flags)`.
A naive subclass would need to hold the transaction _and_ pass a reference to it — but
if the transaction is a member of the derived class, it does not exist yet when the base
constructor runs, causing undefined behaviour.

### Solution — Base-from-Member Idiom

A private base class `interpreter_storage` holds the transaction.  Because base classes
are constructed in declaration order, listing `interpreter_storage` before
`interpreter<contiguous_stack>` guarantees the transaction is alive when the interpreter
constructor binds its reference:

```cpp
struct interpreter_storage {
    chain::transaction tx{};
    interpreter_storage() NOEXCEPT {
        chain::inputs in;
        in.emplace_back(chain::point{}, chain::script{}, 0u);
        tx = chain::transaction(1u, std::move(in), chain::outputs{}, 0u);
    }
};

class interpreter_fixture
  : private interpreter_storage,
    public interpreter<contiguous_stack>
{
public:
    explicit interpreter_fixture(uint32_t flags = 0u) NOEXCEPT
      : interpreter_storage{}
      , interpreter<contiguous_stack>(
            interpreter_storage::tx,
            interpreter_storage::tx.inputs_ptr()->begin(),
            flags)
    {}
    // ...
};
```

The dummy transaction satisfies `interpreter`'s requirements:
- version = 1
- one input with `sequence = 0` (not `0xffffffff`, so `is_final()` returns false — needed
  for `op_check_locktime_verify` tests)
- no outputs, locktime = 0

### Lifting Protected Methods

All `op_*` handlers are `protected` in `interpreter<Stack>`.  The fixture lifts them to
`public` via `using` declarations:

```cpp
using interpreter::op_swap;
using interpreter::op_add;
// ... all 87 ops
```

No mocking, no friend declarations, no changes to the production class.

### Stack Helpers

| Helper | Description |
|--------|-------------|
| `push(data_chunk)` | Push a raw byte chunk |
| `push(int64_t)` | Push a script integer (stored as variant) |
| `push(bool)` | Push a script boolean |
| `pop_top()` | Pop and return the top item as `data_chunk` |
| `size()` | Number of items on the primary stack |
| `empty()` | True when the primary stack has zero items |
| `top()` | Const reference to top `stack_variant` |

---

## File Layout

| File | Suite | Opcodes covered |
|------|-------|-----------------|
| `test/machine/interpreter_fixture.hpp` | — | Fixture definition |
| `test/machine/interpreter_op_push.cpp` | `interpreter_op_push_tests` | `op_push_number`, `op_push_size`, `op_push_one_size`, `op_push_two_size`, `op_push_four_size`, `op_nop` |
| `test/machine/interpreter_op_flow.cpp` | `interpreter_op_flow_tests` | `op_verify`, `op_return`, `op_ver`, `op_verif`, `op_vernotif`, `op_if`, `op_notif`, `op_else`, `op_endif` |
| `test/machine/interpreter_op_stack.cpp` | `interpreter_op_stack_tests` | `op_swap`, `op_dup`, `op_drop`, `op_over`, `op_rot`, `op_nip`, `op_tuck`, `op_depth`, `op_if_dup`, `op_to_alt_stack`, `op_from_alt_stack`, `op_drop2`, `op_dup2`, `op_dup3`, `op_over2`, `op_rot2`, `op_swap2`, `op_pick`, `op_roll` |
| `test/machine/interpreter_op_arithmetic.cpp` | `interpreter_op_arithmetic_tests` | `op_add1`, `op_sub1`, `op_negate`, `op_abs`, `op_not`, `op_nonzero`, `op_add`, `op_sub`, `op_bool_and`, `op_bool_or`, `op_num_equal`, `op_num_equal_verify`, `op_num_not_equal`, `op_less_than`, `op_greater_than`, `op_less_than_or_equal`, `op_greater_than_or_equal`, `op_min`, `op_max`, `op_within` |
| `test/machine/interpreter_op_bitwise.cpp` | `interpreter_op_bitwise_tests` | `op_equal`, `op_equal_verify`, `op_size` |
| `test/machine/interpreter_op_crypto.cpp` | `interpreter_op_crypto_tests` | `op_ripemd160`, `op_sha1`, `op_sha256`, `op_hash160`, `op_hash256`, `op_check_sig_verify` (error paths), `op_check_multisig_verify` (error paths), `op_check_locktime_verify`, `op_check_sequence_verify` |

---

## Coverage Strategy

Each opcode requires at minimum four test cases:

1. **Error path(s)**: underflow, type mismatch, or constraint violation — the handler must
   return the correct `error::op_*` code and leave the stack unchanged where specified.
2. **Happy path**: the canonical success case — correct return code and correct stack state.
3. **Edge cases**: boundary values, empty inputs, maximal values, single-byte data.
4. **Flag sensitivity** (where applicable): handlers whose behaviour differs with
   `nops_rule`, `bip65_rule`, `bip112_rule`, `bip342_rule` are tested both with and
   without the relevant flag.

---

## Script Number Encoding Reference

Bitcoin Script integers use a variable-length little-endian sign-magnitude encoding.
Tests use raw `data_chunk` assertions; this table translates:

| Logical value | Encoded bytes |
|---------------|---------------|
| 0 | `{}` (empty) |
| 1 | `{0x01}` |
| -1 | `{0x81}` |
| 16 | `{0x10}` |
| 127 | `{0x7f}` |
| 128 | `{0x80, 0x00}` |
| -128 | `{0x80, 0x80}` |
| 32767 | `{0xff, 0x7f}` |
| 32768 | `{0x00, 0x80, 0x00}` |

The encoding is implemented in `include/bitcoin/system/impl/machine/number_chunk.ipp`.

**Pitfall — sign bit in the last byte**: `{0x00, 0x00, 0x00, 0x80}` looks like
0x80000000 but is actually *negative zero* (the `0x80` last byte has its high bit set as
the sign flag, leaving magnitude = 0).  To represent 0x80000000 (= 2147483648) use the
5-byte form `{0x00, 0x00, 0x00, 0x80, 0x00}`, or push via `push(int64_t(0x80000000LL))`
which lets the fixture encode it correctly.  This matters for opcodes that call
`peek_unsigned32`, such as `op_check_sequence_verify` with `bip112_rule`.

---

## Flag-Gated Opcodes

Several opcodes change behaviour based on the active-flags bitmask passed to the fixture
constructor:

| Opcode | Flag | Without flag | With flag |
|--------|------|-------------|-----------|
| `op_return` | `nops_rule` | `op_not_implemented` | `op_reserved` |
| `op_ver` | `nops_rule` | `op_not_implemented` | `op_reserved` |
| `op_verif` | `nops_rule` | `op_not_implemented` | `op_invalid` |
| `op_vernotif` | `nops_rule` | `op_not_implemented` | `op_invalid` |
| `op_check_locktime_verify` | `bip65_rule` | `op_success` (acts as NOP2) | enforces locktime |
| `op_check_sequence_verify` | `bip112_rule` | `op_success` (acts as NOP3) | enforces sequence |
| `op_if` / `op_notif` | `bip342_rule` | any top value | strict minimal bool |

`op_return`, `op_ver` → `op_unevaluated` returns `op_reserved` because opcodes 98–100 are
classified as reserved (not invalid).  `op_verif` (101) and `op_vernotif` (102) are
classified as **`is_invalid`** in the opcode enum, so `op_unevaluated` returns `op_invalid`
instead.

`op_check_multisig_verify` with `bip342_rule`: the bip342 code path inside the interpreter
only activates when the transaction is executing a tapscript witness program (the
interpreter carries the script version in its state).  Passing `bip342_rule` as a flag
alone is not sufficient; the full tapscript context is required.  Testing this path is
beyond the scope of the basic fixture.

Pass the flag constant directly to the fixture constructor:

```cpp
interpreter_fixture f(chain::flags::nops_rule);
interpreter_fixture f(chain::flags::bip65_rule);
interpreter_fixture f(chain::flags::bip342_rule);
```

Multiple flags can be combined with `|`.

---

## Build System

The six new `.cpp` files are listed in `builds/cmake/CMakeLists.txt` immediately after
`test/machine/interpreter.cpp`, in the `libbitcoin-system-test` target sources:

```cmake
"../../test/machine/interpreter.cpp"
"../../test/machine/interpreter_op_arithmetic.cpp"
"../../test/machine/interpreter_op_bitwise.cpp"
"../../test/machine/interpreter_op_crypto.cpp"
"../../test/machine/interpreter_op_flow.cpp"
"../../test/machine/interpreter_op_push.cpp"
"../../test/machine/interpreter_op_stack.cpp"
"../../test/machine/number.cpp"
```

The fixture header `interpreter_fixture.hpp` is not listed separately; it is `#include`d
by each `.cpp` file.

---

## Relation to Formal Verification

The Hoare-triple structure is a deliberate design choice, not just a naming convention.
Each test case is a machine-checkable instance of the opcode's formal specification:

```
op_swap : {|stack| >= 2} → {return op_success ∧ stack[0] = pre_stack[1] ∧ stack[1] = pre_stack[0]}
op_swap : {|stack| < 2}  → {return op_swap}
```

If the implementation diverges from the specification, at least one test will fail.
If new opcodes are added, the same triple structure provides a template for writing their
tests before writing their implementation (TDD).

The formal specification for each opcode group is recorded in the block comment above
each `BOOST_AUTO_TEST_SUITE` block in the respective `.cpp` file, in the form:

```cpp
// OP_SWAP
// Consensus rule: {P: stack >= 2} swaps top two items.
// Error: op_swap if stack depth < 2.
// Reference: Script specification, no BIP.
```

This comment is the spec; the test cases below it are the proofs.

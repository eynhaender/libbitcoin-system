# Unit Test Concept: machine::interpreter Opcode Coverage

## Background

The initial PR added spec-driven, data-driven tests that feed Bitcoin Core
test vectors through the complete script engine and observe pass/fail.
Maintainer feedback clarified that this is **acceptance testing**, not unit
testing, and that the codebase needs the latter.

> "These types of tests are costly to maintain and don't target the testing
> to a unit of code. They basically test everything at once, and are poorly
> targeted. … I'd suggest that you start with the machine/interpreter."
> — maintainer feedback

## What the Codebase Actually Needs

```
unit testing       — one function, isolated, every code path
component testing  — a set of functions (e.g. a class or behaviour)
functional testing — public interface behaviour (possibly mocked)
acceptance testing — application behaviour (no mocks)
```

The vector tests we wrote sit at the **acceptance** level.
The machine interpreter has 87 individually callable opcode handlers that
can be tested at the **unit** level with precise isolation.

## Code Architecture

`interpreter<Stack>` is a class template parameterised over the primary
stack container type (`linked_stack` or `contiguous_stack`).
It inherits from `program<Stack>`, which holds all execution state.

```
program<Stack>
  primary stack     (Stack container — contiguous_stack or linked_stack)
  alternate stack
  condition stack
  tx reference
  active flags
  └─ interpreter<Stack>
       op_*() methods   (protected — 87 opcode handlers)
       run_op()         (dispatch)
       run()            (execute all ops)
       connect()        (static — full tx validation)
```

Each `op_*` method reads and mutates state exclusively through the
`protected` interface of `program<Stack>` — `stack_size()`, `push_chunk()`,
`push_bool()`, `push_signed64()`, `pop_chunk_()`, `swap_()`, etc.
No global state, no I/O, no virtual dispatch.

This design is ideal for unit testing: the stack is the only input and
output for the majority of opcodes.

## The Fixture Pattern

Because the `op_*` handlers are `protected`, a thin test-only subclass
is all that is needed to expose them. No dependency-injection framework
or mocking library is required.

```cpp
// test/machine/interpreter_fixture.hpp

class interpreter_fixture
  : public interpreter<contiguous_stack>
{
public:
    // Constructs with a minimal dummy transaction and empty input.
    // Opcodes that do not inspect tx/input context (the majority) work
    // correctly without any further setup.
    explicit interpreter_fixture(uint32_t flags = 0) NOEXCEPT;

    // Stack helpers — thin wrappers over program<Stack>'s protected methods.
    void push(data_chunk data) NOEXCEPT { push_chunk(std::move(data)); }
    void push(int64_t value)   NOEXCEPT { push_signed64(value); }
    void push(bool value)      NOEXCEPT { push_bool(value); }
    size_t size()        const NOEXCEPT { return stack_size(); }

    // Expose every opcode handler as public.
    using interpreter::op_swap;
    using interpreter::op_dup;
    using interpreter::op_rot;
    using interpreter::op_add;
    using interpreter::op_if;
    // … all 87 op_* methods
};
```

For opcodes that inspect `tx()` or `input()` (primarily `op_check_sig`,
`op_check_multisig`, `op_check_locktime_verify`,
`op_check_sequence_verify`), a specialised constructor accepts a real
transaction and input iterator.

## Per-Opcode Test Structure

Naming convention follows the existing test style:
`class__method__condition__expected_outcome`

```cpp
BOOST_AUTO_TEST_SUITE(interpreter_op_stack_tests)

// OP_SWAP — consensus rule
// Exchanges the top two stack items: [a, b, ...] → [b, a, ...]
// Fails with op_swap if stack depth < 2.
// Reference: Script specification, no BIP.

BOOST_AUTO_TEST_CASE(interpreter__op_swap__empty_stack__op_swap)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_swap(), error::op_swap);
}

BOOST_AUTO_TEST_CASE(interpreter__op_swap__one_item__op_swap)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    BOOST_REQUIRE_EQUAL(f.op_swap(), error::op_swap);
}

BOOST_AUTO_TEST_CASE(interpreter__op_swap__two_items__swapped)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });   // index 1 after second push
    f.push(data_chunk{ 0x02 });   // index 0 (top)
    BOOST_REQUIRE_EQUAL(f.op_swap(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 2u);
    // top is now 0x01
}

BOOST_AUTO_TEST_CASE(interpreter__op_swap__three_items__only_top_two_swapped)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    f.push(data_chunk{ 0x02 });
    f.push(data_chunk{ 0x03 });   // top before swap
    BOOST_REQUIRE_EQUAL(f.op_swap(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 3u);
    // bottom item unchanged; top two swapped
}

BOOST_AUTO_TEST_SUITE_END()
```

The comment block above each suite states:
1. The **consensus rule** in plain English
2. Every **error condition** and its return code
3. The **BIP reference** if applicable

## Test File Layout

| File | Opcode group | Representative methods |
|---|---|---|
| `interpreter_op_flow.cpp` | Control flow | `op_if`, `op_notif`, `op_else`, `op_endif`, `op_verify`, `op_return`, `op_ver`, `op_verif`, `op_vernotif` |
| `interpreter_op_stack.cpp` | Stack manipulation | `op_dup`, `op_swap`, `op_rot`, `op_over`, `op_pick`, `op_roll`, `op_drop`, `op_depth`, `op_nip`, `op_tuck`, `op_to_alt_stack`, `op_from_alt_stack`, `op_if_dup`, `op_dup2`, `op_dup3`, `op_drop2`, `op_over2`, `op_rot2`, `op_swap2` |
| `interpreter_op_arithmetic.cpp` | Script numbers | `op_add`, `op_sub`, `op_negate`, `op_abs`, `op_not`, `op_nonzero`, `op_add1`, `op_sub1`, `op_min`, `op_max`, `op_within`, `op_bool_and`, `op_bool_or`, `op_less_than`, `op_greater_than`, `op_less_than_or_equal`, `op_greater_than_or_equal`, `op_num_equal`, `op_num_equal_verify`, `op_num_not_equal` |
| `interpreter_op_bitwise.cpp` | Equality / encoding | `op_equal`, `op_equal_verify`, `op_size` |
| `interpreter_op_crypto.cpp` | Hashing + signing | `op_ripemd160`, `op_sha1`, `op_sha256`, `op_hash160`, `op_hash256`, `op_codeseparator`, `op_check_sig`, `op_check_sig_verify`, `op_check_multisig`, `op_check_multisig_verify`, `op_check_sig_add`, `op_check_locktime_verify`, `op_check_sequence_verify` |
| `interpreter_op_push.cpp` | Push opcodes | `op_push_number`, `op_push_size`, `op_push_one_size`, `op_push_two_size`, `op_push_four_size` |

Each file maps to one `BOOST_AUTO_TEST_SUITE`.
A shared `test/machine/interpreter_fixture.hpp` is included by all of them.

## What Every Opcode Test Set Must Cover

For each `op_*` method:

1. **Every error path** — each distinct early-return produces its specific
   `error::op_*` code, typically stack underflow or type mismatch.
2. **Happy path** — correct inputs produce `error::op_success` and the
   stack state after the call matches the consensus specification exactly.
3. **Edge cases** — empty data items, boundary script-number values
   (`INT32_MIN`, `INT32_MAX`), maximum stack depth where relevant.
4. **Flag sensitivity** — for ops that branch on `is_enabled(flag)`, test
   both the flag-active and flag-inactive paths.

## Comparison with the Vector Tests

| Property | Vector tests (acceptance) | Opcode unit tests |
|---|---|---|
| Failure pinpoints location | No — "something failed somewhere" | Yes — exactly which op, which code path |
| Coverage measurable | No | Yes — every branch visible |
| Documents consensus rule | Partially (via JSON comments) | Explicitly, per test case |
| Catches systematic bugs | No | Yes |
| Sensitive to regressions | Yes, coarsely | Yes, precisely |
| Test count | ~1,300 vectors | ~300–500 targeted cases |
| Maintenance cost | High (vector format changes) | Low (tracks one function) |

The vector tests remain useful as an **end-to-end smoke check**.
The unit tests are what is actually missing and what the maintainer requested.

## Relation to Formal Verification

The maintainer noted that this design "lends itself to formal verification".
Each opcode test case is effectively a pre/post-condition pair:

- **Precondition**: stack contents + active flags before the call
- **Postcondition**: return code + stack contents after the call

This maps directly onto a formal model. A future formal verification pass
could use the same fixture infrastructure and test data as the unit tests,
making the two efforts complementary rather than redundant.

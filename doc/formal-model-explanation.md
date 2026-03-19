# Formal Model: machine::interpreter Opcode Tests

## What the Formal Model Means

Each opcode test is structurally a **Hoare triple** — the classical notation from formal methods:

```
{P} C {Q}
```

- **P** (precondition): the exact stack state + active flags *before* calling `op_*`
- **C** (command): the opcode handler call itself
- **Q** (postcondition): the return code + exact stack state *after* the call

Example for the `op_swap` test with two items:

```
P: stack = [0x01, 0x02]
C: op_swap()
Q: return == op_success ∧ stack = [0x02, 0x01]
```

Every error path is also a triple:

```
P: stack = [] (empty)
C: op_swap()
Q: return == error::op_swap ∧ stack unchanged
```

---

## Why This Is Useful

1. **Complete specification** — if you write every error path, happy path, and edge case for all 87 opcodes, the test suite becomes a *machine-readable specification* of Bitcoin Script semantics. No ambiguity, no prose.

2. **Regression safety with precision** — vector tests tell you "something broke somewhere in the engine." A failing Hoare-triple test tells you "the postcondition of `op_add` with inputs `INT32_MAX, 1` is wrong" — exact function, exact branch.

3. **Bridge to formal verification tools** — tools like Frama-C, F*, Coq, Lean, or TLA+ can consume or be seeded by these pre/post-condition pairs. The fixture infrastructure (a thin subclass exposing protected methods + a minimal transaction stub) is exactly what a verification harness needs too. Unit tests and a formal proof would share the same state model.

---

## How to Achieve It

### Step 1 — The fixture subclass

```cpp
// test/machine/interpreter_fixture.hpp

class interpreter_fixture : public interpreter<contiguous_stack>
{
public:
    // Constructs with a minimal dummy transaction and empty input.
    // Opcodes that do not inspect tx/input context (the majority) work
    // correctly without any further setup.
    explicit interpreter_fixture(uint32_t flags = 0) NOEXCEPT;

    // Stack helpers — thin wrappers over program<Stack>'s protected methods.
    void push(data_chunk data) NOEXCEPT { push_chunk(std::move(data)); }
    void push(int64_t v)       NOEXCEPT { push_signed64(v); }
    void push(bool v)          NOEXCEPT { push_bool(v); }
    size_t size() const        NOEXCEPT { return stack_size(); }

    // Expose every opcode handler as public.
    using interpreter::op_swap;
    using interpreter::op_dup;
    using interpreter::op_rot;
    using interpreter::op_add;
    // … all 87 op_* methods
};
```

`interpreter<Stack>` inherits from `program<Stack>`, which holds all execution state. No mocking framework is required — the subclass simply lifts the access restriction.

For opcodes that inspect `tx()` or `input()` (primarily `op_check_sig`, `op_check_multisig`, `op_check_locktime_verify`, `op_check_sequence_verify`), a specialised constructor accepts a real transaction and input iterator.

### Step 2 — One test file per opcode group

| File | Opcode group | Representative methods |
|---|---|---|
| `interpreter_op_flow.cpp` | Control flow | `op_if`, `op_notif`, `op_else`, `op_endif`, `op_verify`, `op_return` |
| `interpreter_op_stack.cpp` | Stack manipulation | `op_dup`, `op_swap`, `op_rot`, `op_over`, `op_pick`, `op_roll`, `op_drop` |
| `interpreter_op_arithmetic.cpp` | Script numbers | `op_add`, `op_sub`, `op_negate`, `op_abs`, `op_min`, `op_max`, `op_within` |
| `interpreter_op_bitwise.cpp` | Equality / encoding | `op_equal`, `op_equal_verify`, `op_size` |
| `interpreter_op_crypto.cpp` | Hashing + signing | `op_hash160`, `op_hash256`, `op_check_sig`, `op_check_multisig` |
| `interpreter_op_push.cpp` | Push opcodes | `op_push_number`, `op_push_size`, `op_push_one_size` |

Each file maps to one `BOOST_AUTO_TEST_SUITE`. A shared `test/machine/interpreter_fixture.hpp` is included by all of them.

### Step 3 — Required coverage checklist per opcode (four cases per op)

For each `op_*` method:

1. **Every error path** — each distinct early-return produces its specific `error::op_*` code, typically stack underflow or type mismatch.
2. **Happy path** — correct inputs produce `error::op_success` and the stack state after the call matches the consensus specification exactly.
3. **Edge cases** — empty data items, boundary script-number values (`INT32_MIN`, `INT32_MAX`), maximum stack depth where relevant.
4. **Flag sensitivity** — for ops that branch on `is_enabled(flag)`, test both the flag-active and flag-inactive paths.

### Step 4 — Comment block documenting the consensus rule

Above each opcode's test group, a comment states:
1. The **consensus rule** in plain English
2. Every **error condition** and its return code
3. The **BIP reference** if applicable

---

## Example: OP_SWAP

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

---

## The Key Insight

The design works cleanly because `op_*` methods have **no side effects outside the program state** — no I/O, no globals, no virtual dispatch. The stack is the sole input and output. That is what makes the Hoare-triple model exact and formal verification tractable: a proof checker only needs to reason about a bounded data structure (the stack) and a pure function.

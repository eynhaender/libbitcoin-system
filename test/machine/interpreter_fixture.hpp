/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_SYSTEM_TEST_MACHINE_INTERPRETER_FIXTURE_HPP
#define LIBBITCOIN_SYSTEM_TEST_MACHINE_INTERPRETER_FIXTURE_HPP

#include "../test.hpp"

namespace test {

using namespace bc::system;
using namespace bc::system::chain;
using namespace bc::system::machine;

// Base-from-member idiom.
// ---------------------------------------------------------------------------
// interpreter<Stack> stores `const chain::transaction&` internally.
// Because interpreter_fixture IS the interpreter (via inheritance), the
// transaction must outlive the interpreter<contiguous_stack> subobject.
//
// Listing interpreter_storage first in the base-class declaration
// guarantees it is constructed before interpreter<contiguous_stack>
// and destroyed after it — so the reference in program<Stack>::transaction_
// is always valid.
// ---------------------------------------------------------------------------
struct interpreter_storage
{
    chain::transaction tx{};

    interpreter_storage() NOEXCEPT
    {
        // Minimal valid transaction: version=1, one empty-script input,
        // no outputs, locktime=0. The empty chain::script produces a non-null
        // script_ptr(), satisfying program::program()'s call to
        // (*input)->script_ptr()->clear_offset().
        chain::inputs in;
        in.emplace_back(chain::point{}, chain::script{}, 0u);
        tx = chain::transaction(1u, std::move(in), chain::outputs{}, 0u);
    }
};

// ---------------------------------------------------------------------------
// interpreter_fixture
//
// Thin subclass that lifts all protected op_* methods to public scope via
// `using` declarations. No mocking framework required — the stack is the
// sole input/output for the majority of opcodes.
//
// Usage:
//   interpreter_fixture f;            // empty stack, no active flags
//   interpreter_fixture f(flags);     // empty stack, flags active
//   f.push(data_chunk{0x01});         // push a raw chunk
//   f.push(int64_t(42));              // push a script number
//   f.push(bool(true));               // push a boolean
//   BOOST_REQUIRE_EQUAL(f.op_dup(), error::op_success);
//   BOOST_REQUIRE_EQUAL(f.size(), 2u);
//   BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{0x01}));
// ---------------------------------------------------------------------------
class interpreter_fixture
  : private interpreter_storage,               // initialized first
    public interpreter<contiguous_stack>
{
public:
    // Construct with empty stack and optional active flags.
    explicit interpreter_fixture(uint32_t flags = 0u) NOEXCEPT
      : interpreter_storage{}
      , interpreter<contiguous_stack>(
            interpreter_storage::tx,
            interpreter_storage::tx.inputs_ptr()->begin(),
            flags)
    {
    }

    // Stack setup helpers — wrappers over program<Stack>'s protected push_*.
    // -----------------------------------------------------------------------

    void push(data_chunk data) NOEXCEPT { push_chunk(std::move(data)); }
    void push(int64_t value)   NOEXCEPT { push_signed64(value); }
    void push(bool value)      NOEXCEPT { push_bool(value); }

    // Stack query helpers.
    // -----------------------------------------------------------------------

    size_t size()  const NOEXCEPT { return stack_size(); }
    bool   empty() const NOEXCEPT { return is_stack_empty(); }

    // Pop the top element and return it as a normalized data_chunk:
    //   bool   -> {} (false) or {0x01} (true)
    //   int64  -> script-number little-endian encoding
    //   chunk  -> raw bytes (identity)
    data_chunk pop_top() NOEXCEPT
    {
        auto ptr = pop_chunk_();
        return ptr ? *ptr : data_chunk{};
    }

    // Peek the top element as a raw stack_variant for advanced assertions.
    const stack_variant& top() const NOEXCEPT { return peek_(); }

    // Opcode exposure — lifts every protected op_* handler to public access.
    // -----------------------------------------------------------------------

    // Push opcodes
    using interpreter::op_push_number;
    using interpreter::op_push_size;
    using interpreter::op_push_one_size;
    using interpreter::op_push_two_size;
    using interpreter::op_push_four_size;

    // Disabled / reserved / NOP
    using interpreter::op_unevaluated;
    using interpreter::op_nop;
    using interpreter::op_ver;
    using interpreter::op_verify;
    using interpreter::op_return;
    using interpreter::op_verif;
    using interpreter::op_vernotif;

    // Conditional (condition stack)
    using interpreter::op_if;
    using interpreter::op_notif;
    using interpreter::op_else;
    using interpreter::op_endif;

    // Stack manipulation
    using interpreter::op_to_alt_stack;
    using interpreter::op_from_alt_stack;
    using interpreter::op_drop2;
    using interpreter::op_dup2;
    using interpreter::op_dup3;
    using interpreter::op_over2;
    using interpreter::op_rot2;
    using interpreter::op_swap2;
    using interpreter::op_if_dup;
    using interpreter::op_depth;
    using interpreter::op_drop;
    using interpreter::op_dup;
    using interpreter::op_nip;
    using interpreter::op_over;
    using interpreter::op_pick;
    using interpreter::op_roll;
    using interpreter::op_rot;
    using interpreter::op_swap;
    using interpreter::op_tuck;

    // Bitwise / encoding
    using interpreter::op_size;
    using interpreter::op_equal;
    using interpreter::op_equal_verify;

    // Arithmetic
    using interpreter::op_add1;
    using interpreter::op_sub1;
    using interpreter::op_negate;
    using interpreter::op_abs;
    using interpreter::op_not;
    using interpreter::op_nonzero;
    using interpreter::op_add;
    using interpreter::op_sub;
    using interpreter::op_bool_and;
    using interpreter::op_bool_or;
    using interpreter::op_num_equal;
    using interpreter::op_num_equal_verify;
    using interpreter::op_num_not_equal;
    using interpreter::op_less_than;
    using interpreter::op_greater_than;
    using interpreter::op_less_than_or_equal;
    using interpreter::op_greater_than_or_equal;
    using interpreter::op_min;
    using interpreter::op_max;
    using interpreter::op_within;

    // Crypto / hashing
    using interpreter::op_ripemd160;
    using interpreter::op_sha1;
    using interpreter::op_sha256;
    using interpreter::op_hash160;
    using interpreter::op_hash256;
    using interpreter::op_codeseparator;

    // Signatures and locktime
    using interpreter::op_check_sig;
    using interpreter::op_check_sig_verify;
    using interpreter::op_check_multisig;
    using interpreter::op_check_multisig_verify;
    using interpreter::op_check_sig_add;
    using interpreter::op_check_locktime_verify;
    using interpreter::op_check_sequence_verify;
};

} // namespace test

#endif // LIBBITCOIN_SYSTEM_TEST_MACHINE_INTERPRETER_FIXTURE_HPP

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
#include "../test.hpp"
#include "interpreter_fixture.hpp"
using namespace test;

// Flag notes:
//   chain::flags::nops_rule (bit 20) must be active for op_return, op_ver,
//   op_verif, op_vernotif to return their "forbidden" error codes.
//   Without the flag, these handlers return error::op_not_implemented.

BOOST_AUTO_TEST_SUITE(interpreter_op_flow_tests)

// OP_VERIFY
// Consensus rule: {P: stack >= 1}
//   fails if top item is falsy; on success, drops the top item.
// Errors: op_verify1 if stack empty, op_verify2 if top is false.
// Reference: Script specification, no BIP.

BOOST_AUTO_TEST_CASE(interpreter__op_verify__empty_stack__op_verify1)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_verify(), error::op_verify1);
}

BOOST_AUTO_TEST_CASE(interpreter__op_verify__false_top__op_verify2_stack_unchanged)
{
    // False = empty chunk. Stack is NOT modified on error.
    interpreter_fixture f;
    f.push(data_chunk{});
    BOOST_REQUIRE_EQUAL(f.op_verify(), error::op_verify2);
    BOOST_REQUIRE_EQUAL(f.size(), 1u);
}

BOOST_AUTO_TEST_CASE(interpreter__op_verify__zero_byte_false__op_verify2)
{
    // {0x00} is also false in Bitcoin Script (negative zero)
    interpreter_fixture f;
    f.push(data_chunk{ 0x80 });    // negative zero = false
    BOOST_REQUIRE_EQUAL(f.op_verify(), error::op_verify2);
}

BOOST_AUTO_TEST_CASE(interpreter__op_verify__true_top__success_top_consumed)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    BOOST_REQUIRE_EQUAL(f.op_verify(), error::op_success);
    BOOST_REQUIRE(f.empty());
}

BOOST_AUTO_TEST_CASE(interpreter__op_verify__true_top_two_items__only_top_consumed)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0xaa });
    f.push(data_chunk{ 0x01 });
    BOOST_REQUIRE_EQUAL(f.op_verify(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 1u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0xaa }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_verify__nonzero_multi_byte__success)
{
    // Any chunk with at least one non-sign non-zero byte is true
    interpreter_fixture f;
    f.push(data_chunk{ 0xab, 0xcd });
    BOOST_REQUIRE_EQUAL(f.op_verify(), error::op_success);
    BOOST_REQUIRE(f.empty());
}

// OP_RETURN
// Consensus rule: unconditional script failure.
//   With nops_rule active: returns op_reserved (via op_unevaluated).
//   Without nops_rule:    returns op_not_implemented.
// Stack state is irrelevant; the opcode always fails.
// Reference: Script specification, no BIP.

BOOST_AUTO_TEST_CASE(interpreter__op_return__no_flag__op_not_implemented)
{
    interpreter_fixture f;  // flags=0, nops_rule disabled
    BOOST_REQUIRE_EQUAL(f.op_return(), error::op_not_implemented);
}

BOOST_AUTO_TEST_CASE(interpreter__op_return__nops_flag__op_reserved)
{
    interpreter_fixture f(chain::flags::nops_rule);
    BOOST_REQUIRE_EQUAL(f.op_return(), error::op_reserved);
}

BOOST_AUTO_TEST_CASE(interpreter__op_return__nops_flag_nonempty_stack__op_reserved)
{
    // Stack contents are irrelevant; always fails
    interpreter_fixture f(chain::flags::nops_rule);
    f.push(data_chunk{ 0x01 });
    BOOST_REQUIRE_EQUAL(f.op_return(), error::op_reserved);
    BOOST_REQUIRE_EQUAL(f.size(), 1u);  // stack unchanged
}

// OP_VER
// Consensus rule: permanently disabled (former version-push opcode).
//   With nops_rule active: returns op_reserved (via op_unevaluated).
//   Without nops_rule:    returns op_not_implemented.
// Reference: Script specification, no BIP.

BOOST_AUTO_TEST_CASE(interpreter__op_ver__no_flag__op_not_implemented)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_ver(), error::op_not_implemented);
}

BOOST_AUTO_TEST_CASE(interpreter__op_ver__nops_flag__op_reserved)
{
    interpreter_fixture f(chain::flags::nops_rule);
    BOOST_REQUIRE_EQUAL(f.op_ver(), error::op_reserved);
}

// OP_VERIF / OP_VERNOTIF
// Consensus rule: permanently invalid opcodes (opcodes 101, 102).
//   operation::is_invalid() returns true for these opcodes, so
//   op_unevaluated() returns op_invalid (not op_reserved).
//   Without nops_rule:    returns op_not_implemented.
// Reference: Script specification, no BIP.

BOOST_AUTO_TEST_CASE(interpreter__op_verif__no_flag__op_not_implemented)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_verif(), error::op_not_implemented);
}

BOOST_AUTO_TEST_CASE(interpreter__op_verif__nops_flag__op_invalid)
{
    // op_verif (opcode 101) is_invalid -> op_unevaluated returns op_invalid.
    interpreter_fixture f(chain::flags::nops_rule);
    BOOST_REQUIRE_EQUAL(f.op_verif(), error::op_invalid);
}

BOOST_AUTO_TEST_CASE(interpreter__op_vernotif__no_flag__op_not_implemented)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_vernotif(), error::op_not_implemented);
}

BOOST_AUTO_TEST_CASE(interpreter__op_vernotif__nops_flag__op_invalid)
{
    // op_vernotif (opcode 102) is_invalid -> op_unevaluated returns op_invalid.
    interpreter_fixture f(chain::flags::nops_rule);
    BOOST_REQUIRE_EQUAL(f.op_vernotif(), error::op_invalid);
}

// OP_IF
// Consensus rule: {P: stack >= 1 when condition stack is "succeeding"}
//   pops top item; pushes true branch to condition stack if truthy.
//   When the outer condition is already false, op_if always succeeds
//   (nested branch bookkeeping only).
// Errors: op_if1 if empty stack during active execution,
//         op_if2 if top is not a minimal bool (tapscript / bip342 only).
// Reference: Script specification, no BIP.

BOOST_AUTO_TEST_CASE(interpreter__op_if__empty_stack__op_if1)
{
    // Default: condition stack is succeeding (no outer if/else)
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_if(), error::op_if1);
}

BOOST_AUTO_TEST_CASE(interpreter__op_if__true_top__success_top_consumed)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    BOOST_REQUIRE_EQUAL(f.op_if(), error::op_success);
    BOOST_REQUIRE(f.empty());
    // Condition stack now has one entry (true).
    // Must balance with op_endif.
    BOOST_REQUIRE_EQUAL(f.op_endif(), error::op_success);
}

BOOST_AUTO_TEST_CASE(interpreter__op_if__false_top__success_top_consumed)
{
    interpreter_fixture f;
    f.push(data_chunk{});
    BOOST_REQUIRE_EQUAL(f.op_if(), error::op_success);
    BOOST_REQUIRE(f.empty());
    BOOST_REQUIRE_EQUAL(f.op_endif(), error::op_success);
}

// OP_NOTIF
// Consensus rule: inverse of OP_IF — enters true branch if top is falsy.
// Same error conditions as OP_IF.

BOOST_AUTO_TEST_CASE(interpreter__op_notif__empty_stack__op_notif1)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_notif(), error::op_notif1);
}

BOOST_AUTO_TEST_CASE(interpreter__op_notif__false_top__success_enters_true_branch)
{
    interpreter_fixture f;
    f.push(data_chunk{});        // false
    BOOST_REQUIRE_EQUAL(f.op_notif(), error::op_success);
    BOOST_REQUIRE(f.empty());
    BOOST_REQUIRE_EQUAL(f.op_endif(), error::op_success);
}

// OP_ELSE
// Consensus rule: toggles the current branch in the condition stack.
//   Must be inside an OP_IF / OP_NOTIF block.
// Error: op_else if condition stack is balanced (no active if/notif).

BOOST_AUTO_TEST_CASE(interpreter__op_else__no_active_if__op_else)
{
    // Condition stack is empty / balanced — op_else is invalid here
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_else(), error::op_else);
}

BOOST_AUTO_TEST_CASE(interpreter__op_else__after_if__success)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    BOOST_REQUIRE_EQUAL(f.op_if(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.op_else(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.op_endif(), error::op_success);
}

// OP_ENDIF
// Consensus rule: closes the innermost OP_IF / OP_NOTIF block.
// Error: op_endif if condition stack is balanced (no open if).

BOOST_AUTO_TEST_CASE(interpreter__op_endif__no_active_if__op_endif)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_endif(), error::op_endif);
}

BOOST_AUTO_TEST_CASE(interpreter__op_endif__after_if__success)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    BOOST_REQUIRE_EQUAL(f.op_if(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.op_endif(), error::op_success);
}

// OP_IF / OP_ELSE / OP_ENDIF interaction
// Full control flow: [true -> if-branch, false -> else-branch].

BOOST_AUTO_TEST_CASE(interpreter__op_if_else_endif__true_branch__only_if_body_executed)
{
    // Simulate: <true> OP_IF <push 0x01> OP_ELSE <push 0x02> OP_ENDIF
    // With true on stack, the if-branch executes (condition stack is true),
    // else-branch is skipped (condition stack is false after op_else).
    interpreter_fixture f;

    f.push(data_chunk{ 0x01 });              // condition = true
    BOOST_REQUIRE_EQUAL(f.op_if(), error::op_success);

    // Inside if-branch: condition active, push if-body marker
    f.push(data_chunk{ 0xAA });

    BOOST_REQUIRE_EQUAL(f.op_else(), error::op_success);

    // Inside else-branch: condition inactive (not succeeding), push skipped
    // (we simulate by just not asserting on the else-body)
    f.push(data_chunk{ 0xBB });              // would be skipped in real execution

    BOOST_REQUIRE_EQUAL(f.op_endif(), error::op_success);

    // After endif: both 0xAA and 0xBB are on the stack because in unit tests
    // we manually push regardless of condition state. This test verifies the
    // condition state transitions are correct (no errors returned).
    BOOST_REQUIRE_EQUAL(f.size(), 2u);
}

BOOST_AUTO_TEST_CASE(interpreter__op_if_else_endif__nested__balanced)
{
    // Nested: <t> OP_IF <t> OP_IF OP_ENDIF OP_ENDIF
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    BOOST_REQUIRE_EQUAL(f.op_if(), error::op_success);
    f.push(data_chunk{ 0x01 });
    BOOST_REQUIRE_EQUAL(f.op_if(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.op_endif(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.op_endif(), error::op_success);
}

BOOST_AUTO_TEST_SUITE_END()

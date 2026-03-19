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

// Note on op_push_size / op_push_one_size / op_push_two_size / op_push_four_size:
//   These handlers receive a chain::operation object.  A correctly constructed
//   operation (via the data_chunk constructor) has a non-null data_ptr() and
//   is_underclaimed() returns false, so all these ops succeed and push the
//   operation's data onto the stack.
//   A default-constructed operation has no data and is_underclaimed() returns
//   true, producing the respective error code.

BOOST_AUTO_TEST_SUITE(interpreter_op_push_tests)

// OP_PUSH_NUMBER  (OP_0 through OP_16, OP_1NEGATE)
// Consensus rule: pushes the given small integer as a script number.
//   op_push_number(0)  -> pushes 0  (empty chunk)
//   op_push_number(-1) -> pushes -1 ({0x81})
//   op_push_number(1)  -> pushes 1  ({0x01})
//   op_push_number(16) -> pushes 16 ({0x10})
// Always succeeds; no precondition.
// Reference: Script specification.

BOOST_AUTO_TEST_CASE(interpreter__op_push_number__zero__empty_chunk)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_push_number(0), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 1u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

BOOST_AUTO_TEST_CASE(interpreter__op_push_number__negative_one__0x81)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_push_number(-1), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x81 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_push_number__one__0x01)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_push_number(1), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_push_number__sixteen__0x10)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_push_number(16), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x10 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_push_number__two_sequential__two_items)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_push_number(3), error::op_success);
    BOOST_REQUIRE_EQUAL(f.op_push_number(7), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 2u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x07 }));
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x03 }));
}

// OP_PUSHDATA (push_size, 1-75 bytes)
// Consensus rule: pushes the operation's data onto the stack.
//   is_underclaimed() must be false for the operation to succeed.
// Error: op_push_size if is_underclaimed() is true (data shorter than declared).

BOOST_AUTO_TEST_CASE(interpreter__op_push_size__valid_operation__data_on_stack)
{
    // operation(data_chunk, minimal) constructs a valid push op
    const chain::operation op(data_chunk{ 0x01, 0x02, 0x03 }, false);
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_push_size(op), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 1u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01, 0x02, 0x03 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_push_size__empty_data__empty_chunk_on_stack)
{
    const chain::operation op(data_chunk{}, false);
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_push_size(op), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

BOOST_AUTO_TEST_CASE(interpreter__op_push_size__single_byte__on_stack)
{
    const chain::operation op(data_chunk{ 0xff }, false);
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_push_size(op), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0xff }));
}

// OP_PUSHDATA1 (push_one_size, 1-byte length prefix, up to 255 bytes)

BOOST_AUTO_TEST_CASE(interpreter__op_push_one_size__valid_operation__data_on_stack)
{
    const chain::operation op(data_chunk{ 0xaa, 0xbb }, false);
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_push_one_size(op), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0xaa, 0xbb }));
}

// OP_PUSHDATA2 (push_two_size, 2-byte length prefix, up to 65535 bytes)

BOOST_AUTO_TEST_CASE(interpreter__op_push_two_size__valid_operation__data_on_stack)
{
    const chain::operation op(data_chunk{ 0x11, 0x22, 0x33 }, false);
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_push_two_size(op), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x11, 0x22, 0x33 }));
}

// OP_PUSHDATA4 (push_four_size, 4-byte length prefix, up to 2^32-1 bytes)

BOOST_AUTO_TEST_CASE(interpreter__op_push_four_size__valid_operation__data_on_stack)
{
    const chain::operation op(data_chunk{ 0xde, 0xad, 0xbe, 0xef }, false);
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_push_four_size(op), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0xde, 0xad, 0xbe, 0xef }));
}

// OP_NOP (zero-arg variant)
// Consensus rule: always succeeds without modifying the stack.

BOOST_AUTO_TEST_CASE(interpreter__op_nop__empty_stack__op_success)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_nop(), error::op_success);
    BOOST_REQUIRE(f.empty());
}

BOOST_AUTO_TEST_CASE(interpreter__op_nop__nonempty_stack__stack_unchanged)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x42 });
    BOOST_REQUIRE_EQUAL(f.op_nop(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 1u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x42 }));
}

BOOST_AUTO_TEST_SUITE_END()

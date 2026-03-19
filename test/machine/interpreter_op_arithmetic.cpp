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

// Script number encoding reference (little-endian, sign bit in last byte):
//   0        -> {}          (empty)
//   1        -> {0x01}
//   2        -> {0x02}
//   -1       -> {0x81}      (0x01 with sign bit)
//   -2       -> {0x82}
//   127      -> {0x7f}
//   -127     -> {0xff}
//   128      -> {0x80, 0x00} (extra sign byte needed)
//   -128     -> {0x80, 0x80}

BOOST_AUTO_TEST_SUITE(interpreter_op_arithmetic_tests)

// OP_1ADD
// Consensus rule: {P: stack >= 1, top is valid 4-byte script number}
//   increments the top item by 1.
// Error: op_add1 on empty stack or invalid script number.

BOOST_AUTO_TEST_CASE(interpreter__op_add1__empty_stack__op_add1)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_add1(), error::op_add1);
}

BOOST_AUTO_TEST_CASE(interpreter__op_add1__oversized_number__op_add1)
{
    // 5-byte script number exceeds the 4-byte domain for arithmetic ops
    interpreter_fixture f;
    f.push(data_chunk{ 0x01, 0x00, 0x00, 0x00, 0x00 });
    BOOST_REQUIRE_EQUAL(f.op_add1(), error::op_add1);
}

BOOST_AUTO_TEST_CASE(interpreter__op_add1__zero__one)
{
    interpreter_fixture f;
    f.push(int64_t(0));
    BOOST_REQUIRE_EQUAL(f.op_add1(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 1u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_add1__negative_one__zero)
{
    // -1 + 1 = 0; script number 0 encodes as empty chunk
    interpreter_fixture f;
    f.push(int64_t(-1));
    BOOST_REQUIRE_EQUAL(f.op_add1(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

BOOST_AUTO_TEST_CASE(interpreter__op_add1__positive_value__incremented)
{
    interpreter_fixture f;
    f.push(int64_t(41));
    BOOST_REQUIRE_EQUAL(f.op_add1(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x2a }));  // 42
}

BOOST_AUTO_TEST_CASE(interpreter__op_add1__int32_max__int32_max_plus_one)
{
    // max_int32 = 2^31-1; result stored as int64 -> 5-byte script number
    interpreter_fixture f;
    f.push(int64_t(max_int32));
    BOOST_REQUIRE_EQUAL(f.op_add1(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 1u);
    // Just verify it pushed something (no overflow, domain promoted to int64)
    BOOST_REQUIRE(!f.empty());
}

// OP_1SUB
// Consensus rule: {P: stack >= 1, top is valid 4-byte script number}
//   decrements the top item by 1.
// Error: op_sub1 on empty stack or invalid script number.

BOOST_AUTO_TEST_CASE(interpreter__op_sub1__empty_stack__op_sub1)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_sub1(), error::op_sub1);
}

BOOST_AUTO_TEST_CASE(interpreter__op_sub1__zero__negative_one)
{
    interpreter_fixture f;
    f.push(int64_t(0));
    BOOST_REQUIRE_EQUAL(f.op_sub1(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x81 }));  // -1
}

BOOST_AUTO_TEST_CASE(interpreter__op_sub1__one__zero)
{
    interpreter_fixture f;
    f.push(int64_t(1));
    BOOST_REQUIRE_EQUAL(f.op_sub1(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});  // 0
}

BOOST_AUTO_TEST_CASE(interpreter__op_sub1__positive_value__decremented)
{
    interpreter_fixture f;
    f.push(int64_t(43));
    BOOST_REQUIRE_EQUAL(f.op_sub1(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x2a }));  // 42
}

// OP_NEGATE
// Consensus rule: {P: stack >= 1, top is valid 4-byte script number}
//   replaces the top item with its arithmetic negation.
// Error: op_negate on empty stack or invalid script number.

BOOST_AUTO_TEST_CASE(interpreter__op_negate__empty_stack__op_negate)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_negate(), error::op_negate);
}

BOOST_AUTO_TEST_CASE(interpreter__op_negate__zero__zero)
{
    interpreter_fixture f;
    f.push(int64_t(0));
    BOOST_REQUIRE_EQUAL(f.op_negate(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

BOOST_AUTO_TEST_CASE(interpreter__op_negate__positive__negative)
{
    // negate(1) = -1; -1 encodes as {0x81}
    interpreter_fixture f;
    f.push(int64_t(1));
    BOOST_REQUIRE_EQUAL(f.op_negate(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x81 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_negate__negative__positive)
{
    // negate(-1) = 1; 1 encodes as {0x01}
    interpreter_fixture f;
    f.push(int64_t(-1));
    BOOST_REQUIRE_EQUAL(f.op_negate(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

// OP_ABS
// Consensus rule: {P: stack >= 1, top is valid 4-byte script number}
//   replaces the top item with its absolute value.
// Error: op_abs on empty stack or invalid script number.

BOOST_AUTO_TEST_CASE(interpreter__op_abs__empty_stack__op_abs)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_abs(), error::op_abs);
}

BOOST_AUTO_TEST_CASE(interpreter__op_abs__zero__zero)
{
    interpreter_fixture f;
    f.push(int64_t(0));
    BOOST_REQUIRE_EQUAL(f.op_abs(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

BOOST_AUTO_TEST_CASE(interpreter__op_abs__positive__unchanged)
{
    interpreter_fixture f;
    f.push(int64_t(5));
    BOOST_REQUIRE_EQUAL(f.op_abs(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x05 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_abs__negative__positive)
{
    interpreter_fixture f;
    f.push(int64_t(-5));
    BOOST_REQUIRE_EQUAL(f.op_abs(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x05 }));
}

// OP_NOT
// Consensus rule: {P: stack >= 1, top is valid 4-byte script number}
//   pushes 1 if top is 0, else pushes 0.
// Error: op_not on empty stack or invalid script number.
// Note: uses numeric zero test, not byte-level bool.

BOOST_AUTO_TEST_CASE(interpreter__op_not__empty_stack__op_not)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_not(), error::op_not);
}

BOOST_AUTO_TEST_CASE(interpreter__op_not__zero__one)
{
    interpreter_fixture f;
    f.push(int64_t(0));
    BOOST_REQUIRE_EQUAL(f.op_not(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_not__nonzero__zero)
{
    interpreter_fixture f;
    f.push(int64_t(42));
    BOOST_REQUIRE_EQUAL(f.op_not(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

BOOST_AUTO_TEST_CASE(interpreter__op_not__negative__zero)
{
    // Negative numbers are also non-zero
    interpreter_fixture f;
    f.push(int64_t(-1));
    BOOST_REQUIRE_EQUAL(f.op_not(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

// OP_0NOTEQUAL
// Consensus rule: {P: stack >= 1, top is valid 4-byte script number}
//   pushes 1 if top != 0, else pushes 0.
// Error: op_nonzero on empty stack or invalid script number.

BOOST_AUTO_TEST_CASE(interpreter__op_nonzero__empty_stack__op_nonzero)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_nonzero(), error::op_nonzero);
}

BOOST_AUTO_TEST_CASE(interpreter__op_nonzero__zero__zero)
{
    interpreter_fixture f;
    f.push(int64_t(0));
    BOOST_REQUIRE_EQUAL(f.op_nonzero(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

BOOST_AUTO_TEST_CASE(interpreter__op_nonzero__nonzero__one)
{
    interpreter_fixture f;
    f.push(int64_t(7));
    BOOST_REQUIRE_EQUAL(f.op_nonzero(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_nonzero__negative__one)
{
    interpreter_fixture f;
    f.push(int64_t(-3));
    BOOST_REQUIRE_EQUAL(f.op_nonzero(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

// OP_ADD
// Consensus rule: {P: stack >= 2, both items valid 4-byte script numbers}
//   pops two, pushes their sum.
//   Stack order (bottom->top): left, right.  result = left + right.
// Error: op_add on underflow or invalid script number.

BOOST_AUTO_TEST_CASE(interpreter__op_add__empty_stack__op_add)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_add(), error::op_add);
}

BOOST_AUTO_TEST_CASE(interpreter__op_add__one_item__op_add)
{
    interpreter_fixture f;
    f.push(int64_t(1));
    BOOST_REQUIRE_EQUAL(f.op_add(), error::op_add);
    BOOST_REQUIRE_EQUAL(f.size(), 1u);
}

BOOST_AUTO_TEST_CASE(interpreter__op_add__zero_zero__zero)
{
    interpreter_fixture f;
    f.push(int64_t(0));
    f.push(int64_t(0));
    BOOST_REQUIRE_EQUAL(f.op_add(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 1u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

BOOST_AUTO_TEST_CASE(interpreter__op_add__one_two__three)
{
    interpreter_fixture f;
    f.push(int64_t(1));   // left (bottom)
    f.push(int64_t(2));   // right (top)
    BOOST_REQUIRE_EQUAL(f.op_add(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x03 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_add__negative_and_positive__correct_sum)
{
    // -3 + 1 = -2; -2 encodes as {0x82}
    interpreter_fixture f;
    f.push(int64_t(-3));
    f.push(int64_t(1));
    BOOST_REQUIRE_EQUAL(f.op_add(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x82 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_add__negatives_cancel__zero)
{
    interpreter_fixture f;
    f.push(int64_t(-5));
    f.push(int64_t(5));
    BOOST_REQUIRE_EQUAL(f.op_add(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

// OP_SUB
// Consensus rule: {P: stack >= 2, both items valid 4-byte script numbers}
//   pops two, pushes left - right.
//   Stack order (bottom->top): left, right.  result = left - right.
// Error: op_sub on underflow or invalid script number.

BOOST_AUTO_TEST_CASE(interpreter__op_sub__empty_stack__op_sub)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_sub(), error::op_sub);
}

BOOST_AUTO_TEST_CASE(interpreter__op_sub__five_minus_three__two)
{
    interpreter_fixture f;
    f.push(int64_t(5));   // left (bottom)
    f.push(int64_t(3));   // right (top)
    BOOST_REQUIRE_EQUAL(f.op_sub(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x02 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_sub__three_minus_five__negative_two)
{
    // 3 - 5 = -2; -2 encodes as {0x82}
    interpreter_fixture f;
    f.push(int64_t(3));
    f.push(int64_t(5));
    BOOST_REQUIRE_EQUAL(f.op_sub(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x82 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_sub__equal_values__zero)
{
    interpreter_fixture f;
    f.push(int64_t(7));
    f.push(int64_t(7));
    BOOST_REQUIRE_EQUAL(f.op_sub(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

// OP_BOOLAND
// Consensus rule: {P: stack >= 2, both valid script numbers}
//   pushes 1 if both are non-zero, else pushes 0.
// Error: op_bool_and on underflow.

BOOST_AUTO_TEST_CASE(interpreter__op_bool_and__empty_stack__op_bool_and)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_bool_and(), error::op_bool_and);
}

BOOST_AUTO_TEST_CASE(interpreter__op_bool_and__both_nonzero__one)
{
    interpreter_fixture f;
    f.push(int64_t(3));
    f.push(int64_t(7));
    BOOST_REQUIRE_EQUAL(f.op_bool_and(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_bool_and__one_zero__zero)
{
    interpreter_fixture f;
    f.push(int64_t(0));
    f.push(int64_t(7));
    BOOST_REQUIRE_EQUAL(f.op_bool_and(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

BOOST_AUTO_TEST_CASE(interpreter__op_bool_and__both_zero__zero)
{
    interpreter_fixture f;
    f.push(int64_t(0));
    f.push(int64_t(0));
    BOOST_REQUIRE_EQUAL(f.op_bool_and(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

// OP_BOOLOR
// Consensus rule: {P: stack >= 2, both valid script numbers}
//   pushes 1 if either is non-zero, else pushes 0.
// Error: op_bool_or on underflow.

BOOST_AUTO_TEST_CASE(interpreter__op_bool_or__empty_stack__op_bool_or)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_bool_or(), error::op_bool_or);
}

BOOST_AUTO_TEST_CASE(interpreter__op_bool_or__both_nonzero__one)
{
    interpreter_fixture f;
    f.push(int64_t(3));
    f.push(int64_t(7));
    BOOST_REQUIRE_EQUAL(f.op_bool_or(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_bool_or__one_zero_one_nonzero__one)
{
    interpreter_fixture f;
    f.push(int64_t(0));
    f.push(int64_t(7));
    BOOST_REQUIRE_EQUAL(f.op_bool_or(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_bool_or__both_zero__zero)
{
    interpreter_fixture f;
    f.push(int64_t(0));
    f.push(int64_t(0));
    BOOST_REQUIRE_EQUAL(f.op_bool_or(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

// OP_NUMEQUAL
// Consensus rule: {P: stack >= 2, both valid script numbers}
//   pushes 1 if numerically equal, else 0.
// Error: op_num_equal on underflow.

BOOST_AUTO_TEST_CASE(interpreter__op_num_equal__empty_stack__op_num_equal)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_num_equal(), error::op_num_equal);
}

BOOST_AUTO_TEST_CASE(interpreter__op_num_equal__same_values__one)
{
    interpreter_fixture f;
    f.push(int64_t(42));
    f.push(int64_t(42));
    BOOST_REQUIRE_EQUAL(f.op_num_equal(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_num_equal__different_values__zero)
{
    interpreter_fixture f;
    f.push(int64_t(1));
    f.push(int64_t(2));
    BOOST_REQUIRE_EQUAL(f.op_num_equal(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

// OP_NUMEQUALVERIFY
// Consensus rule: same as OP_NUMEQUAL but fails if not equal.
// Errors: op_num_equal_verify1 on underflow, op_num_equal_verify2 if unequal.

BOOST_AUTO_TEST_CASE(interpreter__op_num_equal_verify__empty_stack__op_num_equal_verify1)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_num_equal_verify(), error::op_num_equal_verify1);
}

BOOST_AUTO_TEST_CASE(interpreter__op_num_equal_verify__same_values__success_no_item_left)
{
    interpreter_fixture f;
    f.push(int64_t(7));
    f.push(int64_t(7));
    BOOST_REQUIRE_EQUAL(f.op_num_equal_verify(), error::op_success);
    BOOST_REQUIRE(f.empty());
}

BOOST_AUTO_TEST_CASE(interpreter__op_num_equal_verify__different_values__op_num_equal_verify2)
{
    interpreter_fixture f;
    f.push(int64_t(1));
    f.push(int64_t(2));
    BOOST_REQUIRE_EQUAL(f.op_num_equal_verify(), error::op_num_equal_verify2);
}

// OP_NUMNOTEQUAL
// Consensus rule: {P: stack >= 2, both valid script numbers}
//   pushes 1 if numerically unequal, else 0.
// Error: op_num_not_equal on underflow.

BOOST_AUTO_TEST_CASE(interpreter__op_num_not_equal__different_values__one)
{
    interpreter_fixture f;
    f.push(int64_t(1));
    f.push(int64_t(2));
    BOOST_REQUIRE_EQUAL(f.op_num_not_equal(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_num_not_equal__same_values__zero)
{
    interpreter_fixture f;
    f.push(int64_t(5));
    f.push(int64_t(5));
    BOOST_REQUIRE_EQUAL(f.op_num_not_equal(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

// OP_LESSTHAN
// Consensus rule: {P: stack >= 2, both valid script numbers}
//   pushes 1 if left < right, else 0.
//   Stack order (bottom->top): left, right.
// Error: op_less_than on underflow.

BOOST_AUTO_TEST_CASE(interpreter__op_less_than__empty_stack__op_less_than)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_less_than(), error::op_less_than);
}

BOOST_AUTO_TEST_CASE(interpreter__op_less_than__left_less__one)
{
    interpreter_fixture f;
    f.push(int64_t(1));  // left
    f.push(int64_t(2));  // right
    BOOST_REQUIRE_EQUAL(f.op_less_than(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_less_than__equal__zero)
{
    interpreter_fixture f;
    f.push(int64_t(3));
    f.push(int64_t(3));
    BOOST_REQUIRE_EQUAL(f.op_less_than(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

BOOST_AUTO_TEST_CASE(interpreter__op_less_than__left_greater__zero)
{
    interpreter_fixture f;
    f.push(int64_t(5));
    f.push(int64_t(3));
    BOOST_REQUIRE_EQUAL(f.op_less_than(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

// OP_GREATERTHAN
// Consensus rule: {P: stack >= 2, both valid script numbers}
//   pushes 1 if left > right, else 0.

BOOST_AUTO_TEST_CASE(interpreter__op_greater_than__left_greater__one)
{
    interpreter_fixture f;
    f.push(int64_t(5));
    f.push(int64_t(3));
    BOOST_REQUIRE_EQUAL(f.op_greater_than(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_greater_than__equal__zero)
{
    interpreter_fixture f;
    f.push(int64_t(3));
    f.push(int64_t(3));
    BOOST_REQUIRE_EQUAL(f.op_greater_than(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

// OP_LESSTHANOREQUAL / OP_GREATERTHANOREQUAL
// Consensus rule: {P: stack >= 2, both valid script numbers}

BOOST_AUTO_TEST_CASE(interpreter__op_less_than_or_equal__equal__one)
{
    interpreter_fixture f;
    f.push(int64_t(3));
    f.push(int64_t(3));
    BOOST_REQUIRE_EQUAL(f.op_less_than_or_equal(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_less_than_or_equal__left_greater__zero)
{
    interpreter_fixture f;
    f.push(int64_t(5));
    f.push(int64_t(3));
    BOOST_REQUIRE_EQUAL(f.op_less_than_or_equal(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

BOOST_AUTO_TEST_CASE(interpreter__op_greater_than_or_equal__equal__one)
{
    interpreter_fixture f;
    f.push(int64_t(3));
    f.push(int64_t(3));
    BOOST_REQUIRE_EQUAL(f.op_greater_than_or_equal(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_greater_than_or_equal__left_less__zero)
{
    interpreter_fixture f;
    f.push(int64_t(1));
    f.push(int64_t(3));
    BOOST_REQUIRE_EQUAL(f.op_greater_than_or_equal(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

// OP_MIN
// Consensus rule: {P: stack >= 2, both valid script numbers}
//   pops two, pushes the lesser.
//   Stack order (bottom->top): left, right.  result = min(left, right).
// Error: op_min on underflow.

BOOST_AUTO_TEST_CASE(interpreter__op_min__empty_stack__op_min)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_min(), error::op_min);
}

BOOST_AUTO_TEST_CASE(interpreter__op_min__one_less_than_two__one)
{
    interpreter_fixture f;
    f.push(int64_t(1));
    f.push(int64_t(2));
    BOOST_REQUIRE_EQUAL(f.op_min(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_min__two_less_than_five__two)
{
    interpreter_fixture f;
    f.push(int64_t(5));
    f.push(int64_t(2));
    BOOST_REQUIRE_EQUAL(f.op_min(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x02 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_min__negative_and_positive__negative)
{
    // min(-1, 1) = -1; -1 encodes as {0x81}
    interpreter_fixture f;
    f.push(int64_t(-1));
    f.push(int64_t(1));
    BOOST_REQUIRE_EQUAL(f.op_min(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x81 }));
}

// OP_MAX
// Consensus rule: {P: stack >= 2, both valid script numbers}
//   pops two, pushes the greater.
// Error: op_max on underflow.

BOOST_AUTO_TEST_CASE(interpreter__op_max__empty_stack__op_max)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_max(), error::op_max);
}

BOOST_AUTO_TEST_CASE(interpreter__op_max__one_two__two)
{
    interpreter_fixture f;
    f.push(int64_t(1));
    f.push(int64_t(2));
    BOOST_REQUIRE_EQUAL(f.op_max(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x02 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_max__negative_and_positive__positive)
{
    // max(-1, 1) = 1; 1 encodes as {0x01}
    interpreter_fixture f;
    f.push(int64_t(-1));
    f.push(int64_t(1));
    BOOST_REQUIRE_EQUAL(f.op_max(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

// OP_WITHIN
// Consensus rule: {P: stack >= 3, all items valid 4-byte script numbers}
//   pushes 1 if value is in [lower, upper), else 0.
//   Stack order (bottom->top): value, lower, upper.
// Error: op_within on underflow.
// Reference: value >= lower AND value < upper.

BOOST_AUTO_TEST_CASE(interpreter__op_within__empty_stack__op_within)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_within(), error::op_within);
}

BOOST_AUTO_TEST_CASE(interpreter__op_within__two_items__op_within)
{
    interpreter_fixture f;
    f.push(int64_t(0));
    f.push(int64_t(5));
    BOOST_REQUIRE_EQUAL(f.op_within(), error::op_within);
}

BOOST_AUTO_TEST_CASE(interpreter__op_within__value_in_range__one)
{
    // Stack: value=3 (bottom), lower=0, upper=5 (top)
    interpreter_fixture f;
    f.push(int64_t(3));   // value
    f.push(int64_t(0));   // lower
    f.push(int64_t(5));   // upper (top)
    BOOST_REQUIRE_EQUAL(f.op_within(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_within__value_at_lower_bound__one)
{
    // lower bound is inclusive
    interpreter_fixture f;
    f.push(int64_t(0));   // value == lower
    f.push(int64_t(0));   // lower
    f.push(int64_t(5));   // upper
    BOOST_REQUIRE_EQUAL(f.op_within(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_within__value_at_upper_bound__zero)
{
    // upper bound is exclusive
    interpreter_fixture f;
    f.push(int64_t(5));   // value == upper
    f.push(int64_t(0));   // lower
    f.push(int64_t(5));   // upper
    BOOST_REQUIRE_EQUAL(f.op_within(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

BOOST_AUTO_TEST_CASE(interpreter__op_within__value_below_lower__zero)
{
    interpreter_fixture f;
    f.push(int64_t(-1));  // value < lower
    f.push(int64_t(0));
    f.push(int64_t(5));
    BOOST_REQUIRE_EQUAL(f.op_within(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

BOOST_AUTO_TEST_SUITE_END()

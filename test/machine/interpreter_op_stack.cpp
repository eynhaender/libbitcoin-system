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

BOOST_AUTO_TEST_SUITE(interpreter_op_stack_tests)

// OP_SWAP
// Consensus rule: exchanges the top two stack items.
//   [... a b] -> [... b a]  (b is on top before the call)
// Error: op_swap if stack depth < 2.
// Reference: Script specification, no BIP.

BOOST_AUTO_TEST_CASE(interpreter__op_swap__empty_stack__op_swap)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_swap(), error::op_swap);
    BOOST_REQUIRE(f.empty());
}

BOOST_AUTO_TEST_CASE(interpreter__op_swap__one_item__op_swap)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    BOOST_REQUIRE_EQUAL(f.op_swap(), error::op_swap);
    BOOST_REQUIRE_EQUAL(f.size(), 1u);
}

BOOST_AUTO_TEST_CASE(interpreter__op_swap__two_items__swapped)
{
    // push: 0x01 (bottom), 0x02 (top)
    // after swap: 0x01 (top), 0x02 (bottom)
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    f.push(data_chunk{ 0x02 });
    BOOST_REQUIRE_EQUAL(f.op_swap(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 2u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x02 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_swap__three_items__only_top_two_swapped)
{
    // push: 0xaa (bottom), 0x01, 0x02 (top)
    // after swap: 0x01 (top), 0x02, 0xaa (bottom)
    interpreter_fixture f;
    f.push(data_chunk{ 0xaa });
    f.push(data_chunk{ 0x01 });
    f.push(data_chunk{ 0x02 });
    BOOST_REQUIRE_EQUAL(f.op_swap(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 3u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x02 }));
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0xaa }));
}

// OP_DUP
// Consensus rule: duplicates the top stack item.
//   [... a] -> [... a a]
// Error: op_dup if stack empty.

BOOST_AUTO_TEST_CASE(interpreter__op_dup__empty_stack__op_dup)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_dup(), error::op_dup);
}

BOOST_AUTO_TEST_CASE(interpreter__op_dup__one_item__duplicated)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x42 });
    BOOST_REQUIRE_EQUAL(f.op_dup(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 2u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x42 }));
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x42 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_dup__empty_chunk__empty_chunk_duplicated)
{
    // empty data_chunk is a valid stack element (represents false/0)
    interpreter_fixture f;
    f.push(data_chunk{});
    BOOST_REQUIRE_EQUAL(f.op_dup(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 2u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

// OP_DROP
// Consensus rule: discards the top stack item.
//   [... a] -> [...]
// Error: op_drop if stack empty.

BOOST_AUTO_TEST_CASE(interpreter__op_drop__empty_stack__op_drop)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_drop(), error::op_drop);
}

BOOST_AUTO_TEST_CASE(interpreter__op_drop__one_item__stack_empty)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    BOOST_REQUIRE_EQUAL(f.op_drop(), error::op_success);
    BOOST_REQUIRE(f.empty());
}

BOOST_AUTO_TEST_CASE(interpreter__op_drop__two_items__one_remains)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    f.push(data_chunk{ 0x02 });
    BOOST_REQUIRE_EQUAL(f.op_drop(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 1u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

// OP_OVER
// Consensus rule: copies the second-from-top item to the top.
//   [... a b] -> [... a b a]  (b is on top, a is second)
// Error: op_over if stack depth < 2.

BOOST_AUTO_TEST_CASE(interpreter__op_over__empty_stack__op_over)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_over(), error::op_over);
}

BOOST_AUTO_TEST_CASE(interpreter__op_over__one_item__op_over)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    BOOST_REQUIRE_EQUAL(f.op_over(), error::op_over);
    BOOST_REQUIRE_EQUAL(f.size(), 1u);
}

BOOST_AUTO_TEST_CASE(interpreter__op_over__two_items__second_copied_to_top)
{
    // push: A (bottom), B (top)
    // after over: A (top), B, A (bottom)
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    f.push(data_chunk{ 0x02 });
    BOOST_REQUIRE_EQUAL(f.op_over(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 3u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x02 }));
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

// OP_ROT
// Consensus rule: moves the third-from-top item to the top.
//   [... a b c] -> [... b c a]  (c is on top)
// Error: op_rot if stack depth < 3.

BOOST_AUTO_TEST_CASE(interpreter__op_rot__empty_stack__op_rot)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_rot(), error::op_rot);
}

BOOST_AUTO_TEST_CASE(interpreter__op_rot__two_items__op_rot)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    f.push(data_chunk{ 0x02 });
    BOOST_REQUIRE_EQUAL(f.op_rot(), error::op_rot);
    BOOST_REQUIRE_EQUAL(f.size(), 2u);
}

BOOST_AUTO_TEST_CASE(interpreter__op_rot__three_items__rotated)
{
    // push: A (bottom), B, C (top)
    // after rot: B (bottom), C, A (top)
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    f.push(data_chunk{ 0x02 });
    f.push(data_chunk{ 0x03 });
    BOOST_REQUIRE_EQUAL(f.op_rot(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 3u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x03 }));
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x02 }));
}

// OP_NIP
// Consensus rule: removes the second-from-top item.
//   [... a b] -> [... b]  (b is on top)
// Error: op_nip if stack depth < 2.

BOOST_AUTO_TEST_CASE(interpreter__op_nip__empty_stack__op_nip)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_nip(), error::op_nip);
}

BOOST_AUTO_TEST_CASE(interpreter__op_nip__one_item__op_nip)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    BOOST_REQUIRE_EQUAL(f.op_nip(), error::op_nip);
    BOOST_REQUIRE_EQUAL(f.size(), 1u);
}

BOOST_AUTO_TEST_CASE(interpreter__op_nip__two_items__second_removed)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    f.push(data_chunk{ 0x02 });
    BOOST_REQUIRE_EQUAL(f.op_nip(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 1u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x02 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_nip__three_items__second_removed_others_intact)
{
    // push: A (bottom), B, C (top)
    // after nip: A (bottom), C (top)
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    f.push(data_chunk{ 0x02 });
    f.push(data_chunk{ 0x03 });
    BOOST_REQUIRE_EQUAL(f.op_nip(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 2u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x03 }));
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

// OP_TUCK
// Consensus rule: copies the top item below the second-from-top item.
//   [... a b] -> [... b a b]  (b is on top, a is second)
// Error: op_tuck if stack depth < 2.

BOOST_AUTO_TEST_CASE(interpreter__op_tuck__empty_stack__op_tuck)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_tuck(), error::op_tuck);
}

BOOST_AUTO_TEST_CASE(interpreter__op_tuck__one_item__op_tuck)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    BOOST_REQUIRE_EQUAL(f.op_tuck(), error::op_tuck);
}

BOOST_AUTO_TEST_CASE(interpreter__op_tuck__two_items__top_tucked_below_second)
{
    // push: A (bottom), B (top)
    // after tuck: B (bottom), A, B (top)
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    f.push(data_chunk{ 0x02 });
    BOOST_REQUIRE_EQUAL(f.op_tuck(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 3u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x02 }));
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x02 }));
}

// OP_DEPTH
// Consensus rule: pushes the current stack depth as a script number.
//   [...n items...] -> [n, ...n items...]
// Always succeeds; no precondition on depth.

BOOST_AUTO_TEST_CASE(interpreter__op_depth__empty_stack__zero_pushed)
{
    // depth before call = 0; script number 0 encodes as empty chunk
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_depth(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 1u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

BOOST_AUTO_TEST_CASE(interpreter__op_depth__one_item__one_pushed)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0xaa });
    BOOST_REQUIRE_EQUAL(f.op_depth(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 2u);
    // script number 1 = {0x01}
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0xaa }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_depth__two_items__two_pushed)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    f.push(data_chunk{ 0x02 });
    BOOST_REQUIRE_EQUAL(f.op_depth(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 3u);
    // script number 2 = {0x02}
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x02 }));
}

// OP_IFDUP
// Consensus rule: duplicates the top item only if it is truthy.
//   [... 0] -> [... 0]          (false: unchanged)
//   [... a] -> [... a a]        (a truthy: duplicated)
// Error: op_if_dup if stack empty.

BOOST_AUTO_TEST_CASE(interpreter__op_if_dup__empty_stack__op_if_dup)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_if_dup(), error::op_if_dup);
}

BOOST_AUTO_TEST_CASE(interpreter__op_if_dup__false_top__not_duplicated)
{
    interpreter_fixture f;
    f.push(data_chunk{});            // empty chunk = false
    BOOST_REQUIRE_EQUAL(f.op_if_dup(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 1u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

BOOST_AUTO_TEST_CASE(interpreter__op_if_dup__true_top__duplicated)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });      // non-empty = true
    BOOST_REQUIRE_EQUAL(f.op_if_dup(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 2u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_if_dup__negative_true_top__duplicated)
{
    // Negative values are truthy in Bitcoin Script (sign byte present != 0)
    interpreter_fixture f;
    f.push(data_chunk{ 0x81 });      // script number -1 = true
    BOOST_REQUIRE_EQUAL(f.op_if_dup(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 2u);
}

// OP_TOALTSTACK / OP_FROMALTSTACK
// Consensus rule:
//   op_to_alt_stack:   pops top of primary stack, pushes to alt stack
//   op_from_alt_stack: pops top of alt stack, pushes to primary stack
// Errors: op_to_alt_stack / op_from_alt_stack on underflow.

BOOST_AUTO_TEST_CASE(interpreter__op_to_alt_stack__empty_stack__op_to_alt_stack)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_to_alt_stack(), error::op_to_alt_stack);
}

BOOST_AUTO_TEST_CASE(interpreter__op_from_alt_stack__empty_alt__op_from_alt_stack)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_from_alt_stack(), error::op_from_alt_stack);
}

BOOST_AUTO_TEST_CASE(interpreter__op_to_alt_stack__one_item__primary_empty)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x42 });
    BOOST_REQUIRE_EQUAL(f.op_to_alt_stack(), error::op_success);
    BOOST_REQUIRE(f.empty());
}

BOOST_AUTO_TEST_CASE(interpreter__op_to_alt_from_alt__roundtrip__item_restored)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x42 });
    BOOST_REQUIRE_EQUAL(f.op_to_alt_stack(), error::op_success);
    BOOST_REQUIRE(f.empty());
    BOOST_REQUIRE_EQUAL(f.op_from_alt_stack(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 1u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x42 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_to_alt_stack__two_items__only_top_moved)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    f.push(data_chunk{ 0x02 });
    BOOST_REQUIRE_EQUAL(f.op_to_alt_stack(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 1u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

// OP_2DROP
// Consensus rule: removes the top two stack items.
//   [... a b] -> [...]  (b is on top)
// Error: op_drop2 if stack depth < 2.

BOOST_AUTO_TEST_CASE(interpreter__op_drop2__empty_stack__op_drop2)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_drop2(), error::op_drop2);
}

BOOST_AUTO_TEST_CASE(interpreter__op_drop2__one_item__op_drop2)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    BOOST_REQUIRE_EQUAL(f.op_drop2(), error::op_drop2);
    BOOST_REQUIRE_EQUAL(f.size(), 1u);
}

BOOST_AUTO_TEST_CASE(interpreter__op_drop2__two_items__stack_empty)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    f.push(data_chunk{ 0x02 });
    BOOST_REQUIRE_EQUAL(f.op_drop2(), error::op_success);
    BOOST_REQUIRE(f.empty());
}

BOOST_AUTO_TEST_CASE(interpreter__op_drop2__three_items__one_remains)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    f.push(data_chunk{ 0x02 });
    f.push(data_chunk{ 0x03 });
    BOOST_REQUIRE_EQUAL(f.op_drop2(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 1u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

// OP_2DUP
// Consensus rule: duplicates the top two stack items.
//   [... a b] -> [... a b a b]  (b is on top)
// Error: op_dup2 if stack depth < 2.

BOOST_AUTO_TEST_CASE(interpreter__op_dup2__empty_stack__op_dup2)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_dup2(), error::op_dup2);
}

BOOST_AUTO_TEST_CASE(interpreter__op_dup2__one_item__op_dup2)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    BOOST_REQUIRE_EQUAL(f.op_dup2(), error::op_dup2);
}

BOOST_AUTO_TEST_CASE(interpreter__op_dup2__two_items__both_duplicated)
{
    // push: A (bottom), B (top)
    // after 2dup: A (bottom), B, A, B (top)
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    f.push(data_chunk{ 0x02 });
    BOOST_REQUIRE_EQUAL(f.op_dup2(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 4u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x02 }));
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x02 }));
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

// OP_3DUP
// Consensus rule: duplicates the top three stack items.
//   [... a b c] -> [... a b c a b c]  (c is on top)
// Error: op_dup3 if stack depth < 3.

BOOST_AUTO_TEST_CASE(interpreter__op_dup3__two_items__op_dup3)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    f.push(data_chunk{ 0x02 });
    BOOST_REQUIRE_EQUAL(f.op_dup3(), error::op_dup3);
    BOOST_REQUIRE_EQUAL(f.size(), 2u);
}

BOOST_AUTO_TEST_CASE(interpreter__op_dup3__three_items__all_three_duplicated)
{
    // push: A (bottom), B, C (top)
    // after 3dup: A (bottom), B, C, A, B, C (top)
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    f.push(data_chunk{ 0x02 });
    f.push(data_chunk{ 0x03 });
    BOOST_REQUIRE_EQUAL(f.op_dup3(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 6u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x03 }));
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x02 }));
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x03 }));
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x02 }));
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

// OP_2OVER
// Consensus rule: copies the third and fourth items to the top.
//   [... a1 a2 b1 b2] -> [... a1 a2 b1 b2 a1 a2]  (b2 is on top)
// Error: op_over2 if stack depth < 4.

BOOST_AUTO_TEST_CASE(interpreter__op_over2__three_items__op_over2)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    f.push(data_chunk{ 0x02 });
    f.push(data_chunk{ 0x03 });
    BOOST_REQUIRE_EQUAL(f.op_over2(), error::op_over2);
    BOOST_REQUIRE_EQUAL(f.size(), 3u);
}

BOOST_AUTO_TEST_CASE(interpreter__op_over2__four_items__third_fourth_copied)
{
    // push: A1, A2, B1, B2 (B2 on top)
    // after 2over: B2 is still on top, then B1, A2, A1, A2, A1
    // result order top→bottom: A2, A1, B2, B1, A2, A1
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });  // A1 (bottom)
    f.push(data_chunk{ 0x02 });  // A2
    f.push(data_chunk{ 0x03 });  // B1
    f.push(data_chunk{ 0x04 });  // B2 (top)
    BOOST_REQUIRE_EQUAL(f.op_over2(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 6u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x02 }));  // A2 on top
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));  // A1
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x04 }));  // B2
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x03 }));  // B1
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x02 }));  // A2
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));  // A1 (bottom)
}

// OP_2ROT
// Consensus rule: moves the fifth and sixth items to the top.
//   [... a1 a2 b1 b2 c1 c2] -> [... b1 b2 c1 c2 a1 a2]  (c2 is on top before)
// Error: op_rot2 if stack depth < 6.

BOOST_AUTO_TEST_CASE(interpreter__op_rot2__five_items__op_rot2)
{
    interpreter_fixture f;
    for (uint8_t i = 1u; i <= 5u; ++i)
        f.push(data_chunk{ i });
    BOOST_REQUIRE_EQUAL(f.op_rot2(), error::op_rot2);
    BOOST_REQUIRE_EQUAL(f.size(), 5u);
}

BOOST_AUTO_TEST_CASE(interpreter__op_rot2__six_items__first_pair_moved_to_top)
{
    // push: A1, A2, B1, B2, C1, C2 (C2 on top)
    // after 2rot: result top→bottom: A2, A1, C2, C1, B2, B1
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });  // A1 (bottom)
    f.push(data_chunk{ 0x02 });  // A2
    f.push(data_chunk{ 0x03 });  // B1
    f.push(data_chunk{ 0x04 });  // B2
    f.push(data_chunk{ 0x05 });  // C1
    f.push(data_chunk{ 0x06 });  // C2 (top)
    BOOST_REQUIRE_EQUAL(f.op_rot2(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 6u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x02 }));  // A2 on top
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));  // A1
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x06 }));  // C2
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x05 }));  // C1
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x04 }));  // B2
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x03 }));  // B1 (bottom)
}

// OP_2SWAP
// Consensus rule: swaps the top two pairs of stack items.
//   [... a1 a2 b1 b2] -> [... b1 b2 a1 a2]  (b2 is on top before)
// Error: op_swap2 if stack depth < 4.

BOOST_AUTO_TEST_CASE(interpreter__op_swap2__three_items__op_swap2)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    f.push(data_chunk{ 0x02 });
    f.push(data_chunk{ 0x03 });
    BOOST_REQUIRE_EQUAL(f.op_swap2(), error::op_swap2);
    BOOST_REQUIRE_EQUAL(f.size(), 3u);
}

BOOST_AUTO_TEST_CASE(interpreter__op_swap2__four_items__pairs_swapped)
{
    // push: A1 (bottom), A2, B1, B2 (top)
    // after 2swap: top→bottom: A2, A1, B2, B1
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });  // A1 (bottom)
    f.push(data_chunk{ 0x02 });  // A2
    f.push(data_chunk{ 0x03 });  // B1
    f.push(data_chunk{ 0x04 });  // B2 (top)
    BOOST_REQUIRE_EQUAL(f.op_swap2(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 4u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x02 }));  // A2 on top
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));  // A1
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x04 }));  // B2
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x03 }));  // B1 (bottom)
}

// OP_PICK
// Consensus rule: pops index n from top, then copies the nth item to top.
//   The index 0 would copy the new top (after popping n).
//   [... a_n ... a_1 a_0 n] -> [... a_n ... a_1 a_0 a_n]
// Error: op_pick on underflow, negative index, or out-of-bounds index.

BOOST_AUTO_TEST_CASE(interpreter__op_pick__empty_stack__op_pick)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_pick(), error::op_pick);
}

BOOST_AUTO_TEST_CASE(interpreter__op_pick__index_zero__top_duplicated)
{
    // push: A (item), 0 (index on top)
    // pop index=0, remaining [A], stack_size=1, 0 < 1 -> valid
    // peek_(0) = A, push A -> [A, A]
    interpreter_fixture f;
    f.push(data_chunk{ 0x42 });
    f.push(int64_t(0));
    BOOST_REQUIRE_EQUAL(f.op_pick(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 2u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x42 }));
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x42 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_pick__index_one__second_item_copied)
{
    // push: A (bottom), B, 1 (index on top)
    // pop index=1, remaining [B, A], peek_(1)=A, push A -> [A, B, A]
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });  // A (bottom)
    f.push(data_chunk{ 0x02 });  // B
    f.push(int64_t(1));          // index
    BOOST_REQUIRE_EQUAL(f.op_pick(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 3u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x02 }));
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_pick__out_of_bounds_index__op_pick)
{
    // push: A (item), 5 (index way out of bounds)
    // pop index=5, remaining [A], 5 >= 1 -> false
    interpreter_fixture f;
    f.push(data_chunk{ 0x42 });
    f.push(int64_t(5));
    BOOST_REQUIRE_EQUAL(f.op_pick(), error::op_pick);
    BOOST_REQUIRE_EQUAL(f.size(), 1u);
}

BOOST_AUTO_TEST_CASE(interpreter__op_pick__negative_index__op_pick)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x42 });
    f.push(int64_t(-1));
    BOOST_REQUIRE_EQUAL(f.op_pick(), error::op_pick);
}

// OP_ROLL
// Consensus rule: pops index n from top, removes the nth item and pushes it.
//   [... a_n ... a_1 a_0 n] -> [... ... a_1 a_0 a_n]
// Error: op_roll on underflow, negative index, or out-of-bounds index.

BOOST_AUTO_TEST_CASE(interpreter__op_roll__empty_stack__op_roll)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_roll(), error::op_roll);
}

BOOST_AUTO_TEST_CASE(interpreter__op_roll__index_one__second_moved_to_top)
{
    // push: A (bottom), B, 1 (index on top)
    // pop index=1, remaining [B, A], erase_(1)=remove A -> [B], push A -> [A, B]
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });  // A (bottom)
    f.push(data_chunk{ 0x02 });  // B
    f.push(int64_t(1));          // index
    BOOST_REQUIRE_EQUAL(f.op_roll(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 2u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));  // A moved to top
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x02 }));  // B remains below
}

BOOST_AUTO_TEST_CASE(interpreter__op_roll__out_of_bounds_index__op_roll)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x42 });
    f.push(int64_t(5));
    BOOST_REQUIRE_EQUAL(f.op_roll(), error::op_roll);
}

BOOST_AUTO_TEST_SUITE_END()

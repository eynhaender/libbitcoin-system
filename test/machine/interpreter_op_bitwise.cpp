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

BOOST_AUTO_TEST_SUITE(interpreter_op_bitwise_tests)

// OP_EQUAL
// Consensus rule: {P: stack >= 2} pops two items, pushes 1 (true) if their
//   byte representations are identical, else pushes 0 (false).
//   Comparison is byte-level, not numeric: {0x00} != {}.
// Error: op_equal if stack depth < 2.
// Reference: Script specification, no BIP.

BOOST_AUTO_TEST_CASE(interpreter__op_equal__empty_stack__op_equal)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_equal(), error::op_equal);
}

BOOST_AUTO_TEST_CASE(interpreter__op_equal__one_item__op_equal)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    BOOST_REQUIRE_EQUAL(f.op_equal(), error::op_equal);
    BOOST_REQUIRE_EQUAL(f.size(), 1u);
}

BOOST_AUTO_TEST_CASE(interpreter__op_equal__identical_items__true)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0xab, 0xcd });
    f.push(data_chunk{ 0xab, 0xcd });
    BOOST_REQUIRE_EQUAL(f.op_equal(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 1u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_equal__different_items__false)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    f.push(data_chunk{ 0x02 });
    BOOST_REQUIRE_EQUAL(f.op_equal(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

BOOST_AUTO_TEST_CASE(interpreter__op_equal__both_empty__true)
{
    interpreter_fixture f;
    f.push(data_chunk{});
    f.push(data_chunk{});
    BOOST_REQUIRE_EQUAL(f.op_equal(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

BOOST_AUTO_TEST_CASE(interpreter__op_equal__empty_vs_zero_byte__false)
{
    // OP_EQUAL is byte-level: {} != {0x00}
    interpreter_fixture f;
    f.push(data_chunk{});
    f.push(data_chunk{ 0x00 });
    BOOST_REQUIRE_EQUAL(f.op_equal(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

BOOST_AUTO_TEST_CASE(interpreter__op_equal__different_lengths__false)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    f.push(data_chunk{ 0x01, 0x00 });
    BOOST_REQUIRE_EQUAL(f.op_equal(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});
}

BOOST_AUTO_TEST_CASE(interpreter__op_equal__multi_byte_identical__true)
{
    const data_chunk payload{ 0xde, 0xad, 0xbe, 0xef };
    interpreter_fixture f;
    f.push(payload);
    f.push(payload);
    BOOST_REQUIRE_EQUAL(f.op_equal(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));
}

// OP_EQUALVERIFY
// Consensus rule: same as OP_EQUAL but additionally fails execution if
//   the items are not equal. No item is left on the stack on success.
// Errors: op_equal_verify1 if stack < 2, op_equal_verify2 if not equal.
// Reference: Script specification, no BIP.

BOOST_AUTO_TEST_CASE(interpreter__op_equal_verify__empty_stack__op_equal_verify1)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_equal_verify(), error::op_equal_verify1);
}

BOOST_AUTO_TEST_CASE(interpreter__op_equal_verify__one_item__op_equal_verify1)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0xff });
    BOOST_REQUIRE_EQUAL(f.op_equal_verify(), error::op_equal_verify1);
}

BOOST_AUTO_TEST_CASE(interpreter__op_equal_verify__equal_items__success_stack_empty)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0xff });
    f.push(data_chunk{ 0xff });
    BOOST_REQUIRE_EQUAL(f.op_equal_verify(), error::op_success);
    BOOST_REQUIRE(f.empty());
}

BOOST_AUTO_TEST_CASE(interpreter__op_equal_verify__unequal_items__op_equal_verify2)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    f.push(data_chunk{ 0x02 });
    BOOST_REQUIRE_EQUAL(f.op_equal_verify(), error::op_equal_verify2);
}

BOOST_AUTO_TEST_CASE(interpreter__op_equal_verify__equal_multi_byte__success)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01, 0x02, 0x03 });
    f.push(data_chunk{ 0x01, 0x02, 0x03 });
    BOOST_REQUIRE_EQUAL(f.op_equal_verify(), error::op_success);
    BOOST_REQUIRE(f.empty());
}

BOOST_AUTO_TEST_CASE(interpreter__op_equal_verify__both_empty__success)
{
    interpreter_fixture f;
    f.push(data_chunk{});
    f.push(data_chunk{});
    BOOST_REQUIRE_EQUAL(f.op_equal_verify(), error::op_success);
    BOOST_REQUIRE(f.empty());
}

// OP_SIZE
// Consensus rule: {P: stack >= 1} pushes the byte length of the top item
//   as a script number. The top item itself is NOT removed.
//   [... a] -> [... a len(a)]
// Error: op_size if stack empty.
// Reference: Script specification, no BIP.

BOOST_AUTO_TEST_CASE(interpreter__op_size__empty_stack__op_size)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_size(), error::op_size);
}

BOOST_AUTO_TEST_CASE(interpreter__op_size__empty_chunk__zero_pushed_original_intact)
{
    // Size of an empty chunk is 0; script number 0 = empty chunk
    interpreter_fixture f;
    f.push(data_chunk{});
    BOOST_REQUIRE_EQUAL(f.op_size(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 2u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});      // size = 0
    BOOST_REQUIRE_EQUAL(f.pop_top(), data_chunk{});      // original unchanged
}

BOOST_AUTO_TEST_CASE(interpreter__op_size__one_byte_chunk__one_pushed)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0xab });
    BOOST_REQUIRE_EQUAL(f.op_size(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 2u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));      // size = 1
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0xab }));      // original intact
}

BOOST_AUTO_TEST_CASE(interpreter__op_size__three_byte_chunk__three_pushed)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0xaa, 0xbb, 0xcc });
    BOOST_REQUIRE_EQUAL(f.op_size(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 2u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x03 }));              // size = 3
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0xaa, 0xbb, 0xcc })); // original intact
}

BOOST_AUTO_TEST_CASE(interpreter__op_size__two_items__only_size_of_top_pushed)
{
    // OP_SIZE only reads the top item, leaves the rest untouched
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });          // bottom (1 byte)
    f.push(data_chunk{ 0x02, 0x03 });    // top    (2 bytes)
    BOOST_REQUIRE_EQUAL(f.op_size(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 3u);
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x02 }));          // size of top = 2
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x02, 0x03 }));    // top original
    BOOST_REQUIRE_EQUAL(f.pop_top(), (data_chunk{ 0x01 }));          // bottom untouched
}

BOOST_AUTO_TEST_SUITE_END()

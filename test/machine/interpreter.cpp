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
#include "interpreter.hpp"

BOOST_AUTO_TEST_SUITE(interpreter_tests)

using namespace system::chain;
using namespace system::machine;
using namespace script_vector_detail;

// Path to the vendored Bitcoin Core test vectors.
// Defined at compile time via target_compile_definitions in CMakeLists.txt.
#ifndef TEST_VECTORS_DIR
#define TEST_VECTORS_DIR "../../test/vectors"
#endif

static const std::string vectors_path =
    TEST_VECTORS_DIR "/script_tests.json";

// ============================================================================
// Spec-driven vector tests.
//
// Each non-witness entry from script_tests.json is fed through
// interpreter<contiguous_stack>::connect().  The test asserts the outcome
// specified by the vector, not the current implementation's output.
//
// The transaction construction exactly replicates Bitcoin Core's
// BuildCreditingTransaction / BuildSpendingTransaction helpers: a coinbase
// crediting tx carries the scriptPubKey; the spending tx references its hash
// as the prevout so that CHECKSIG sighash computation is identical.
//
// Known limitations of this fixture and currently observed deviations:
//
//  1. Witness entries (first JSON element is a JSON array) are skipped; they
//     require witness stacks and prevout values not in the non-witness format.
//
//  2. Flags with no direct libbitcoin equivalent are not mapped (see
//     interpreter.hpp).  This accounts for the following invalid vectors that
//     this fixture currently passes (libbitcoin too permissive):
//       SCRIPTNUM (54)          — non-minimal number encoding not enforced
//       MINIMALDATA (21)        — non-minimal push encoding not enforced
//       DISCOURAGE_UPGRADABLE_NOPS (10) — upgradable-NOP policy not enforced
//       CLEANSTACK (2)          — clean-stack rule not enforced
//       SIG_PUSHONLY (2)        — scriptSig push-only rule not enforced
//       SIG_NULLDUMMY (2)       — checkmultisig dummy element not enforced
//       SIG_HASHTYPE (2)        — invalid sighash type not enforced
//       SIG_COUNT (2)           — invalid signature count not enforced
//       PUBKEY_COUNT (2)        — invalid pubkey count not enforced
//       NULLFAIL (3)            — CHECKSIG nullfail rule not enforced
//       PUBKEYTYPE (4)          — invalid pubkey type not enforced
//       SIG_DER (4)             — non-DER sig accepted without DERSIG flag
//       INVALID_STACK_OPERATION (5) — some stack underflows not caught
//       SIG_HIGH_S (1)          — high-S not enforced without LOW_S flag
//
//  3. Libbitcoin enforces strict DER signature encoding unconditionally, even
//     without the DERSIG/STRICTENC flag.  This causes 8 valid vectors of the
//     form [malformed-sig, "0 CHECKSIG NOT", ""] to be incorrectly rejected.
//
//  4. The test transaction uses nSequence=0xffffffff (FINAL) per the JSON
//     header; CHECKLOCKTIMEVERIFY therefore always fails when the flag is
//     active, which is the expected behaviour for invalid CLTV vectors and for
//     valid vectors where the CLTV opcode is in an untaken branch.
// ============================================================================

// Every entry with expected_error == "OK" must be accepted by the interpreter.
BOOST_AUTO_TEST_CASE(interpreter__script_vectors__valid__accepted)
{
    const auto all = load_script_vectors(vectors_path);
    BOOST_REQUIRE_MESSAGE(!all.empty(),
        "Failed to load vectors from: " + vectors_path);

    size_t tested = 0u;
    for (const auto& v : all)
    {
        if (v.is_witness || v.expected_error != "OK")
            continue;

        const auto accepted = run_vector(v);
        const auto label =
            "sig=["   + v.sig_script    + "] "
            "pubkey=[" + v.pubkey_script + "] "
            "flags=["  + v.flags         + "] "
            + (v.comment.empty() ? "" : "(" + v.comment + ")");

        BOOST_CHECK_MESSAGE(accepted, label);
        ++tested;
    }

    BOOST_REQUIRE_MESSAGE(tested > 0u, "No valid vectors were executed");
}

// Every entry with expected_error != "OK" must be rejected by the interpreter.
BOOST_AUTO_TEST_CASE(interpreter__script_vectors__invalid__rejected)
{
    const auto all = load_script_vectors(vectors_path);
    BOOST_REQUIRE_MESSAGE(!all.empty(),
        "Failed to load vectors from: " + vectors_path);

    size_t tested = 0u;
    for (const auto& v : all)
    {
        if (v.is_witness || v.expected_error == "OK")
            continue;

        const auto accepted = run_vector(v);
        const auto label =
            "sig=["    + v.sig_script     + "] "
            "pubkey=[" + v.pubkey_script  + "] "
            "flags=["  + v.flags          + "] "
            "expected_error=[" + v.expected_error + "] "
            + (v.comment.empty() ? "" : "(" + v.comment + ")");

        BOOST_CHECK_MESSAGE(!accepted, label);
        ++tested;
    }

    BOOST_REQUIRE_MESSAGE(tested > 0u, "No invalid vectors were executed");
}

BOOST_AUTO_TEST_SUITE_END()

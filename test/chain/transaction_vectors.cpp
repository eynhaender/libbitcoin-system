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
#include "transaction_vectors.hpp"

BOOST_AUTO_TEST_SUITE(transaction_vectors_tests)

using namespace system::chain;
using namespace tx_vector_detail;

#ifndef TEST_VECTORS_DIR
#define TEST_VECTORS_DIR "../../test/vectors"
#endif

static const std::string tx_valid_path   = TEST_VECTORS_DIR "/tx_valid.json";
static const std::string tx_invalid_path = TEST_VECTORS_DIR "/tx_invalid.json";

// ============================================================================
// tx_valid.json — 121 vectors that the transaction validator must accept.
//
// Each entry supplies the set of prevout UTXOs (txid, vout, scriptPubKey,
// ?amount) needed to fully verify the spending transaction, along with the
// verification flags active for that vector.
//
// Known deviations — vectors incorrectly rejected (libbitcoin too strict):
//
//   LOW_S / STRICTENC unconditionally enforced (~10 vectors):
//     Libbitcoin enforces low-S and strict-encoding regardless of which flags
//     are active in the context.  Vectors where flags=[DERSIG,LOW_S,STRICTENC]
//     or flags=[LOW_S] require a Core-compliant flag-gated check; libbitcoin
//     may reject signatures it should accept under those flags alone.
//
//   NONE-flagged segwit transactions (~26 vectors across both suites):
//     Transactions in the segwit extended format (marker 0x00 + flag 0x01)
//     with flags=[NONE] are expected to be accepted by Core because, without
//     the WITNESS flag, a native segwit output (OP_0 <hash>) evaluates as a
//     simple stack push (non-empty = truthy), making the spend valid.
//     Libbitcoin may reject these due to unconditional witness-path evaluation
//     or script assembly differences for version-0 witness programs.
//
// Total false rejections observed: ~36 of 121.
// ============================================================================

BOOST_AUTO_TEST_CASE(interpreter__tx_valid_vectors__accepted)
{
    const auto all = load_tx_vectors(tx_valid_path);
    BOOST_REQUIRE_MESSAGE(!all.empty(),
        "Failed to load vectors from: " + tx_valid_path);

    size_t tested = 0u;
    for (const auto& v : all)
    {
        const auto accepted = run_tx_vector(v);
        const auto label =
            "tx=[" + v.tx_hex.substr(0u, 40u) + "...] "
            "flags=[" + v.flags + "]";

        BOOST_CHECK_MESSAGE(accepted, label);
        ++tested;
    }

    BOOST_REQUIRE_MESSAGE(tested > 0u, "No valid tx vectors were executed");
}

// ============================================================================
// tx_invalid.json — 93 vectors that the transaction validator must reject.
//
// A vector counts as correctly rejected if either:
//   a) The transaction fails to deserialize (is_valid() == false), or
//   b) tx.connect(ctx) returns a non-success error code.
//
// Known deviations — vectors incorrectly accepted (libbitcoin too permissive):
//
//   BADTX — coinbase-flagged inputs (9 vectors):
//     These txs contain inputs with index=0xffffffff (the coinbase sentinel),
//     which makes is_coinbase() return true.  tx.connect() short-circuits for
//     coinbase transactions and unconditionally returns success.  Detecting
//     that a non-first-in-block transaction is fraudulently claiming the
//     coinbase marker requires chain context that connect() does not have.
//
//   CONST_SCRIPTCODE — OP_CODESEPARATOR subscript rule (12 vectors):
//     Bitcoin Core enforces that the subscript used for signature hashing does
//     not carry a CODESEPARATOR into the committed data.  This flag has no
//     libbitcoin equivalent and is not enforced.
//
//   DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM (3 vectors):
//     Core rejects spends of version-2..16 native segwit programs under this
//     policy flag.  Libbitcoin has no mapping for it.
//
//   NONE-flagged txs with missing-prevout or structural invalidity:
//     A handful of NONE-flagged vectors encode structural invalidity (e.g.
//     spending a prevout that does not exist in the supplied set) that is only
//     caught by full chain validation, not by connect() alone.
//
// Total false acceptances observed: ~24 of 93.
// ============================================================================

BOOST_AUTO_TEST_CASE(interpreter__tx_invalid_vectors__rejected)
{
    const auto all = load_tx_vectors(tx_invalid_path);
    BOOST_REQUIRE_MESSAGE(!all.empty(),
        "Failed to load vectors from: " + tx_invalid_path);

    size_t tested = 0u;
    for (const auto& v : all)
    {
        const auto accepted = run_tx_vector(v);
        const auto label =
            "tx=[" + v.tx_hex.substr(0u, 40u) + "...] "
            "flags=[" + v.flags + "]";

        BOOST_CHECK_MESSAGE(!accepted, label);
        ++tested;
    }

    BOOST_REQUIRE_MESSAGE(tested > 0u, "No invalid tx vectors were executed");
}

BOOST_AUTO_TEST_SUITE_END()

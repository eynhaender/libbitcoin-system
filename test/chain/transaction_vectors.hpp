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
#ifndef LIBBITCOIN_SYSTEM_TEST_CHAIN_TRANSACTION_VECTORS_HPP
#define LIBBITCOIN_SYSTEM_TEST_CHAIN_TRANSACTION_VECTORS_HPP

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>
#include <boost/json.hpp>
#include "../test.hpp"
#include "../machine/interpreter.hpp"

// ============================================================================
// Bitcoin Core transaction test vector support.
//
// Vectors are loaded from test/vectors/tx_valid.json and tx_invalid.json.
// Each non-comment entry has the form:
//
//   [ [[txid, vout, scriptPubKey, ?amount], ...], "tx_hex", "flags" ]
//
// Where:
//   txid         — prevout txid in display order (big-endian, reversed internally)
//   vout         — prevout output index (uint32)
//   scriptPubKey — prevout script in Bitcoin Core asm notation
//   amount       — prevout value in satoshis (present only in segwit vectors)
//   tx_hex       — raw serialized transaction hex
//   flags        — comma-separated verification flags (same format as script_tests.json)
//
// The fixture:
//   1. Deserializes the transaction with witness=true.
//   2. For each tx input, looks up the matching prevout by (txid, vout) and
//      attaches it as a shared output so the interpreter can read the script
//      and value.
//   3. Calls tx.connect(ctx) with flags mapped to a libbitcoin context.
//
// sighash.json is NOT exercised here.  All 500 Core vectors use 32-bit
// hash_type values whose high three bytes are non-zero.  libbitcoin's
// signature_hash() API accepts only uint8_t sighash_flags, which zero-extends
// to 4 bytes in the preimage serialization.  As a result, every vector would
// produce a hash mismatch.  Coverage of legacy sighash must wait for a 32-bit
// sighash_flags overload.
// ============================================================================

namespace tx_vector_detail {

// Parsed representation of one prevout entry in the inputs array.
struct tx_prevout_descriptor
{
    hash_digest txid;       // internal byte order
    uint32_t    vout;
    data_chunk  script_bytes;
    uint64_t    value;      // satoshis; 0 if the field was absent (pre-segwit)
};

// Parsed representation of one tx vector.
struct tx_vector
{
    std::vector<tx_prevout_descriptor> prevouts;
    std::string tx_hex;
    std::string flags;
};

// Load all vectors from tx_valid.json or tx_invalid.json.
static std::vector<tx_vector> load_tx_vectors(const std::string& path)
{
    namespace json = boost::json;

    std::ifstream file(path);
    if (!file.is_open())
        return {};

    const std::string content(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    boost::system::error_code ec;
    const auto jv = json::parse(content, ec);
    if (ec || !jv.is_array())
        return {};

    std::vector<tx_vector> result;

    for (const auto& entry : jv.as_array())
    {
        if (!entry.is_array())
            continue;

        const auto& arr = entry.as_array();

        // Each valid entry: [array_of_prevouts, tx_hex_string, flags_string]
        if (arr.size() < 3u || !arr[0].is_array() ||
            !arr[1].is_string() || !arr[2].is_string())
            continue;

        tx_vector v;
        v.tx_hex = arr[1].as_string().c_str();
        v.flags  = arr[2].as_string().c_str();

        for (const auto& inp : arr[0].as_array())
        {
            if (!inp.is_array() || inp.as_array().size() < 3u)
                continue;

            const auto& ia = inp.as_array();
            if (!ia[0].is_string() || !ia[1].is_number() || !ia[2].is_string())
                continue;

            tx_prevout_descriptor pd;
            decode_hash(pd.txid, ia[0].as_string().c_str());
            pd.vout         = static_cast<uint32_t>(ia[1].to_number<int64_t>());
            pd.script_bytes = script_vector_detail::core_asm_to_bytes(
                                  ia[2].as_string().c_str());
            pd.value        = ia.size() > 3u
                ? static_cast<uint64_t>(ia[3].to_number<int64_t>())
                : uint64_t{ 0u };

            v.prevouts.push_back(std::move(pd));
        }

        result.push_back(std::move(v));
    }

    return result;
}

// Deserialize the transaction and attach prevouts so the interpreter can
// access the prevout script and value for each input.
static chain::transaction build_tx(const tx_vector& v) NOEXCEPT
{
    data_chunk raw;
    if (!decode_base16(raw, v.tx_hex))
        return {};

    // Parse with witness support (the non-witness format is a subset).
    chain::transaction tx(raw, true);
    if (!tx.is_valid())
        return {};

    const auto& inputs = *tx.inputs_ptr();
    for (const auto& inp_ptr : inputs)
    {
        const auto& pt = inp_ptr->point();

        // Find the matching prevout descriptor by (txid, vout).
        for (const auto& pd : v.prevouts)
        {
            if (pd.txid == pt.hash() && pd.vout == pt.index())
            {
                inp_ptr->prevout = to_shared(chain::output{
                    pd.value,
                    chain::script{ pd.script_bytes, false }
                });
                break;
            }
        }
    }

    return tx;
}

// Run one vector.  Returns true if the transaction deserializes successfully
// and all inputs are accepted by the interpreter.
static bool run_tx_vector(const tx_vector& v) NOEXCEPT
{
    const auto tx = build_tx(v);
    if (!tx.is_valid())
        return false;

    const chain::context ctx{
        script_vector_detail::flags_from_core_string(v.flags) };
    return !tx.connect(ctx);
}

} // namespace tx_vector_detail

#endif // LIBBITCOIN_SYSTEM_TEST_CHAIN_TRANSACTION_VECTORS_HPP

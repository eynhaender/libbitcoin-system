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

BOOST_AUTO_TEST_SUITE(transaction_mainnet_tests)

using namespace system::chain;

// ============================================================================
// Spec-driven deserialization tests.  Each test feeds a known raw-hex Bitcoin
// transaction through the deserializer and asserts exact field values against
// an externally-sourced ground truth.  No roundtrip is involved: the input hex
// is the oracle, not the output of any library operation.
//
// Transactions:
//   1. Genesis coinbase (block 0, 3 Jan 2009)
//      txid: 4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b
//      Source: Bitcoin's genesis block (the same bytes are embedded in
//      settings.cpp for every Bitcoin client, and the SHA256d hash matches the
//      well-known merkle root of block 0).
//
//   2. Block-170 (Satoshi → Hal Finney, 12 Jan 2009)
//      txid: f4184fc596403b9d638783cf57adfe4c75c605f6356fbc91338530e9831e9e16
//      Source: blockchain.info raw tx endpoint; txid verified by SHA256d.
//
//   3. Segwit P2WPKH spend (Bitcoin Core tx_valid.json)
//      txid:  b2ce556154e5ab22bec0a2f990b2b843f4f4085486c0d2cd82873685c0012004
//      wtxid: 7944c8f36d682addda15124399bf954ec5d4b3a426e9d505a5f74a08644f0ebb
//      Source: Bitcoin Core src/test/data/tx_valid.json.  The prevout is
//      synthetic (0x...0100:0) but the transaction structure and witness data
//      are cryptographically valid P2WPKH, and both hashes were verified by
//      SHA256d over the stripped/full serializations.
// ============================================================================

// ----------------------------------------------------------------------------
// 1. Genesis coinbase
//    version=1  inputs=1 (coinbase)  outputs=1  locktime=0
//    input:  null prevout, scriptSig = "The Times..." newspaper headline
//    output: 5,000,000,000 satoshis, P2PK to Satoshi's uncompressed pubkey
// ----------------------------------------------------------------------------

static const auto genesis_coinbase_data = base16_chunk(
    "01000000"                                              // version=1
    "01"                                                    // 1 input
    "0000000000000000000000000000000000000000000000000000000000000000"
    "ffffffff"                                              // coinbase index (0xffffffff)
    "4d"                                                    // scriptSig len=77
    "04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368"
    "616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c"
    "6f757420666f722062616e6b73"
    "ffffffff"                                              // sequence=FINAL
    "01"                                                    // 1 output
    "00f2052a01000000"                                      // 5,000,000,000 satoshis
    "43"                                                    // script len=67
    "4104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61"
    "deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf"
    "11d5fac"
    "00000000");                                             // locktime=0

constexpr auto genesis_coinbase_txid = base16_hash(
    "4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b");

BOOST_AUTO_TEST_CASE(transaction__genesis_coinbase__deserialization__valid)
{
    const transaction tx(genesis_coinbase_data, false);
    BOOST_REQUIRE(tx.is_valid());
}

BOOST_AUTO_TEST_CASE(transaction__genesis_coinbase__structure__expected)
{
    const transaction tx(genesis_coinbase_data, false);
    BOOST_REQUIRE(tx.is_valid());

    BOOST_REQUIRE_EQUAL(tx.version(), 1u);
    BOOST_REQUIRE_EQUAL(tx.inputs(), 1u);
    BOOST_REQUIRE_EQUAL(tx.outputs(), 1u);
    BOOST_REQUIRE_EQUAL(tx.locktime(), 0u);

    BOOST_REQUIRE(tx.is_coinbase());
    BOOST_REQUIRE(!tx.is_segregated());
}

BOOST_AUTO_TEST_CASE(transaction__genesis_coinbase__input__expected)
{
    const transaction tx(genesis_coinbase_data, false);
    BOOST_REQUIRE(tx.is_valid());

    const auto& in = *tx.inputs_ptr()->front();

    // Coinbase: prevout hash is all zeros, index is 0xffffffff.
    BOOST_REQUIRE_EQUAL(in.point().index(), max_uint32);
    BOOST_REQUIRE_EQUAL(in.sequence(), max_uint32);

    // ScriptSig contains the famous newspaper headline (77 bytes).
    BOOST_REQUIRE_EQUAL(in.script().serialized_size(false), 77u);
}

BOOST_AUTO_TEST_CASE(transaction__genesis_coinbase__output__expected)
{
    const transaction tx(genesis_coinbase_data, false);
    BOOST_REQUIRE(tx.is_valid());

    const auto& out = *tx.outputs_ptr()->front();

    // Exactly 50 BTC block reward.
    BOOST_REQUIRE_EQUAL(out.value(), 5'000'000'000u);

    // P2PK: push-65 + uncompressed pubkey (65 bytes) + OP_CHECKSIG = 67 bytes.
    BOOST_REQUIRE_EQUAL(out.script().serialized_size(false), 67u);
}

BOOST_AUTO_TEST_CASE(transaction__genesis_coinbase__hash__expected)
{
    const transaction tx(genesis_coinbase_data, false);
    BOOST_REQUIRE(tx.is_valid());

    // Non-witness hash equals the genesis block's merkle root.
    BOOST_REQUIRE_EQUAL(tx.hash(false), genesis_coinbase_txid);

    // Non-segregated: witness and non-witness hashes are identical.
    BOOST_REQUIRE_EQUAL(tx.hash(true), genesis_coinbase_txid);
}

BOOST_AUTO_TEST_CASE(transaction__genesis_coinbase__roundtrip__expected)
{
    const transaction tx(genesis_coinbase_data, false);
    BOOST_REQUIRE(tx.is_valid());

    BOOST_REQUIRE_EQUAL(tx.to_data(false), genesis_coinbase_data);
}

// ----------------------------------------------------------------------------
// 2. Block-170: first non-coinbase Bitcoin transaction.
//    Satoshi Nakamoto → Hal Finney, 12 Jan 2009.
//    version=1  inputs=1  outputs=2  locktime=0
//    input:  spends block-9 coinbase (0437cd7f...:0)
//    output[0]: 10 BTC P2PK to Hal Finney's uncompressed pubkey
//    output[1]: 40 BTC P2PK change back to Satoshi
// ----------------------------------------------------------------------------

static const auto block170_data = base16_chunk(
    "01000000"                                              // version=1
    "01"                                                    // 1 input
    "c997a5e56e104102fa209c6a852dd90660a20b2d9c352423edce25857fcd3704"
    "00000000"                                              // prevout index=0
    "48"                                                    // scriptSig len=72
    "47304402204e45e16932b8af514961a1d3a1a25fdf3f4f7732e9d624c6c61548"
    "ab5fb8cd410220181522ec8eca07de4860a4acdd12909d831cc56cbbac462208"
    "2221a8768d1d0901"
    "ffffffff"                                              // sequence=FINAL
    "02"                                                    // 2 outputs
    // output[0]: 10 BTC → Hal Finney's P2PK
    "00ca9a3b00000000"                                      // 1,000,000,000 satoshis
    "43"                                                    // script len=67
    "4104ae1a62fe09c5f51b13905f07f06b99a2f7159b2225f374cd378d71302fa"
    "28414e7aab37397f554a7df5f142c21c1b7303b8a0626f1baded5c72a704f7e"
    "6cd84cac"
    // output[1]: 40 BTC → Satoshi's change P2PK
    "00286bee00000000"                                      // 4,000,000,000 satoshis
    "43"                                                    // script len=67
    "410411db93e1dcdb8a016b49840f8c53bc1eb68a382e97b1482ecad7b148a690"
    "9a5cb2e0eaddfb84ccf9744464f82e160bfa9b8b64f9d4c03f999b8643f656b"
    "412a3ac"
    "00000000");                                             // locktime=0

constexpr auto block170_prevout_txid = base16_hash(
    "0437cd7f8525ceed2324359c2d0ba26006d92d856a9c20fa0241106ee5a597c9");

constexpr auto block170_txid = base16_hash(
    "f4184fc596403b9d638783cf57adfe4c75c605f6356fbc91338530e9831e9e16");

BOOST_AUTO_TEST_CASE(transaction__block170__deserialization__valid)
{
    const transaction tx(block170_data, false);
    BOOST_REQUIRE(tx.is_valid());
}

BOOST_AUTO_TEST_CASE(transaction__block170__structure__expected)
{
    const transaction tx(block170_data, false);
    BOOST_REQUIRE(tx.is_valid());

    BOOST_REQUIRE_EQUAL(tx.version(), 1u);
    BOOST_REQUIRE_EQUAL(tx.inputs(), 1u);
    BOOST_REQUIRE_EQUAL(tx.outputs(), 2u);
    BOOST_REQUIRE_EQUAL(tx.locktime(), 0u);

    BOOST_REQUIRE(!tx.is_coinbase());
    BOOST_REQUIRE(!tx.is_segregated());
}

BOOST_AUTO_TEST_CASE(transaction__block170__input__expected)
{
    const transaction tx(block170_data, false);
    BOOST_REQUIRE(tx.is_valid());

    const auto& in = *tx.inputs_ptr()->front();

    // Prevout: block-9 coinbase txid, output index 0.
    BOOST_REQUIRE_EQUAL(in.point().hash(), block170_prevout_txid);
    BOOST_REQUIRE_EQUAL(in.point().index(), 0u);
    BOOST_REQUIRE_EQUAL(in.sequence(), max_uint32);
}

BOOST_AUTO_TEST_CASE(transaction__block170__outputs__expected)
{
    const transaction tx(block170_data, false);
    BOOST_REQUIRE(tx.is_valid());

    const auto& outs = *tx.outputs_ptr();

    // output[0]: 10 BTC to Hal Finney.
    BOOST_REQUIRE_EQUAL(outs[0]->value(), 1'000'000'000u);

    // output[1]: 40 BTC change back to Satoshi.
    BOOST_REQUIRE_EQUAL(outs[1]->value(), 4'000'000'000u);

    // Both are P2PK: push-65 + uncompressed pubkey + OP_CHECKSIG = 67 bytes.
    BOOST_REQUIRE_EQUAL(outs[0]->script().serialized_size(false), 67u);
    BOOST_REQUIRE_EQUAL(outs[1]->script().serialized_size(false), 67u);
}

BOOST_AUTO_TEST_CASE(transaction__block170__hash__expected)
{
    const transaction tx(block170_data, false);
    BOOST_REQUIRE(tx.is_valid());

    BOOST_REQUIRE_EQUAL(tx.hash(false), block170_txid);
    BOOST_REQUIRE_EQUAL(tx.hash(true), block170_txid);
}

BOOST_AUTO_TEST_CASE(transaction__block170__roundtrip__expected)
{
    const transaction tx(block170_data, false);
    BOOST_REQUIRE(tx.is_valid());

    BOOST_REQUIRE_EQUAL(tx.to_data(false), block170_data);
}

// ----------------------------------------------------------------------------
// 3. Segwit P2WPKH spending transaction (Bitcoin Core tx_valid.json).
//    version=1  inputs=1  outputs=1  locktime=0  (segregated, witness present)
//    input:  native P2WPKH spend, empty scriptSig, 2-item witness stack
//    output: 1000 satoshis, P2PKH
//    txid  (stripped): b2ce556154e5ab22bec0a2f990b2b843f4f4085486c0d2cd82873685c0012004
//    wtxid (witness):  7944c8f36d682addda15124399bf954ec5d4b3a426e9d505a5f74a08644f0ebb
// ----------------------------------------------------------------------------

static const auto segwit_p2wpkh_data = base16_chunk(
    "01000000"                                              // version=1
    "0001"                                                  // segwit marker + flag
    "01"                                                    // 1 input
    "0001000000000000000000000000000000000000000000000000000000000000"
    "00000000"                                              // prevout index=0
    "00"                                                    // empty scriptSig (native segwit)
    "ffffffff"                                              // sequence=FINAL
    "01"                                                    // 1 output
    "e803000000000000"                                      // 1,000 satoshis
    "19"                                                    // script len=25
    "76a9144c9c3dfac4207d5d8cb89df5722cb3d712385e3f88ac"   // P2PKH
    // witness for input 0: 2 items (DER signature + compressed pubkey)
    "02"
    "48"                                                    // 72-byte DER signature
    "3045022100cfb07164b36ba64c1b1e8c7720a56ad64d96f6ef332d3d37f9cb3c"
    "96477dc44502200a464cd7a9cf94cd70f66ce4f4f0625ef650052c7afcfe29d7"
    "d7e01830ff91ed01"
    "21"                                                    // 33-byte compressed pubkey
    "03596d3451025c19dbbdeb932d6bf8bfb4ad499b95b6f88db8899efac102e5fc71"
    "00000000");                                             // locktime=0

constexpr auto segwit_txid = base16_hash(
    "b2ce556154e5ab22bec0a2f990b2b843f4f4085486c0d2cd82873685c0012004");

constexpr auto segwit_wtxid = base16_hash(
    "7944c8f36d682addda15124399bf954ec5d4b3a426e9d505a5f74a08644f0ebb");

BOOST_AUTO_TEST_CASE(transaction__segwit_p2wpkh__deserialization__valid)
{
    const transaction tx(segwit_p2wpkh_data, true);
    BOOST_REQUIRE(tx.is_valid());
}

BOOST_AUTO_TEST_CASE(transaction__segwit_p2wpkh__structure__expected)
{
    const transaction tx(segwit_p2wpkh_data, true);
    BOOST_REQUIRE(tx.is_valid());

    BOOST_REQUIRE_EQUAL(tx.version(), 1u);
    BOOST_REQUIRE_EQUAL(tx.inputs(), 1u);
    BOOST_REQUIRE_EQUAL(tx.outputs(), 1u);
    BOOST_REQUIRE_EQUAL(tx.locktime(), 0u);

    BOOST_REQUIRE(!tx.is_coinbase());
    BOOST_REQUIRE(tx.is_segregated());
}

BOOST_AUTO_TEST_CASE(transaction__segwit_p2wpkh__input__expected)
{
    const transaction tx(segwit_p2wpkh_data, true);
    BOOST_REQUIRE(tx.is_valid());

    const auto& in = *tx.inputs_ptr()->front();

    BOOST_REQUIRE_EQUAL(in.point().index(), 0u);
    BOOST_REQUIRE_EQUAL(in.sequence(), max_uint32);

    // Native P2WPKH has an empty scriptSig.
    BOOST_REQUIRE_EQUAL(in.script().serialized_size(false), 0u);

    // Witness carries 2 items: DER sig (72 bytes) + compressed pubkey (33 bytes).
    const auto& stack = in.witness().stack();
    BOOST_REQUIRE_EQUAL(stack.size(), 2u);
    BOOST_REQUIRE_EQUAL(stack[0]->size(), 72u);
    BOOST_REQUIRE_EQUAL(stack[1]->size(), 33u);
}

BOOST_AUTO_TEST_CASE(transaction__segwit_p2wpkh__output__expected)
{
    const transaction tx(segwit_p2wpkh_data, true);
    BOOST_REQUIRE(tx.is_valid());

    const auto& out = *tx.outputs_ptr()->front();

    BOOST_REQUIRE_EQUAL(out.value(), 1000u);

    // P2PKH script: OP_DUP OP_HASH160 PUSH20 <hash> OP_EQUALVERIFY OP_CHECKSIG = 25 bytes.
    BOOST_REQUIRE_EQUAL(out.script().serialized_size(false), 25u);
}

BOOST_AUTO_TEST_CASE(transaction__segwit_p2wpkh__hash__expected)
{
    const transaction tx(segwit_p2wpkh_data, true);
    BOOST_REQUIRE(tx.is_valid());

    // Non-witness txid (stripped serialization, no witness data).
    BOOST_REQUIRE_EQUAL(tx.hash(false), segwit_txid);

    // Witness txid / wtxid (full serialization including witness).
    BOOST_REQUIRE_EQUAL(tx.hash(true), segwit_wtxid);

    // The two hashes must differ for any segregated transaction.
    BOOST_REQUIRE(tx.hash(false) != tx.hash(true));
}

BOOST_AUTO_TEST_CASE(transaction__segwit_p2wpkh__roundtrip__expected)
{
    const transaction tx(segwit_p2wpkh_data, true);
    BOOST_REQUIRE(tx.is_valid());

    // Witness serialization round-trip.
    BOOST_REQUIRE_EQUAL(tx.to_data(true), segwit_p2wpkh_data);

    // Stripped serialization is shorter (no marker, flag, or witness fields).
    BOOST_REQUIRE(tx.to_data(false).size() < tx.to_data(true).size());
}

BOOST_AUTO_TEST_SUITE_END()

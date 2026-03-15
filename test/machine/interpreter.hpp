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
#ifndef LIBBITCOIN_SYSTEM_TEST_MACHINE_INTERPRETER_HPP
#define LIBBITCOIN_SYSTEM_TEST_MACHINE_INTERPRETER_HPP

#include "../test.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <boost/json.hpp>

// ============================================================================
// Bitcoin Core script test vector support.
//
// Vectors are loaded from test/vectors/script_tests.json, which combines
// valid and invalid entries.  Each non-comment, non-witness entry has the
// form:
//   [scriptSig, scriptPubKey, flags, expected_error, comment?]
//
// "expected_error" is "OK" for vectors that must be accepted and any other
// string (e.g. "EVAL_FALSE", "CLEANSTACK") for vectors that must be rejected.
//
// The transaction construction matches the Bitcoin Core description in the
// JSON header: all nLockTimes are 0, all nSequences are max (0xffffffff),
// prevout value is 0 satoshi.
//
// Script assembly format (Bitcoin Core style):
//   NAME / OP_NAME  → opcode byte (case-insensitive, OP_ prefix optional)
//   0xNN...         → raw hex bytes appended directly to the serialized script
//   N (integer)     → CScriptNum-encoded push (OP_0, OP_1..OP_16, or data)
//   'string'        → literal bytes with appropriate PUSH prefix
//
// Flag mapping (unmapped Bitcoin Core flags are silently ignored; libbitcoin
// may enforce their equivalent behaviour as part of its baseline rules):
//   P2SH                  → bip16_rule
//   DERSIG                → bip66_rule
//   NULLDUMMY             → bip147_rule
//   CHECKLOCKTIMEVERIFY   → bip65_rule
//   CHECKSEQUENCEVERIFY   → bip112_rule | bip68_rule | bip113_rule
//   WITNESS               → bip141_rule | bip143_rule | bip147_rule
//   TAPROOT               → bip341_rule | bip342_rule
//   (unmapped: STRICTENC, LOW_S, MINIMALDATA, SIGPUSHONLY, CLEANSTACK,
//    DISCOURAGE_UPGRADABLE_NOPS, DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM,
//    MINIMALIF, NULLFAIL, WITNESS_PUBKEYTYPE,
//    DISCOURAGE_UPGRADABLE_TAPROOT_VERSION, DISCOURAGE_OP_SUCCESS,
//    DISCOURAGE_UPGRADABLE_PUBKEYTYPE)
//
// Witness entries (first element is a JSON array) are skipped; they require
// prevout values, witness stacks, and segwit-aware transaction construction
// that is out of scope for this fixture.
// ============================================================================

namespace script_vector_detail {

// ----------------------------------------------------------------------------
// Opcode name → wire byte (both with and without the OP_ prefix are accepted).
// ----------------------------------------------------------------------------
static const std::unordered_map<std::string, uint8_t>& opcode_table() NOEXCEPT
{
    static const std::unordered_map<std::string, uint8_t> table
    {
        // Push
        { "0",                      0x00 },
        { "false",                  0x00 },
        { "pushdata1",              0x4c },
        { "pushdata2",              0x4d },
        { "pushdata4",              0x4e },
        { "1negate",                0x4f },
        { "reserved",               0x50 },
        { "1",                      0x51 },
        { "true",                   0x51 },
        { "2",                      0x52 },
        { "3",                      0x53 },
        { "4",                      0x54 },
        { "5",                      0x55 },
        { "6",                      0x56 },
        { "7",                      0x57 },
        { "8",                      0x58 },
        { "9",                      0x59 },
        { "10",                     0x5a },
        { "11",                     0x5b },
        { "12",                     0x5c },
        { "13",                     0x5d },
        { "14",                     0x5e },
        { "15",                     0x5f },
        { "16",                     0x60 },
        // Flow control
        { "nop",                    0x61 },
        { "ver",                    0x62 },
        { "if",                     0x63 },
        { "notif",                  0x64 },
        { "verif",                  0x65 },
        { "vernotif",               0x66 },
        { "else",                   0x67 },
        { "endif",                  0x68 },
        { "verify",                 0x69 },
        { "return",                 0x6a },
        // Stack
        { "toaltstack",             0x6b },
        { "fromaltstack",           0x6c },
        { "2drop",                  0x6d },
        { "2dup",                   0x6e },
        { "3dup",                   0x6f },
        { "2over",                  0x70 },
        { "2rot",                   0x71 },
        { "2swap",                  0x72 },
        { "ifdup",                  0x73 },
        { "depth",                  0x74 },
        { "drop",                   0x75 },
        { "dup",                    0x76 },
        { "nip",                    0x77 },
        { "over",                   0x78 },
        { "pick",                   0x79 },
        { "roll",                   0x7a },
        { "rot",                    0x7b },
        { "swap",                   0x7c },
        { "tuck",                   0x7d },
        // Splice (disabled post-cats_rule)
        { "cat",                    0x7e },
        { "substr",                 0x7f },
        { "left",                   0x80 },
        { "right",                  0x81 },
        { "size",                   0x82 },
        // Bitwise (disabled post-cats_rule)
        { "invert",                 0x83 },
        { "and",                    0x84 },
        { "or",                     0x85 },
        { "xor",                    0x86 },
        // Equality
        { "equal",                  0x87 },
        { "equalverify",            0x88 },
        { "reserved1",              0x89 },
        { "reserved2",              0x8a },
        // Arithmetic
        { "1add",                   0x8b },
        { "1sub",                   0x8c },
        { "2mul",                   0x8d },
        { "2div",                   0x8e },
        { "negate",                 0x8f },
        { "abs",                    0x90 },
        { "not",                    0x91 },
        { "0notequal",              0x92 },
        { "add",                    0x93 },
        { "sub",                    0x94 },
        { "mul",                    0x95 },
        { "div",                    0x96 },
        { "mod",                    0x97 },
        { "lshift",                 0x98 },
        { "rshift",                 0x99 },
        { "booland",                0x9a },
        { "boolor",                 0x9b },
        { "numequal",               0x9c },
        { "numequalverify",         0x9d },
        { "numnotequal",            0x9e },
        { "lessthan",               0x9f },
        { "greaterthan",            0xa0 },
        { "lessthanorequal",        0xa1 },
        { "greaterthanorequal",     0xa2 },
        { "min",                    0xa3 },
        { "max",                    0xa4 },
        { "within",                 0xa5 },
        // Crypto
        { "ripemd160",              0xa6 },
        { "sha1",                   0xa7 },
        { "sha256",                 0xa8 },
        { "hash160",                0xa9 },
        { "hash256",                0xaa },
        { "codeseparator",          0xab },
        { "checksig",               0xac },
        { "checksigverify",         0xad },
        { "checkmultisig",          0xae },
        { "checkmultisigverify",    0xaf },
        // Locktime / NOP expansions
        { "nop1",                   0xb0 },
        { "checklocktimeverify",    0xb1 },
        { "nop2",                   0xb1 },
        { "checksequenceverify",    0xb2 },
        { "nop3",                   0xb2 },
        { "nop4",                   0xb3 },
        { "nop5",                   0xb4 },
        { "nop6",                   0xb5 },
        { "nop7",                   0xb6 },
        { "nop8",                   0xb7 },
        { "nop9",                   0xb8 },
        { "nop10",                  0xb9 },
        // Tapscript
        { "checksigadd",            0xba },
        // Sentinel for unrecognised names
        { "invalidopcode",          0xff },
    };
    return table;
}

// ----------------------------------------------------------------------------
// Encode integer n as the minimal Bitcoin script push instruction(s).
// Matches CScript << int64_t in Bitcoin Core.
// ----------------------------------------------------------------------------
static data_chunk int_to_script_bytes(int64_t n) NOEXCEPT
{
    if (n == 0)
        return { 0x00 };
    if (n == -1)
        return { 0x4f };
    if (n >= 1 && n <= 16)
        return { static_cast<uint8_t>(0x50 + n) };

    // CScriptNum: little-endian, sign bit in the MSB of the last byte.
    const bool negative = n < 0;
    uint64_t absval = negative
        ? static_cast<uint64_t>(-(n + 1)) + 1u
        : static_cast<uint64_t>(n);

    data_chunk num;
    while (absval > 0)
    {
        num.push_back(static_cast<uint8_t>(absval & 0xff));
        absval >>= 8;
    }

    if (num.back() & 0x80)
        num.push_back(negative ? 0x80u : 0x00u);
    else if (negative)
        num.back() |= 0x80;

    // Prepend the push opcode.
    const auto len = num.size();
    data_chunk result;
    if (len <= 75u)
    {
        result.push_back(static_cast<uint8_t>(len));
    }
    else if (len <= 0xffu)
    {
        result.push_back(0x4c);
        result.push_back(static_cast<uint8_t>(len));
    }
    else if (len <= 0xffffu)
    {
        result.push_back(0x4d);
        result.push_back(static_cast<uint8_t>(len & 0xffu));
        result.push_back(static_cast<uint8_t>(len >> 8));
    }
    else
    {
        result.push_back(0x4e);
        result.push_back(static_cast<uint8_t>((len >>  0) & 0xffu));
        result.push_back(static_cast<uint8_t>((len >>  8) & 0xffu));
        result.push_back(static_cast<uint8_t>((len >> 16) & 0xffu));
        result.push_back(static_cast<uint8_t>((len >> 24) & 0xffu));
    }

    result.insert(result.end(), num.begin(), num.end());
    return result;
}

// ----------------------------------------------------------------------------
// Encode a string literal push ('...').
// Matches CScript << std::vector<uint8_t> in Bitcoin Core.
// ----------------------------------------------------------------------------
static data_chunk str_to_script_bytes(const std::string& s) NOEXCEPT
{
    const auto len = s.size();
    data_chunk result;

    if (len == 0u)
    {
        result.push_back(0x00);
        return result;
    }

    if (len <= 75u)
        result.push_back(static_cast<uint8_t>(len));
    else if (len <= 0xffu)
    {
        result.push_back(0x4c);
        result.push_back(static_cast<uint8_t>(len));
    }
    else
    {
        result.push_back(0x4d);
        result.push_back(static_cast<uint8_t>(len & 0xffu));
        result.push_back(static_cast<uint8_t>(len >> 8));
    }

    for (const auto c : s)
        result.push_back(static_cast<uint8_t>(c));

    return result;
}

// ----------------------------------------------------------------------------
// Parse a Bitcoin Core script assembly string to raw serialized script bytes.
// ----------------------------------------------------------------------------
static data_chunk core_asm_to_bytes(const std::string& asm_str) NOEXCEPT
{
    data_chunk result;
    const auto& opcodes = opcode_table();

    std::istringstream ss(asm_str);
    std::string token;
    while (ss >> token)
    {
        if (token.empty())
            continue;

        // 0xNN... — raw hex bytes appended verbatim (no push wrapper).
        if (token.size() >= 2u &&
            token[0] == '0' && (token[1] == 'x' || token[1] == 'X'))
        {
            const auto hex_view = std::string_view(token).substr(2);
            data_chunk bytes(hex_view.size() / 2);
            if (!hex_view.empty() && decode_base16(bytes, hex_view))
                result.insert(result.end(), bytes.begin(), bytes.end());
            continue;
        }

        // 'string literal' — push the literal bytes with a push prefix.
        if (token.front() == '\'' && token.back() == '\'' && token.size() >= 2u)
        {
            const auto pushed = str_to_script_bytes(
                token.substr(1u, token.size() - 2u));
            result.insert(result.end(), pushed.begin(), pushed.end());
            continue;
        }

        // Opcode lookup: strip OP_ prefix if present, lowercase, look up.
        std::string name = token;
        if (name.size() > 3u && name.substr(0u, 3u) == "OP_")
            name = name.substr(3u);

        std::transform(name.begin(), name.end(), name.begin(),
            [](unsigned char c){ return std::tolower(c); });

        const auto it = opcodes.find(name);
        if (it != opcodes.end())
        {
            result.push_back(it->second);
            continue;
        }

        // Integer literal (possibly negative).
        try
        {
            const int64_t n = std::stoll(token);
            const auto pushed = int_to_script_bytes(n);
            result.insert(result.end(), pushed.begin(), pushed.end());
            continue;
        }
        catch (...) {}

        // Unrecognised token — emit OP_INVALIDOPCODE so the script fails.
        result.push_back(0xff);
    }

    return result;
}

// ----------------------------------------------------------------------------
// Map a Bitcoin Core comma-separated flag string to libbitcoin chain::flags.
// ----------------------------------------------------------------------------
static uint32_t flags_from_core_string(const std::string& flag_str) NOEXCEPT
{
    using namespace system::chain;
    uint32_t result = flags::no_rules;

    std::istringstream ss(flag_str);
    std::string flag;
    while (std::getline(ss, flag, ','))
    {
        const auto first = flag.find_first_not_of(" \t");
        const auto last  = flag.find_last_not_of(" \t");
        if (first == std::string::npos)
            continue;
        flag = flag.substr(first, last - first + 1u);

        if      (flag == "P2SH")
            result |= flags::bip16_rule;
        else if (flag == "DERSIG")
            result |= flags::bip66_rule;
        else if (flag == "NULLDUMMY")
            result |= flags::bip147_rule;
        else if (flag == "CHECKLOCKTIMEVERIFY")
            result |= flags::bip65_rule;
        else if (flag == "CHECKSEQUENCEVERIFY")
            result |= (flags::bip112_rule | flags::bip68_rule | flags::bip113_rule);
        else if (flag == "WITNESS")
            result |= (flags::bip141_rule | flags::bip143_rule | flags::bip147_rule);
        else if (flag == "TAPROOT")
            result |= (flags::bip341_rule | flags::bip342_rule);
        // else: silently ignore flags with no libbitcoin equivalent
    }

    return result;
}

// ----------------------------------------------------------------------------
// Parsed representation of one test vector.
// ----------------------------------------------------------------------------
struct script_vector
{
    std::string sig_script;
    std::string pubkey_script;
    std::string flags;
    std::string expected_error;  // "OK" = must pass; else must fail
    std::string comment;
    bool        is_witness;      // true → entry skipped by this fixture
};

// ----------------------------------------------------------------------------
// Load all vectors from the Bitcoin Core script_tests.json file.
// ----------------------------------------------------------------------------
static std::vector<script_vector> load_script_vectors(const std::string& path)
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

    std::vector<script_vector> result;
    for (const auto& entry : jv.as_array())
    {
        if (!entry.is_array())
            continue;

        const auto& arr = entry.as_array();
        if (arr.empty())
            continue;

        // Witness format: first element is an array.
        // [[witness_items..., amount], scriptSig, scriptPubKey, flags, error, comment?]
        if (arr[0].is_array())
        {
            if (arr.size() < 5u)
                continue;

            script_vector v;
            v.is_witness     = true;
            v.sig_script     = arr[1].is_string() ? arr[1].as_string().c_str() : "";
            v.pubkey_script  = arr[2].is_string() ? arr[2].as_string().c_str() : "";
            v.flags          = arr[3].is_string() ? arr[3].as_string().c_str() : "";
            v.expected_error = arr[4].is_string() ? arr[4].as_string().c_str() : "";
            v.comment        = arr.size() > 5u && arr[5].is_string()
                               ? arr[5].as_string().c_str() : "";
            result.push_back(std::move(v));
            continue;
        }

        // Comment-only entries or malformed entries.
        if (!arr[0].is_string() || arr.size() < 4u)
            continue;

        // Legacy format: [scriptSig, scriptPubKey, flags, error, comment?]
        script_vector v;
        v.is_witness     = false;
        v.sig_script     = arr[0].as_string().c_str();
        v.pubkey_script  = arr[1].is_string() ? arr[1].as_string().c_str() : "";
        v.flags          = arr[2].is_string() ? arr[2].as_string().c_str() : "";
        v.expected_error = arr[3].is_string() ? arr[3].as_string().c_str() : "";
        v.comment        = arr.size() > 4u && arr[4].is_string()
                           ? arr[4].as_string().c_str() : "";
        result.push_back(std::move(v));
    }

    return result;
}

// Build the spending transaction that matches Bitcoin Core's script_tests.json
// construction exactly:
//
//   Crediting tx (coinbase-style):
//     version=1, locktime=0, sequence=FINAL
//     vin[0]:  prevout={null_hash, 0xffffffff}, scriptSig={OP_0, OP_0}
//     vout[0]: value=0, scriptPubKey=<tested scriptPubKey>
//
//   Spending tx:
//     version=1, locktime=0, sequence=FINAL
//     vin[0]:  prevout={hash(creditingTx), 0}, scriptSig=<tested scriptSig>
//     vout[0]: value=0, scriptPubKey=<empty>
//
// The spending tx's prevout shared_ptr is set to output{0, pubkey} so that the
// interpreter can read the prevout script and value without needing the actual
// crediting tx in memory.
//
// Using the crediting tx hash as the prevout hash exactly replicates the sighash
// that Bitcoin Core uses when testing CHECKSIG vectors, so real signatures in
// the valid vector set verify correctly.
static system::chain::transaction make_vector_tx(
    const system::chain::script& sig,
    const system::chain::script& pubkey) NOEXCEPT
{
    using namespace system::chain;

    // Crediting transaction: coinbase-style, output carries the tested script.
    // scriptSig = {OP_0, OP_0} as in Bitcoin Core's BuildCreditingTransaction.
    const script crediting_sig{ data_chunk{ 0x00u, 0x00u }, false };
    const transaction crediting_tx
    {
        1u,
        inputs{{ point{}, crediting_sig, max_uint32 }},
        outputs{{ 0u, pubkey }},
        0u
    };

    // Spending transaction: prevout points to the crediting tx.
    const auto crediting_hash = crediting_tx.hash(false);
    const transaction spending_tx
    {
        1u,
        inputs{{ point{ crediting_hash, 0u }, sig, max_uint32 }},
        outputs{{ 0u, script{} }},
        0u
    };

    // Set the prevout so the interpreter can access the script and value.
    spending_tx.inputs_ptr()->front()->prevout = to_shared(output{ 0u, pubkey });
    return spending_tx;
}

// ----------------------------------------------------------------------------
// Run one legacy (non-witness) vector through interpreter::connect().
// Returns true if the interpreter accepted the script pair.
// ----------------------------------------------------------------------------
static bool run_vector(const script_vector& v) NOEXCEPT
{
    using namespace system::chain;
    using namespace system::machine;

    const auto sig_bytes    = core_asm_to_bytes(v.sig_script);
    const auto pubkey_bytes = core_asm_to_bytes(v.pubkey_script);

    const script sig    { sig_bytes,    false };
    const script pubkey { pubkey_bytes, false };

    const auto tx = make_vector_tx(sig, pubkey);
    if (!tx.is_valid())
        return false;

    const context ctx{ flags_from_core_string(v.flags) };
    const auto result = interpreter<contiguous_stack>::connect(ctx, tx, 0u);
    return !result;
}

} // namespace script_vector_detail

#endif // LIBBITCOIN_SYSTEM_TEST_MACHINE_INTERPRETER_HPP

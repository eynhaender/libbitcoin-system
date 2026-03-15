# Spec-Driven Transaction Tests

## Overview

This document covers three test files that together exercise
`bc::system::chain::transaction` and `bc::machine::interpreter` against
externally-sourced, cryptographically-verified ground truth:

| File | Vectors | What it tests |
|------|---------|--------------|
| `test/chain/transaction_mainnet.cpp` | 18 assertions | Deserialization field values and hashes for three historic mainnet transactions |
| `test/chain/transaction_vectors.cpp` | 214 vectors | Full script validation via `tx.connect()` against Core's `tx_valid.json` and `tx_invalid.json` |
| `test/machine/interpreter.cpp` | 1,109 vectors | Script interpreter accept/reject against Core's `script_tests.json` |

All three files use the same principle: **the input is the oracle**, not any output
the library itself produces.


## Motivation: spec-driven vs. roundtrip tests

A **roundtrip test** calls library code to produce output, saves that output,
and then asserts that the same code produces the same output next time.  It
detects regressions but cannot detect that the library's behavior was wrong to
begin with.

A **spec-driven test** starts from an external artifact — a transaction whose
bytes, field values, and hash are published and independently verifiable — and
asserts that the library interprets those bytes correctly.  If the deserializer
has a systematic bug the roundtrip will hide it; the spec-driven test will
catch it.


## What is a test vector?

A **test vector** is a concrete input/output pair drawn from a specification or
a trusted reference implementation rather than from the code under test.  In
Bitcoin development the canonical source of test vectors is the Bitcoin Core
repository (`bitcoin/bitcoin`), which maintains JSON files in
`src/test/data/`.  The primary files are:

| File | Contents |
|------|----------|
| `script_tests.json` | Script interpreter vectors — valid and invalid entries combined in one file; valid entries carry `"OK"`, invalid entries carry a specific error name |
| `tx_valid.json` | Transaction-level validation vectors that must be accepted |
| `tx_invalid.json` | Transaction-level validation vectors that must be rejected |
| `sighash.json` | Sighash computation vectors: `(tx, script, input_index, hash_type) → expected_sighash` |

These vectors were authored alongside — and in many cases before — the
consensus rules they exercise, so they are suitable as an oracle against which
any compatible implementation must agree.

Using Core's vectors instead of constructing inputs by hand has two advantages:

1. **Correctness**: the bytes and expected outcomes were reviewed by Core
   contributors who understand the protocol specification.
2. **Coverage of edge cases**: Core's test data is specifically designed to
   exercise boundary conditions, malleability, encoding rules, and flags that
   a hand-crafted set would likely miss.


## Coverage status

| Vector file | In repo | Test fixture | Vectors run | Notes |
|-------------|---------|-------------|-------------|-------|
| `script_tests.json` | Yes | `test/machine/interpreter.cpp` | 1,086 of 1,109 | 23 witness entries skipped |
| `tx_valid.json` | Yes | `test/chain/transaction_vectors.cpp` | 121 of 121 | ~36 known false rejections |
| `tx_invalid.json` | Yes | `test/chain/transaction_vectors.cpp` | 93 of 93 | ~24 known false acceptances |
| `sighash.json` | Yes | — | 0 of 500 | API limitation (see below) |

All four files live in `test/vectors/`.


---

## test/chain/transaction_mainnet.cpp

### What it tests

18 assertions across three historic mainnet transactions, each getting six
test cases: valid parse, structure fields, input fields, output fields,
hash(es), and serialization round-trip.

### Transactions under test

#### 1. Genesis coinbase (block 0, 3 January 2009)

```
txid: 4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b
```

The coinbase of the genesis block is the simplest valid Bitcoin transaction:
one input with a null prevout (all-zero hash, index `0xffffffff`), one P2PK
output paying 50 BTC to Satoshi Nakamoto's uncompressed public key.  The
scriptSig contains the famous newspaper headline
_"The Times 03/Jan/2009 Chancellor on brink of second bailout for banks"_.

**Source**: the raw bytes are embedded verbatim in `src/settings.cpp` of this
library (and every Bitcoin client) as the statically-compiled genesis block.
The txid equals the genesis block's merkle root — a fact that can be verified
by SHA-256d over the stripped serialization.

**What the tests assert**:
- `is_valid()` — the bytes parse without error
- `version == 1`, `inputs == 1`, `outputs == 1`, `locktime == 0`
- `is_coinbase()` — prevout hash is null, index is `max_uint32`
- `!is_segregated()` — no witness marker/flag bytes
- `input.point().index() == 0xffffffff`
- `input.sequence() == 0xffffffff` (SEQUENCE_FINAL)
- `input.script().serialized_size(false) == 77` — the 77-byte scriptSig
- `output.value() == 5_000_000_000` — 50 BTC in satoshis
- `output.script().serialized_size(false) == 67` — P2PK (push-65 + pubkey + OP_CHECKSIG)
- `hash(false) == hash(true) == genesis_coinbase_txid` — non-segregated hashes are identical
- `to_data(false) == genesis_coinbase_data` — serialization round-trip

---

#### 2. Block-170 (Satoshi Nakamoto → Hal Finney, 12 January 2009)

```
txid: f4184fc596403b9d638783cf57adfe4c75c605f6356fbc91338530e9831e9e16
```

The first non-coinbase Bitcoin transaction ever broadcast on mainnet.  Satoshi
paid 10 BTC to Hal Finney and returned 40 BTC in change to himself.  Both
outputs use uncompressed P2PK scripts.

**Source**: raw hex from multiple block explorers; txid verified by SHA-256d.

**What the tests assert**:
- `is_valid()`
- `version == 1`, `inputs == 1`, `outputs == 2`, `locktime == 0`
- `!is_coinbase()`, `!is_segregated()`
- `input.point().hash() == block170_prevout_txid` — spends output 0 of the
  block-9 coinbase (`0437cd7f…`)
- `input.point().index() == 0`
- `input.sequence() == max_uint32`
- `outputs[0].value() == 1_000_000_000` — 10 BTC to Hal Finney
- `outputs[1].value() == 4_000_000_000` — 40 BTC change to Satoshi
- Both output scripts are 67 bytes (uncompressed P2PK)
- `hash(false) == hash(true) == block170_txid`
- Serialization round-trip

---

#### 3. Segwit P2WPKH spend (Bitcoin Core `tx_valid.json`)

```
txid  (stripped): b2ce556154e5ab22bec0a2f990b2b843f4f4085486c0d2cd82873685c0012004
wtxid (witness):  7944c8f36d682addda15124399bf954ec5d4b3a426e9d505a5f74a08644f0ebb
```

A minimal native P2WPKH spending transaction sourced from Bitcoin Core's
`tx_valid.json`.  The prevout is synthetic but the DER signature and compressed
public key are cryptographically valid; both hashes were verified by SHA-256d
over stripped and full serializations respectively.

The extended (segwit) wire format adds marker `0x00` + flag `0x01` after the
version field and appends a witness section before the locktime.

**What the tests assert**:
- `is_valid()` when parsed with `witness=true`
- `version == 1`, `inputs == 1`, `outputs == 1`, `locktime == 0`
- `!is_coinbase()`, `is_segregated()` — marker/flag bytes present
- `input.script().serialized_size(false) == 0` — empty scriptSig (native segwit)
- `witness.stack().size() == 2` — DER signature + compressed pubkey
- `witness.stack()[0]->size() == 72` — DER-encoded ECDSA signature
- `witness.stack()[1]->size() == 33` — compressed 33-byte public key
- `output.value() == 1000`, `output.script().serialized_size(false) == 25` (P2PKH)
- `hash(false) == segwit_txid`, `hash(true) == segwit_wtxid`
- `hash(false) != hash(true)` — hashes differ for segregated txs
- `to_data(true) == segwit_p2wpkh_data` — full witness round-trip
- `to_data(false).size() < to_data(true).size()` — stripped form is shorter


---

## test/chain/transaction_vectors.cpp

### What it tests

Full end-to-end script validation.  For each vector the fixture:

1. Deserializes the raw transaction hex with `witness=true`.
2. Looks up each tx input's prevout by `(txid, vout)` in the vector's prevout
   descriptor table and attaches it as a shared `output` object so the
   interpreter can read the script and value.
3. Builds a `context` from the vector's comma-separated flag string using the
   same flag-to-`chain::flags` mapping as the `script_tests.json` fixture.
4. Calls `tx.connect(ctx)`, which iterates all inputs and dispatches to
   `interpreter::connect` per input.

### Format of tx_valid.json and tx_invalid.json

```
[ [[txid, vout, scriptPubKey, ?amount], ...], "tx_hex", "flags" ]
```

- `txid` — prevout txid in display order (big-endian); reversed to internal
  order for the prevout lookup
- `vout` — prevout output index
- `scriptPubKey` — prevout output script in Bitcoin Core asm notation
- `amount` — prevout value in satoshis; present only for segwit vectors
- `tx_hex` — raw serialized transaction
- `flags` — comma-separated verification flags (same format as `script_tests.json`)

### Known deviations — tx_valid.json (false rejections, ~36 of 121)

**LOW_S / STRICTENC enforced without flag (~10 vectors)**
Libbitcoin enforces low-S signature encoding and strict-encoding
unconditionally regardless of which flags are active.  Vectors with
`flags=[DERSIG,LOW_S,STRICTENC]` or `flags=[LOW_S]` alone may be incorrectly
rejected because libbitcoin applies these checks even without the corresponding
BIP flag being set.

**NONE-flagged segwit transactions (~26 vectors, shared with tx_invalid)**
Transactions in the segwit extended format with `flags=[NONE]` are expected to
be accepted by Core: without the WITNESS flag, a native segwit output
(`OP_0 <hash>`) is treated as a simple stack push — non-empty and therefore
truthy.  Libbitcoin may reject these due to unconditional witness-path
evaluation or script assembly differences for version-0 witness programs.

### Known deviations — tx_invalid.json (false acceptances, ~24 of 93)

**BADTX — coinbase sentinel index (9 vectors)**
These transactions contain inputs with `index=0xffffffff` (the coinbase
indicator), making `is_coinbase()` return true.  `tx.connect()` short-circuits
and returns success for coinbase transactions.  Detecting that a non-coinbase
transaction is fraudulently using the coinbase marker requires chain context
that `connect()` does not have.

**CONST_SCRIPTCODE — OP_CODESEPARATOR subscript rule (12 vectors)**
Core enforces that the subscript committed to in the signature hash does not
carry an `OP_CODESEPARATOR`.  This flag has no libbitcoin equivalent.

**DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM (3 vectors)**
Core rejects spends of version-2..16 native segwit programs under this policy
flag.  Libbitcoin has no mapping for it.


---

## sighash.json — not currently wired

`test/vectors/sighash.json` (500 vectors) is fetched and present but has no
test fixture.  Each vector supplies `(tx_hex, script_hex, input_index,
hash_type) → expected_sighash` where `hash_type` is a signed 32-bit integer.

The Bitcoin legacy sighash algorithm serializes the full 4-byte `hash_type`
into the preimage (as a little-endian `int32`), so two vectors that differ only
in the high bytes of `hash_type` produce different hashes.  All 500 Core
vectors use 32-bit values whose high three bytes are non-zero.

Libbitcoin's `transaction::signature_hash()` accepts `uint8_t sighash_flags`,
which it zero-extends to 4 bytes in the preimage.  This means the high three
bytes are always zero, and no vector from Core's file can produce a matching
hash.  A fixture must wait for a `uint32_t` sighash API overload.


---

## API surface exercised

### chain::transaction (transaction_mainnet.cpp + transaction_vectors.cpp)

| Method | Purpose |
|--------|---------|
| `transaction(data_slice, bool witness)` | Deserialize from raw bytes |
| `is_valid()` | Confirm successful parse |
| `version()` | Wire version field |
| `inputs()` / `outputs()` | Input/output counts |
| `locktime()` | Locktime field |
| `is_coinbase()` | Null prevout detection |
| `is_segregated()` | Witness marker/flag detection |
| `inputs_ptr()` / `outputs_ptr()` | Access input/output collections |
| `input::point().hash()` | Prevout txid (internal byte order) |
| `input::point().index()` | Prevout output index |
| `input::sequence()` | Input sequence number |
| `input::script().serialized_size(false)` | ScriptSig byte length |
| `input::witness().stack()` | Witness item vector |
| `output::value()` | Satoshi amount |
| `output::script().serialized_size(false)` | Output script byte length |
| `hash(bool witness)` | txid (`false`) or wtxid (`true`) |
| `to_data(bool witness)` | Serialize to bytes |
| `connect(context)` | Full script validation for all inputs |


---

## How to run

```sh
# Build
cmake --build obj/nix-gnu-debug-static --target libbitcoin-system-test

# Deserialization field tests (18 assertions, no failures expected)
obj/nix-gnu-debug-static/libbitcoin-system-test \
    --run_test=transaction_mainnet_tests

# Core tx_valid / tx_invalid vector tests (214 vectors, ~60 known failures)
obj/nix-gnu-debug-static/libbitcoin-system-test \
    --run_test=transaction_vectors_tests

# Script interpreter vector tests (1,109 vectors, ~114 known failures)
obj/nix-gnu-debug-static/libbitcoin-system-test \
    --run_test=interpreter_tests

# All tests
obj/nix-gnu-debug-static/libbitcoin-system-test
```

The `~60` and `~114` failures in the vector suites are expected and documented;
they represent known gaps in libbitcoin's flag enforcement, not regressions.
A change that increases the failure count signals a regression; one that
decreases it signals a gap being closed.

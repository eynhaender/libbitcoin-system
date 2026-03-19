# PR: Spec-driven transaction and script validation tests

## Summary

Adds spec-driven test coverage for `chain::transaction` deserialization and
`machine::interpreter` script validation using Bitcoin Core's published test
vector files as the oracle.  Tests start from externally-verifiable raw bytes
and assert exact outcomes; they cannot be fooled by a library that consistently
produces wrong output.

Three test files, four Core vector files:

| Test file | Vector file | Vectors | Suite name |
|-----------|-------------|---------|------------|
| `test/chain/transaction_mainnet.cpp` | — (historic mainnet txs) | 18 assertions | `transaction_mainnet_tests` |
| `test/chain/transaction_vectors.cpp` | `tx_valid.json` + `tx_invalid.json` | 214 | `transaction_vectors_tests` |
| `test/machine/interpreter.cpp` | `script_tests.json` | 1,109 | `interpreter_tests` |

`sighash.json` is fetched and present in `test/vectors/` but has no fixture
yet — see _Known limitations_ below.

## transaction_mainnet.cpp — 18 assertions over 3 historic transactions

| Transaction | txid |
|-------------|------|
| Genesis coinbase (block 0) | `4a5e1e4b…` |
| Block-170 (Satoshi → Hal Finney) | `f4184fc5…` |
| Segwit P2WPKH spend (Core `tx_valid.json`) | `b2ce5561…` / wtxid `7944c8f3…` |

Each transaction gets six test cases: valid parse, structure fields, input
fields, output fields, hash(es), and serialization round-trip.  All 18 pass
with no failures.

## transaction_vectors.cpp — 214 Core tx validation vectors

For each vector the fixture deserializes the raw transaction, attaches the
prevout scripts and amounts from the vector's UTXO table, builds a `context`
from the flag string, and calls `tx.connect(ctx)`.

**tx_valid.json** (121 vectors — must be accepted):
- ~85 pass
- ~36 known false rejections: libbitcoin enforces `LOW_S`/`STRICTENC`
  unconditionally without the flag; NONE-flagged segwit transactions rejected
  when they should be treated as simple stack pushes

**tx_invalid.json** (93 vectors — must be rejected):
- ~69 pass
- ~24 known false acceptances: `BADTX` coinbase-sentinel inputs short-circuit
  `connect()`; `CONST_SCRIPTCODE` and `DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM`
  flags have no libbitcoin equivalent

All failures use `BOOST_CHECK` (non-fatal) with per-vector labels and are
documented in the test file.  A future change that reduces the count closes
a known gap; one that increases it signals a regression.

## script_tests.json — 1,109 Core script interpreter vectors (pre-existing)

Already in `test/machine/interpreter.cpp`.  Non-witness entries are fully
exercised; the 23 witness entries are skipped pending a witness-aware
crediting/spending fixture.  ~114 known deviations documented in the test file.

## Known limitations

**sighash.json not wired**: all 500 Core vectors use 32-bit `hash_type` values
with non-zero high bytes.  `transaction::signature_hash()` accepts `uint8_t`,
which zero-extends to 4 bytes in the preimage, making every vector produce a
wrong hash.  Coverage requires a `uint32_t sighash_flags` API overload.

**Witness script_tests.json entries not run**: the 23 witness entries in
`script_tests.json` need a witness-stack-aware crediting/spending fixture not
yet implemented.

## Files changed

- `test/vectors/tx_valid.json` — new: Bitcoin Core tx validation vectors (must accept); source: `bitcoin/bitcoin` `src/test/data/tx_valid.json`
- `test/vectors/tx_invalid.json` — new: Bitcoin Core tx validation vectors (must reject); source: `bitcoin/bitcoin` `src/test/data/tx_invalid.json`
- `test/vectors/sighash.json` — new: Bitcoin Core sighash vectors (no fixture yet); source: `bitcoin/bitcoin` `src/test/data/sighash.json`
- `test/chain/transaction_mainnet.cpp` — new: 18 field-level deserialization tests
- `test/chain/transaction_vectors.hpp` — new: JSON loader + prevout matcher shared by tx_valid/invalid fixture
- `test/chain/transaction_vectors.cpp` — new: 214-vector tx connect() fixture
- `builds/cmake/CMakeLists.txt` — adds the two new `.cpp` files to the test build target
- `doc/transaction-mainnet-tests.md` — updated: full coverage of all three test files
- `doc/known-test-vector-deviations.md` — new: categorised list of all ~174 known deviations with root causes and priorities

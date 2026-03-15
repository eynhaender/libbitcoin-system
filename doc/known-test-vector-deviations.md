# Known Test Vector Deviations

This document lists all known gaps between libbitcoin's script/transaction
validation and Bitcoin Core's reference behaviour, as surfaced by the
Core test vector suites (`script_tests.json`, `tx_valid.json`,
`tx_invalid.json`).

The ~114 and ~60 failure counts are **expected and documented** — they are
not regressions. A change that *increases* the count signals a regression;
one that *decreases* it signals a gap being closed.

---

## `script_tests.json` — ~114 failures (interpreter)

### libbitcoin too permissive (accepts what it should reject)

| Flag / Category | Count | What libbitcoin is missing |
|---|---|---|
| `SCRIPTNUM` | 54 | Minimal encoding for script integers not enforced |
| `MINIMALDATA` | 21 | Minimal push encoding not enforced |
| `DISCOURAGE_UPGRADABLE_NOPS` | 10 | Policy flag for reserved NOPs not implemented |
| `INVALID_STACK_OPERATION` | 5 | Some stack underflows not caught |
| `PUBKEYTYPE` | 4 | Invalid pubkey types not rejected |
| `SIG_DER` | 4 | Non-DER signature accepted without `DERSIG` flag |
| `NULLFAIL` | 3 | `CHECKSIG` must leave empty stack on failure |
| `CLEANSTACK` | 2 | Stack must contain exactly one element after execution |
| `SIG_PUSHONLY` | 2 | scriptSig must contain push operations only |
| `SIG_NULLDUMMY` | 2 | `CHECKMULTISIG` dummy element must be empty |
| `SIG_HASHTYPE` | 2 | Invalid sighash type not detected |
| `SIG_COUNT` | 2 | Signature count exceeding `OP_N` not rejected |
| `PUBKEY_COUNT` | 2 | Pubkey count exceeding `OP_N` not rejected |
| `SIG_HIGH_S` | 1 | High-S not enforced without `LOW_S` flag |

### libbitcoin too strict (rejects what it should accept)

| Category | Count | Root cause |
|---|---|---|
| Unconditional DER | 8 | Strict-DER always enforced, even without `DERSIG`/`STRICTENC` flag |

---

## `tx_valid.json` + `tx_invalid.json` — ~60 failures (transaction)

### Too strict — ~36 false rejections (tx_valid)

| Category | Count | Root cause |
|---|---|---|
| `LOW_S` / `STRICTENC` unconditional | ~10 | Flag-gated checks not implemented; libbitcoin enforces these regardless of active flags |
| Segwit tx with `flags=NONE` | ~26 | Witness path always evaluated; without `WITNESS` flag, `OP_0 <hash>` should be treated as a plain push (non-empty = truthy) |

### Too permissive — ~24 false acceptances (tx_invalid)

| Category | Count | Root cause |
|---|---|---|
| `BADTX` coinbase sentinel | ~9 | `connect()` short-circuits for `is_coinbase()`; fraudulent use of `index=0xffffffff` requires chain context that `connect()` does not have |
| `CONST_SCRIPTCODE` | ~12 | `OP_CODESEPARATOR` subscript rule for sighash not implemented |
| `DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM` | ~3 | Policy flag for version-2..16 native segwit programs not mapped |

---

## Root causes and prioritization

Most failures trace back to five core problems:

1. **Flag-gating absent** — `DER`, `LOW_S`, `STRICTENC` are enforced
   unconditionally instead of conditionally on the active flag set.
   High-risk change; deep in the interpreter.

2. **Minimal encoding** (`SCRIPTNUM` + `MINIMALDATA`, **75 failures**) —
   the single largest block. Purely additive checks with no existing
   behaviour to break.

3. **Policy flags not mapped** (`CLEANSTACK`, `NULLFAIL`, `SIG_PUSHONLY`,
   `SIG_NULLDUMMY`, `SIG_HASHTYPE`, `SIG_COUNT`, `PUBKEY_COUNT`,
   `PUBKEYTYPE`) — each requires a flag mapping entry and a targeted
   enforcement check in the interpreter.

4. **Coinbase sentinel in `connect()`** — detecting that a non-first-in-block
   transaction is fraudulently claiming the coinbase marker requires either
   passing chain context into `connect()` or explicit sentinel validation
   before dispatch.

5. **Witness evaluation without `WITNESS` flag** — segwit program recognition
   must be conditional on the `WITNESS` flag being active so that
   version-0 programs evaluate as plain pushes when the flag is absent.

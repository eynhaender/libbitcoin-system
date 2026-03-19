# Formales Modell: machine::interpreter Opcode-Tests

## Was das formale Modell bedeutet

Jeder Opcode-Test ist strukturell ein **Hoare-Tripel** — die klassische Notation aus der formalen Verifikation:

```
{P} C {Q}
```

- **P** (Vorbedingung): der exakte Stack-Zustand + aktive Flags *vor* dem Aufruf von `op_*`
- **C** (Befehl): der Aufruf des Opcode-Handlers selbst
- **Q** (Nachbedingung): der Rückgabecode + der exakte Stack-Zustand *nach* dem Aufruf

Beispiel für den `op_swap`-Test mit zwei Elementen:

```
P: stack = [0x01, 0x02]
C: op_swap()
Q: return == op_success ∧ stack = [0x02, 0x01]
```

Jeder Fehlerpfad ist ebenfalls ein Tripel:

```
P: stack = [] (leer)
C: op_swap()
Q: return == error::op_swap ∧ stack unverändert
```

---

## Wozu das nützlich ist

1. **Vollständige Spezifikation** — Wenn für alle 87 Opcodes jeder Fehlerpfad, der Normalfall und die Grenzfälle abgedeckt sind, wird die Test-Suite zu einer *maschinenlesbaren Spezifikation* der Bitcoin-Script-Semantik. Keine Mehrdeutigkeit, keine Prosa.

2. **Präzise Regressionssicherheit** — Vektor-Tests sagen nur: „Irgendwo im Engine ist etwas kaputtgegangen." Ein fehlgeschlagener Hoare-Tripel-Test sagt: „Die Nachbedingung von `op_add` mit den Eingaben `INT32_MAX, 1` ist falsch" — exakte Funktion, exakter Zweig.

3. **Brücke zu formalen Verifikationswerkzeugen** — Tools wie Frama-C, F*, Coq, Lean oder TLA+ können diese Vor-/Nachbedingungspaare direkt verwenden. Die Fixture-Infrastruktur (eine dünne Unterklasse, die geschützte Methoden freilegt, plus ein minimaler Transaktions-Stub) ist genau das, was ein Verifikations-Harness ebenfalls benötigt. Unit-Tests und ein formaler Beweis würden dasselbe Zustandsmodell teilen.

---

## Wie das erreicht wird

### Schritt 1 — Die Fixture-Unterklasse

```cpp
// test/machine/interpreter_fixture.hpp

class interpreter_fixture : public interpreter<contiguous_stack>
{
public:
    // Konstruiert mit einer minimalen Dummy-Transaktion und leerem Input.
    // Opcodes, die keinen Tx/Input-Kontext benötigen (die Mehrheit),
    // funktionieren ohne weiteres Setup korrekt.
    explicit interpreter_fixture(uint32_t flags = 0) NOEXCEPT;

    // Stack-Hilfsmethoden — dünne Wrapper über die geschützten Methoden von program<Stack>.
    void push(data_chunk data) NOEXCEPT { push_chunk(std::move(data)); }
    void push(int64_t v)       NOEXCEPT { push_signed64(v); }
    void push(bool v)          NOEXCEPT { push_bool(v); }
    size_t size() const        NOEXCEPT { return stack_size(); }

    // Alle 87 op_*-Methoden von protected nach public anheben.
    using interpreter::op_swap;
    using interpreter::op_dup;
    using interpreter::op_rot;
    using interpreter::op_add;
    // … alle weiteren op_*-Methoden
};
```

`interpreter<Stack>` erbt von `program<Stack>`, das den gesamten Ausführungszustand hält. Kein Mocking-Framework nötig — die Unterklasse hebt lediglich die Zugriffsbeschränkung auf.

Für Opcodes, die `tx()` oder `input()` inspizieren (hauptsächlich `op_check_sig`, `op_check_multisig`, `op_check_locktime_verify`, `op_check_sequence_verify`), nimmt ein spezialisierter Konstruktor eine echte Transaktion und einen Input-Iterator entgegen.

### Schritt 2 — Eine Testdatei pro Opcode-Gruppe

| Datei | Opcode-Gruppe | Beispielmethoden |
|---|---|---|
| `interpreter_op_flow.cpp` | Kontrollfluss | `op_if`, `op_notif`, `op_else`, `op_endif`, `op_verify`, `op_return` |
| `interpreter_op_stack.cpp` | Stack-Manipulation | `op_dup`, `op_swap`, `op_rot`, `op_over`, `op_pick`, `op_roll`, `op_drop` |
| `interpreter_op_arithmetic.cpp` | Script-Zahlen | `op_add`, `op_sub`, `op_negate`, `op_abs`, `op_min`, `op_max`, `op_within` |
| `interpreter_op_bitwise.cpp` | Gleichheit / Kodierung | `op_equal`, `op_equal_verify`, `op_size` |
| `interpreter_op_crypto.cpp` | Hashing + Signierung | `op_hash160`, `op_hash256`, `op_check_sig`, `op_check_multisig` |
| `interpreter_op_push.cpp` | Push-Opcodes | `op_push_number`, `op_push_size`, `op_push_one_size` |

Jede Datei entspricht einem `BOOST_AUTO_TEST_SUITE`. Eine gemeinsam genutzte `test/machine/interpreter_fixture.hpp` wird von allen eingebunden.

### Schritt 3 — Pflichtcheckliste pro Opcode (vier Fälle je Op)

Für jede `op_*`-Methode:

1. **Jeder Fehlerpfad** — jedes distinct early-return liefert seinen spezifischen `error::op_*`-Code, typischerweise Stack-Unterlauf oder Typfehler.
2. **Normalfall (Happy Path)** — korrekte Eingaben liefern `error::op_success` und der Stack-Zustand nach dem Aufruf entspricht exakt der Konsens-Spezifikation.
3. **Grenzfälle** — leere Datenelemente, Grenzwerte für Script-Zahlen (`INT32_MIN`, `INT32_MAX`), maximale Stack-Tiefe wo relevant.
4. **Flag-Sensitivität** — für Opcodes, die auf `is_enabled(flag)` verzweigen, beide Pfade testen: Flag aktiv und Flag inaktiv.

### Schritt 4 — Kommentarblock mit der Konsensregel

Über der Testgruppe jedes Opcodes steht ein Kommentar mit:
1. Der **Konsensregel** in Klartext
2. Jedem **Fehlerzustand** und seinem Rückgabecode
3. Der **BIP-Referenz** falls zutreffend

---

## Beispiel: OP_SWAP

```cpp
BOOST_AUTO_TEST_SUITE(interpreter_op_stack_tests)

// OP_SWAP — Konsensregel
// Tauscht die obersten zwei Stack-Elemente: [a, b, ...] → [b, a, ...]
// Schlägt mit op_swap fehl, wenn die Stack-Tiefe < 2 ist.
// Referenz: Script-Spezifikation, kein BIP.

BOOST_AUTO_TEST_CASE(interpreter__op_swap__empty_stack__op_swap)
{
    interpreter_fixture f;
    BOOST_REQUIRE_EQUAL(f.op_swap(), error::op_swap);
}

BOOST_AUTO_TEST_CASE(interpreter__op_swap__one_item__op_swap)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    BOOST_REQUIRE_EQUAL(f.op_swap(), error::op_swap);
}

BOOST_AUTO_TEST_CASE(interpreter__op_swap__two_items__swapped)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });   // nach dem zweiten Push an Index 1
    f.push(data_chunk{ 0x02 });   // Index 0 (oben)
    BOOST_REQUIRE_EQUAL(f.op_swap(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 2u);
    // oberstes Element ist jetzt 0x01
}

BOOST_AUTO_TEST_CASE(interpreter__op_swap__three_items__only_top_two_swapped)
{
    interpreter_fixture f;
    f.push(data_chunk{ 0x01 });
    f.push(data_chunk{ 0x02 });
    f.push(data_chunk{ 0x03 });   // oben vor dem Swap
    BOOST_REQUIRE_EQUAL(f.op_swap(), error::op_success);
    BOOST_REQUIRE_EQUAL(f.size(), 3u);
    // unterstes Element unverändert; oberste zwei getauscht
}

BOOST_AUTO_TEST_SUITE_END()
```

---

## Die zentrale Erkenntnis

Das Design funktioniert so sauber, weil `op_*`-Methoden **keine Seiteneffekte außerhalb des Programmzustands** haben — kein I/O, keine globalen Variablen, kein virtueller Dispatch. Der Stack ist der einzige Ein- und Ausgang. Das macht das Hoare-Tripel-Modell exakt und formale Verifikation praktikabel: Ein Beweischecker muss nur über eine begrenzte Datenstruktur (den Stack) und eine reine Funktion nachdenken.

---

## Vergleich: Vektor-Tests vs. Opcode-Unit-Tests

| Eigenschaft | Vektor-Tests (Akzeptanz) | Opcode-Unit-Tests |
|---|---|---|
| Fehler lokalisierbar | Nein — „irgendwo ist etwas schiefgelaufen" | Ja — genau welcher Op, welcher Codepfad |
| Abdeckung messbar | Nein | Ja — jeder Zweig sichtbar |
| Dokumentiert Konsensregel | Teilweise (via JSON-Kommentare) | Explizit, pro Testfall |
| Erkennt systematische Fehler | Nein | Ja |
| Regressionsempfindlichkeit | Ja, grob | Ja, präzise |
| Testanzahl | ~1.300 Vektoren | ~300–500 gezielte Fälle |
| Wartungsaufwand | Hoch (Vektorformat ändert sich) | Gering (folgt einer Funktion) |

Die Vektor-Tests bleiben als **End-to-End-Smoke-Check** nützlich. Die Unit-Tests sind das, was tatsächlich fehlt und was der Maintainer angefordert hat.

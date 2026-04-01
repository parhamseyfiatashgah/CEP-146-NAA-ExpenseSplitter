# CEP-146-NAA – Expense Splitter

**CEP 146-NAA | Lab – Expense Splitter**  
**Student:** Parham Seyfi Atasgah

---

## Description

A terminal-based C program that splits bills equally or by custom weights,
tracks expenses in a persistent ledger, saves roommate names between sessions,
and displays a summary report.

## Files

| File | Purpose |
|------|----------|
| `main.c` | Entry point and main menu loop |
| `ledger.c / ledger.h` | Append, print, and summarize ledger.txt |
| `people.c / people.h` | Person struct, name collection, share calculation |
| `input.c / input.h` | Safe input helpers (strings, doubles, menu choices) |
| `names_file.c / names_file.h` | Persistent roommate names (roommates.txt) |
| `Makefile` | Build, clean, and run targets |

## Build & Run

```bash
make          # compile → expense_splitter
make run      # compile and launch
make clean    # remove binary
```

Requires GCC 4.8+ with C11 support (CentOS 7 / Linux).

## Lab Report

See `ExpenseSplitter_CEP146_ParhamSeyfiAtasgah_Report.pdf` for the full course lab report.

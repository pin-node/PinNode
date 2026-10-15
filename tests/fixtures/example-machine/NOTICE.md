# Fixture: RetroPinUpgrade ExampleMachine

`ExampleMachineMEGA.ino`, `OperatorMenus.h`, `RPU.h` and `RPU_Config.h` are copied unmodified from
https://github.com/RetroPinUpgrade/ExampleMachine at commit `4b6083b793`
(fetched 2026-09-30). They're licensed under **GPL-3.0** by their authors.
They're here only as test input for `pinnode-gen scan`.

The `.ino` files are stored as `.ino.txt` so Arduino tools don't treat them as sketches; the tests
restore the names (tests/_fixtures.py).

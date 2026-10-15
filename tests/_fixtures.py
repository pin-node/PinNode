"""The test fixtures, ready to use: a copy of tests/fixtures with each `*.ino.txt` named `*.ino` again.

They're stored as .ino.txt so Arduino's linter doesn't take them for sketches in the library
(rule LD003: sketches only in examples/)."""
import shutil
import tempfile
from pathlib import Path

_SRC = Path(__file__).resolve().parent / "fixtures"
FIX = Path(tempfile.mkdtemp(prefix="pinnode-fixtures-")) / "fixtures"
shutil.copytree(_SRC, FIX)
for _f in FIX.rglob("*.ino.txt"):
    _f.rename(_f.with_name(_f.name[:-len(".txt")]))

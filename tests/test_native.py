"""Builds and runs the native PinNodeCtrl tests (tests/native) with the host C++ compiler."""

import shutil
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
NATIVE = ROOT / "tests" / "native"


@pytest.mark.skipif(not (shutil.which("c++") or shutil.which("clang++") or shutil.which("g++")),
                    reason="no host C++ compiler")
def test_pinnodectrl_native(tmp_path):
    cxx = shutil.which("c++") or shutil.which("clang++") or shutil.which("g++")
    exe = tmp_path / "test_pinnodectrl"
    cmd = [cxx, "-std=c++17", "-Wall", "-Wextra", "-Wno-missing-field-initializers",
           "-DPINNODE_HOST_BUILD", f"-I{NATIVE / 'shim'}", f"-I{ROOT / 'src'}",
           str(NATIVE / "test_pinnodectrl.cpp"), str(ROOT / "src" / "PinNodeCtrl.cpp"),
           "-o", str(exe)]
    build = subprocess.run(cmd, capture_output=True, text=True)
    assert build.returncode == 0, build.stderr
    run = subprocess.run([str(exe)], capture_output=True, text=True, timeout=60)
    assert run.returncode == 0, run.stdout + run.stderr


def _cxx():
    return shutil.which("c++") or shutil.which("clang++") or shutil.which("g++")


def _build_run(tmp_path, name, sources, defines):
    exe = tmp_path / name
    cmd = [_cxx(), "-std=c++17", "-Wall", "-Wextra", "-Wno-missing-field-initializers",
           *[f"-D{d}" for d in defines], f"-I{NATIVE / 'shim'}", f"-I{ROOT / 'src'}",
           *map(str, sources), "-o", str(exe)]
    build = subprocess.run(cmd, capture_output=True, text=True)
    assert build.returncode == 0, build.stderr
    run = subprocess.run([str(exe)], capture_output=True, text=True, timeout=60)
    assert run.returncode == 0, run.stdout + run.stderr


@pytest.mark.skipif(not _cxx(), reason="no host C++ compiler")
def test_rpulink_dormancy_native(tmp_path):
    sources = [NATIVE / "test_rpulink.cpp", ROOT / "src" / "RPULink.cpp"]
    _build_run(tmp_path, "test_rpulink", sources, ["PINNODE_HOST_BUILD"])


@pytest.mark.skipif(not _cxx(), reason="no host C++ compiler")
def test_disabled_links_no_library(tmp_path):
    # no library sources: any call that isn't stubbed fails to link
    _build_run(tmp_path, "test_disabled", [NATIVE / "test_disabled.cpp"], ["PINNODE_HOST_BUILD"])


@pytest.mark.skipif(not _cxx(), reason="no host C++ compiler")
def test_operator_menu_helpers_native(tmp_path):
    # one self-test press in attract opens the menu, two mid-game; the blank page's idle and phone exits
    _build_run(tmp_path, "test_menu", [NATIVE / "test_menu.cpp"], ["PINNODE_HOST_BUILD"])


@pytest.mark.skipif(not _cxx(), reason="no host C++ compiler")
def test_phone_tests_native(tmp_path):
    # PinNode_Tests: diagnostics from attract only, lamp/display/coil/switch tests, clear audits, exit
    _build_run(tmp_path, "test_phone_tests", [NATIVE / "test_phone_tests.cpp"], ["PINNODE_HOST_BUILD"])

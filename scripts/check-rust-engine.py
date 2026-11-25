"""Build and exercise the actual C++/Rust ABI against captured legacy typing traces."""
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
cargo = shutil.which("cargo") or str(Path.home() / ".cargo/bin/cargo")
fixtures = (ROOT / "Sources/OpenKey/engine-rust/tests/reference.tsv").read_text().splitlines()
inputs = ["\t".join(line.split("\t")[:2]) for line in fixtures]
with tempfile.TemporaryDirectory(prefix="openkey-abi-") as tmp:
    target = Path(tmp) / "rust"
    subprocess.run([cargo, "build", "--locked", "--release", "--manifest-path", "Sources/OpenKey/engine-rust/Cargo.toml", "--target-dir", str(target)], cwd=ROOT, check=True)
    binary = str(Path(tmp) / "bridge-test")
    subprocess.run(["clang++", "-std=c++17", "-Wno-deprecated-declarations", "-DOPENKEY_FIXTURE_TEST", "scripts/engine-fixtures.cpp", "Sources/OpenKey/engine-rust/RustBridge.cpp", str(target / "release/libopenkey_engine.a"), "-framework", "Security", "-framework", "CoreFoundation", "-liconv", "-o", binary], cwd=ROOT, check=True)
    output = subprocess.run([binary], input="\n".join(inputs) + "\n", text=True, capture_output=True, check=True).stdout.splitlines()
    if len(output) != len(fixtures):
        raise SystemExit(f"Expected {len(fixtures)} traces, received {len(output)}")
    failures = [i for i, (expected, actual) in enumerate(zip(fixtures, output)) if expected != actual]
    if failures:
        for i in failures[:10]:
            print(f"Mismatch at fixture {i+1}: {inputs[i]}")
        raise SystemExit(f"{len(failures)} native bridge mismatches")
print(f"Native C++/Rust bridge: {len(fixtures)} reference traces matched")

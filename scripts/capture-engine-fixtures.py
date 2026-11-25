"""Regenerate reference traces from the unchanged C++ engine (macOS toolchain)."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
words = "a as af ar ax aj aa aas aaf aar aax aaj aw aws awf awr awx awj dd ddd ee ees oos ow ows uw uws uow uows uowng thuw thuow thuown ddawng tieengs tieesng vieetj Vieetj Tieengs hieeur bieets nguowif nguwowif nguyeexn nguyexn quyeenf quys quaf quas quoccs quyetts gi gin gif gias giois giuos y uus wow www w ] [ [[ ]] z az asz ass ddass thoongf hoas hoaf hoar hoax hoaj huys huynhf uys oai oais uay uays oaif aaaa aww ooo uooo aeois a123 samsung viettel openkey hello world http://google.com xin chao ban how are you".split()
words += ["xin chaof", "Tooi laf nguwowif Vieetj Nam.", "tooi<is", "as<af", "vieetj <<<as", "hello  <<as", "as|as", "as^as", "nguwowif<<is", "vieetj,", "Tooi. ban", "aas<ss", "aa<<a", "hoas<<<af", "dddd", "ddas"]
words += "a1 a2 a3 a4 a5 a6 a61 a8 a81 d9 d99 e6 o6 o7 u7 u7o7ng tie61ng Vie65t ngu7o72i qua1 hoa2 huy3".split()
cases = [f"{mode} {code} {modern} 0\t{word}" for mode in range(4) for code in range(5) for modern in range(2) for word in words]
for profile in [1, 2, 4, 8, 16, 24, 32, 64]:
    cases += [f"0 0 0 {profile}\t{word}" for word in words + "fan jang wen vag vinh tt cc gg kk nn qq pp".split()]
for mode in range(4):
    for code in range(5):
        for profile in [128, 384]:
            cases += [f"{mode} {code} 0 {profile}\t{word}" for word in ["btw ", "btw,", "btw.", "btw "+"btw ", "ms ", "Ms ", "MS ", "ddc ", "btw< ", "btw| ", "btw^ "]]
cases += ["0 0 0 0\t" + "a" * 90 + "s"]
with tempfile.TemporaryDirectory() as tmp:
    binary = str(Path(tmp) / "reference")
    sources = ["Macro", "SmartSwitchKey", "Vietnamese", "ConvertTool"]
    subprocess.run(["clang++", "-std=c++17", "-Wno-deprecated-declarations", "-DOPENKEY_REFERENCE", "scripts/engine-fixtures.cpp", *[f"Sources/OpenKey/engine/{x}.cpp" for x in sources], "-o", binary], cwd=ROOT, check=True)
    result = subprocess.run([binary], input="\n".join(cases) + "\n", text=True, capture_output=True, check=True)
    (ROOT / "Sources/OpenKey/engine-rust/tests/reference.tsv").write_text(result.stdout)
print(f"Captured {len(cases)} independent reference traces")

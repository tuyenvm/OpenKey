use super::{engine::Engine, ffi::*, keys::*, tables::*, text::*, Config};

fn config(mode: i32, code: i32, modern: i32, profile: i32) -> Config {
    Config {
        input_type: mode,
        code_table: code,
        modern,
        spelling: i32::from(profile & 1 == 0),
        restore_invalid: i32::from(profile & 2 != 0),
        quick_telex: i32::from(profile & 4 != 0),
        quick_start: i32::from(profile & 8 != 0),
        quick_end: i32::from(profile & 16 != 0),
        allow_consonants: i32::from(profile & 32 != 0),
        upper_first: i32::from(profile & 64 != 0),
        macros: i32::from(profile & 128 != 0),
        auto_caps_macro: i32::from(profile & 256 != 0),
        ..Config::default()
    }
}
fn rendered(x: u32) -> u32 {
    if x & (CHAR_CODE_MASK | PURE_CHARACTER_MASK) != 0 {
        x & 0xffff
    } else {
        key_character(x)
    }
}
fn key(c: char) -> (u32, u32) {
    if c == '<' {
        return (KEY_DELETE, 0);
    }
    if c == '^' {
        return (KEY_LEFT, 0);
    }
    for k in 0..128 {
        if key_character(k) == c as u32 {
            return (k, 0);
        }
        if key_character(k | CAPS_MASK) == c as u32 {
            return (k, 1);
        }
    }
    panic!("unknown fixture character: {c}");
}
#[test]
fn cpp_reference_traces() {
    for (line_no, line) in include_str!("../tests/reference.tsv").lines().enumerate() {
        let mut fields = line.split('\t');
        let options = fields.next().unwrap();
        let input = fields.next().unwrap();
        let expected = fields.next().unwrap();
        let cfg: Vec<i32> = options
            .split_whitespace()
            .map(|x| x.parse().unwrap())
            .collect();
        let mut engine = Engine::default();
        engine.cfg = config(cfg[0], cfg[1], cfg[2], cfg[3]);
        engine.reset();
        engine.event(1, 2, 0, 0, false);
        engine.new_session();
        engine.macros.add("btw", "by the way");
        engine.macros.add("ms", "một giây 😀");
        engine.macros.add("đc", "địa chỉ");
        let mut output = Vec::new();
        let mut trace = String::new();
        for c in input.chars() {
            if c == '|' {
                engine.event(1, 2, 0, 0, false);
                trace.push_str("|;");
                continue;
            }
            let (key, caps) = key(c);
            engine.event(0, 0, key, caps, false);
            let h = &engine.hook;
            if h.code == 0 {
                if key == KEY_DELETE {
                    output.pop();
                } else if key != KEY_LEFT {
                    output.push(rendered(key | if caps != 0 { CAPS_MASK } else { 0 }));
                }
            } else {
                for _ in 0..h.backspaces {
                    output.pop();
                }
                if h.code == 4 {
                    output.extend(h.macro_data.iter().map(|&x| rendered(x)));
                    output.push(rendered(key | if caps != 0 { CAPS_MASK } else { 0 }));
                } else {
                    for i in (0..h.count as usize).rev() {
                        output.push(rendered(h.chars[i]));
                    }
                }
                if (h.code == 3 || h.code == 5) && key != KEY_LEFT {
                    output.push(rendered(key | if caps != 0 { CAPS_MASK } else { 0 }));
                }
                if h.code == 5 {
                    engine.new_session();
                }
            }
            for x in &output {
                trace.push_str(&format!("{x},"));
            }
            trace.push(';');
        }
        assert_eq!(
            trace,
            expected,
            "fixture {}: {options} {input:?}",
            line_no + 1
        );
    }
}
#[test]
fn macro_and_app_data_roundtrip_and_reject_truncation() {
    let mut macros = Macros::default();
    assert!(macros.add("ms", "một giây 😀"));
    assert!(macros.add("đc", "địa chỉ"));
    let bytes = macros.save(0);
    for end in 0..bytes.len() {
        let mut copy = Macros::default();
        copy.add("keep", "giữ");
        assert!(!copy.load(&bytes[..end]));
        assert!(copy.0.contains_key("keep"));
    }
    let mut copy = Macros::default();
    assert!(copy.load(&bytes));
    assert_eq!(copy.0, macros.0);
    assert_eq!(
        copy.find(&encode("ms", 0), 0, false),
        Some(encode("một giây 😀", 0))
    );
    assert_eq!(
        copy.find(&encode("MS", 0), 0, true),
        Some(encode("MỘT GIÂY 😀", 0))
    );
    assert_eq!(
        copy.find(&encode("Ms", 0), 0, true),
        Some(encode("Một giây 😀", 0))
    );
    let apps = std::collections::BTreeMap::from([
        ("com.apple.Terminal".into(), 1),
        ("com.apple.Notes".into(), 9),
    ]);
    let bytes = save_apps(&apps);
    assert_eq!(load_apps(&bytes), Some(apps));
    for end in 0..bytes.len() {
        assert!(load_apps(&bytes[..end]).is_none());
    }
    assert!(!copy.add(&"a".repeat(256), "too long"));
    assert!(!copy.import("broken input", false));
    assert_eq!(copy.0, macros.0);
    let mut imported = Macros::default();
    assert!(imported.import(&copy.export(), false));
    assert_eq!(copy.0, imported.0);
}
#[test]
fn encoding_roundtrips_and_preserves_case_and_emoji() {
    let text = "Tiếng Việt — ĐẶNG 😀";
    for code in [0, 2, 3, 4] {
        assert_eq!(convert(&convert(text, 0, code, 0), code, 0, 0), text);
    }
    assert_eq!(convert(text, 0, 0, 0), text);
    assert_eq!(convert("Tiếng Việt", 0, 0, 16), "Tieng Viet");
    assert_eq!(convert("Tiếng Việt", 0, 0, 1), "TIẾNG VIỆT");
    assert_eq!(convert("Tiếng Việt", 0, 0, 2), "tiếng việt");
    for (key, values) in CODE_0 {
        for (i, &value) in values.iter().enumerate() {
            if value != 0 {
                for (code, table) in CODES.iter().enumerate() {
                    assert_eq!(
                        encode(&char::from_u32(value).unwrap().to_string(), code),
                        vec![lookup(table, *key)[i] | CHAR_CODE_MASK]
                    );
                }
            }
        }
    }
}
#[test]
fn long_random_editing_sequences_stay_bounded() {
    let alphabet = "aaeeoouuwsfrxjzdnghct1234567890 <>^|.,"
        .chars()
        .collect::<Vec<_>>();
    let mut seed = 0x73a98u64;
    for mode in 0..4 {
        for code in 0..5 {
            let mut e = Engine::default();
            e.cfg = config(mode, code, 1, 24);
            e.reset();
            for _ in 0..25000 {
                seed = seed.wrapping_mul(6364136223846793005).wrapping_add(1);
                let c = alphabet[(seed >> 32) as usize % alphabet.len()];
                if c == '|' {
                    e.event(1, 2, 0, 0, false);
                } else {
                    let (k, caps) = key(c);
                    e.event(0, 0, k, caps, false);
                }
                assert!(e.hook.count <= 32);
                assert!(e.hook.code <= 5);
            }
        }
    }
}
#[test]
fn ffi_checks_inputs_and_matches_c_layout() {
    assert_eq!(std::mem::size_of::<Config>(), 52);
    assert_eq!(std::mem::size_of::<ResultState>(), 136);
    unsafe {
        let engine = ok_engine_create();
        let cfg = config(0, 0, 0, 0);
        let mut out = ResultState::default();
        assert_eq!(ok_engine_reset(engine, &cfg, 1), 1);
        assert_eq!(
            ok_engine_process(engine, &cfg, 0, 0, KEY_A, 0, 0, 0, &mut out),
            1
        );
        assert_eq!(
            ok_engine_process(engine, &cfg, 0, 0, KEY_S, 0, 0, 0, &mut out),
            1
        );
        assert_eq!(out.chars[0] & 0xffff, 'á' as u32);
        assert_eq!(
            ok_engine_process(engine, &cfg, 0, 0, 99999, 0, 0, 0, &mut out),
            0
        );
        assert_eq!(out.count, 0);
        let buffer = ok_engine_command(engine, 0, std::ptr::null(), 10, 0, 0);
        assert_eq!(buffer.status, 0);
        ok_buffer_free(buffer);
        let buffer = ok_engine_command(engine, 2, "ms\0một".as_ptr(), "ms\0một".len(), 0, 0);
        assert_eq!(buffer.status, 1);
        ok_buffer_free(buffer);
        ok_engine_destroy(engine);
    }
}

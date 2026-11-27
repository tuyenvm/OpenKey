use crate::{keys::*, tables::*};
use std::collections::BTreeMap;

pub fn lookup<T: Copy + Default>(map: &[(u32, T)], key: u32) -> T {
    map.iter()
        .find(|(k, _)| *k == key)
        .map(|(_, v)| *v)
        .unwrap_or_default()
}
pub fn key_character(key: u32) -> u32 {
    _characterMap
        .iter()
        .find(|(_, v)| *v == key)
        .map(|(k, _)| *k)
        .unwrap_or(0)
}
pub fn character_code(data: u32, code: usize) -> u32 {
    let Some(table) = CODES.get(code) else {
        return data;
    };
    let caps = usize::from(data & CAPS_MASK == 0);
    let mut key = data & CHAR_MASK;
    if data & MARK_MASK != 0 {
        let mark = [MARK1_MASK, MARK2_MASK, MARK3_MASK, MARK4_MASK, MARK5_MASK]
            .iter()
            .position(|&m| m == data & MARK_MASK);
        let Some(mark) = mark else {
            return data;
        };
        let mut index = mark * 2 + caps;
        if [KEY_A, KEY_O, KEY_U, KEY_E].contains(&key) && data & (TONE_MASK | TONEW_MASK) == 0 {
            index += 4;
        }
        if data & TONE_MASK != 0 {
            key |= TONE_MASK;
        } else if data & TONEW_MASK != 0 {
            key |= TONEW_MASK;
        }
        lookup(table, key)
            .get(index)
            .map(|x| x | CHAR_CODE_MASK)
            .unwrap_or(data)
    } else {
        let index = if data & TONE_MASK != 0 {
            caps
        } else if data & TONEW_MASK != 0 {
            caps + 2
        } else {
            return data;
        };
        lookup(table, key)
            .get(index)
            .map(|x| x | CHAR_CODE_MASK)
            .unwrap_or(data)
    }
}
pub fn encode(text: &str, code: usize) -> Vec<u32> {
    let mut out = Vec::new();
    for unit in text.encode_utf16() {
        let x = u32::from(unit);
        if let Some((_, key)) = _characterMap.iter().find(|(k, _)| *k == x) {
            out.push(*key);
            continue;
        }
        if let Some((key, index)) = find_code(x, 0) {
            out.push(lookup(CODES[code.min(4)], key)[index] | CHAR_CODE_MASK);
        } else {
            out.push(x | PURE_CHARACTER_MASK);
        }
    }
    out
}
fn find_code(value: u32, code: usize) -> Option<(u32, usize)> {
    CODES.get(code)?.iter().find_map(|(key, values)| {
        values
            .iter()
            .position(|&v| v == value && v != 0)
            .map(|i| (*key, i))
    })
}
fn set_case(value: u32, upper: bool, code: usize) -> u32 {
    if value & CHAR_CODE_MASK == 0 {
        if value & PURE_CHARACTER_MASK != 0 {
            let Some(c) = char::from_u32(value & CHAR_MASK) else {
                return value;
            };
            return if upper {
                c.to_uppercase().next().unwrap_or(c) as u32
            } else {
                c.to_lowercase().next().unwrap_or(c) as u32
            } | PURE_CHARACTER_MASK;
        }
        let ch = key_character(value);
        let Some(ch) = char::from_u32(ch) else {
            return value;
        };
        let changed = if upper {
            ch.to_ascii_uppercase()
        } else {
            ch.to_ascii_lowercase()
        } as u32;
        return _characterMap
            .iter()
            .find(|(k, _)| *k == changed)
            .map(|(_, v)| *v)
            .unwrap_or(value);
    }
    if let Some((key, i)) = find_code(value & CHAR_MASK, code) {
        return lookup(CODES[code], key)[(i & !1) + usize::from(!upper)] | CHAR_CODE_MASK;
    }
    value
}

#[derive(Default)]
pub struct Macros(pub BTreeMap<String, String>);
impl Macros {
    pub fn add(&mut self, key: &str, value: &str) -> bool {
        // Keep persisted lengths representable by the existing OpenKey format.
        if key.is_empty()
            || key.len() > 255
            || value.len() > 65535
            || (self.0.len() >= 65535 && !self.0.contains_key(key))
        {
            return false;
        }
        self.0.insert(key.to_string(), value.to_string());
        true
    }
    pub fn find(&self, key: &[u32], code: usize, auto_caps: bool) -> Option<Vec<u32>> {
        for (name, value) in &self.0 {
            let encoded = encode(name, code);
            if encoded == key {
                return Some(encode(value, code));
            }
        }
        if !auto_caps || key.is_empty() {
            return None;
        }
        let lowercase: Vec<u32> = key.iter().map(|&x| set_case(x, false, code)).collect();
        if lowercase[0] == key[0] {
            return None;
        }
        let all_caps = key.len() > 1 && lowercase[1] != key[1];
        for (name, value) in &self.0 {
            if encode(name, code) == lowercase {
                return Some(
                    encode(value, code)
                        .into_iter()
                        .enumerate()
                        .map(|(i, x)| {
                            if i == 0 || all_caps {
                                set_case(x, true, code)
                            } else {
                                x
                            }
                        })
                        .collect(),
                );
            }
        }
        None
    }
    pub fn load(&mut self, bytes: &[u8]) -> bool {
        let Some(mut cursor) = Cursor::new(bytes) else {
            return false;
        };
        let mut next = Self::default();
        for _ in 0..cursor.count {
            let Some(name) = cursor.string8() else {
                return false;
            };
            let Some(value) = cursor.string16() else {
                return false;
            };
            if !next.add(name, value) {
                return false;
            }
        }
        if cursor.at != bytes.len() {
            return false;
        }
        *self = next;
        true
    }
    pub fn save(&self, code: usize) -> Vec<u8> {
        let mut out = (self.0.len() as u16).to_le_bytes().to_vec();
        let mut entries: Vec<_> = self.0.iter().collect();
        entries.sort_by_cached_key(|(key, _)| encode(key, code));
        for (name, value) in entries {
            out.push(name.len() as u8);
            out.extend(name.as_bytes());
            out.extend((value.len() as u16).to_le_bytes());
            out.extend(value.as_bytes());
        }
        out
    }
    pub fn export(&self) -> String {
        let mut text =
            String::from(";Compatible OpenKey Macro Data file for UniKey*** version=1 ***\n");
        for (key, value) in &self.0 {
            text.push_str(key);
            text.push(':');
            text.push_str(value);
            text.push('\n');
        }
        text
    }
    pub fn import(&mut self, text: &str, append: bool) -> bool {
        let mut next = if append {
            Self(self.0.clone())
        } else {
            Self::default()
        };
        let mut lines = text.lines();
        if !lines
            .next()
            .is_some_and(|line| line.trim_start_matches('\u{feff}').starts_with(';'))
        {
            return false;
        }
        for line in lines {
            if line.is_empty() || line.starts_with(';') {
                continue;
            }
            let Some((key, value)) = line.split_once(':') else {
                return false;
            };
            let (key, value) = if key.is_empty() {
                let Some((rest, content)) = value.split_once(':') else {
                    return false;
                };
                (format!(":{rest}"), content)
            } else {
                (key.to_string(), value)
            };
            if !next.0.contains_key(&key) && !next.add(&key, value) {
                return false;
            }
        }
        *self = next;
        true
    }
}
pub struct Cursor<'a> {
    data: &'a [u8],
    pub at: usize,
    pub count: u16,
}
impl<'a> Cursor<'a> {
    pub fn new(data: &'a [u8]) -> Option<Self> {
        Some(Self {
            data,
            at: 2,
            count: u16::from_le_bytes(data.get(..2)?.try_into().ok()?),
        })
    }
    pub fn take(&mut self, len: usize) -> Option<&'a [u8]> {
        let end = self.at.checked_add(len)?;
        let bytes = self.data.get(self.at..end)?;
        self.at = end;
        Some(bytes)
    }
    pub fn byte(&mut self) -> Option<u8> {
        Some(*self.take(1)?.first()?)
    }
    pub fn string8(&mut self) -> Option<&'a str> {
        let len = self.byte()? as usize;
        std::str::from_utf8(self.take(len)?).ok()
    }
    pub fn string16(&mut self) -> Option<&'a str> {
        let len = u16::from_le_bytes(self.take(2)?.try_into().ok()?) as usize;
        std::str::from_utf8(self.take(len)?).ok()
    }
}
pub fn load_apps(bytes: &[u8]) -> Option<BTreeMap<String, u8>> {
    let mut cursor = Cursor::new(bytes)?;
    let mut out = BTreeMap::new();
    for _ in 0..cursor.count {
        let name = cursor.string8()?.to_string();
        let value = cursor.byte()?;
        if name.is_empty() || value > 9 {
            return None;
        }
        out.insert(name, value);
    }
    if cursor.at != bytes.len() {
        return None;
    }
    Some(out)
}
pub fn save_apps(apps: &BTreeMap<String, u8>) -> Vec<u8> {
    let mut out = (apps.len() as u16).to_le_bytes().to_vec();
    for (key, &value) in apps {
        out.push(key.len() as u8);
        out.extend(key.as_bytes());
        out.push(value);
    }
    out
}
pub fn convert(text: &str, from: usize, to: usize, flags: u32) -> String {
    if from >= 5 || to >= 5 {
        return text.to_string();
    }
    let data: Vec<u16> = text.encode_utf16().collect();
    let mut out = Vec::new();
    let mut i = 0;
    let upper = flags & 1 != 0;
    let lower = flags & 2 != 0;
    let sentence = flags & 4 != 0;
    let words = flags & 8 != 0;
    let remove = flags & 16 != 0;
    let mut next_upper = (sentence || words) && !lower;
    let mut has_break = false;
    while i < data.len() {
        let mut found = None;
        let x = u32::from(data[i]);
        if i + 1 < data.len() {
            let y = u32::from(data[i + 1]);
            let packed = if from == 2 || from == 4 {
                Some(x | (y << 8))
            } else if from == 3 {
                MARKS
                    .iter()
                    .position(|&m| m == y)
                    .map(|m| x | (((m + 1) as u32) << 13))
            } else {
                Some(x)
            };
            if let Some(packed) = packed {
                found = find_code(packed, from)
                    .map(|entry| (entry, usize::from(from == 2 || from == 3 || from == 4)));
            }
        }
        if found.is_none() {
            found = find_code(x, from).map(|entry| (entry, 0));
        }
        if let Some(((key, index), extra)) = found {
            let index = if upper || next_upper {
                index & !1
            } else if lower {
                index | 1
            } else {
                index
            };
            let packed = lookup(CODES[to], key)[index];
            if remove {
                let base = key_character((key & 0xff) | if index % 2 == 0 { CAPS_MASK } else { 0 });
                out.push(base as u16);
            } else if to == 2 || to == 4 {
                out.push((packed & 255) as u16);
                if packed >> 8 > 32 {
                    out.push((packed >> 8) as u16);
                }
            } else if to == 3 && packed >> 13 > 0 {
                out.push((packed & 0x1fff) as u16);
                out.push(MARKS[((packed >> 13) - 1) as usize] as u16);
            } else {
                out.push(packed as u16);
            }
            i += 1 + extra;
            next_upper = false;
            has_break = false;
            continue;
        }
        // Preserve surrogate pairs and case by default; the legacy converter lowercased everything.
        if let Some(c) = char::from_u32(x) {
            let converted = if upper || next_upper {
                c.to_uppercase().collect::<String>()
            } else if lower {
                c.to_lowercase().collect::<String>()
            } else {
                c.to_string()
            };
            out.extend(converted.encode_utf16());
        } else {
            out.push(data[i]);
        }
        if x == 10 || (has_break && x == 32) || (x == 32 && words) {
            next_upper = sentence || words;
        } else if [46, 63, 33].contains(&x) {
            has_break = true;
        } else {
            next_upper = false;
            has_break = false;
        }
        i += 1;
    }
    String::from_utf16_lossy(&out)
}

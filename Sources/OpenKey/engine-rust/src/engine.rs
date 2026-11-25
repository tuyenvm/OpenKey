use crate::text::{character_code, lookup};
use crate::{keys::*, tables::*, Config};

#[derive(Default, Clone)]
pub struct Hook {
    pub code: u8,
    pub backspaces: u8,
    pub count: u8,
    pub ext: u8,
    pub chars: [u32; 32],
    pub macro_key: Vec<u32>,
    pub macro_data: Vec<u32>,
}

#[derive(Default)]
pub struct Engine {
    pub cfg: Config,
    pub hook: Hook,
    word: [u32; 32],
    len: usize,
    raw: Vec<u32>,
    long_word: Vec<u32>,
    history: Vec<Vec<u32>>,
    special: Vec<u32>,
    spaces: usize,
    disabled: bool,
    pub temporary_off: bool,
    pub spelling_before: bool,
    macro_handled: bool,
    quick_handled: bool,
    upper_status: u8,
    pub macros: crate::text::Macros,
    pub apps: std::collections::BTreeMap<String, u8>,
}
fn consonant(key: u32) -> bool {
    ![KEY_A, KEY_E, KEY_I, KEY_O, KEY_U, KEY_Y].contains(&key)
}
fn number(key: u32) -> bool {
    [
        KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
    ]
    .contains(&key)
}
fn punctuation(key: u32) -> bool {
    [
        KEY_BACKQUOTE,
        KEY_1,
        KEY_2,
        KEY_3,
        KEY_4,
        KEY_5,
        KEY_6,
        KEY_7,
        KEY_8,
        KEY_9,
        KEY_0,
        KEY_MINUS,
        KEY_EQUALS,
        KEY_LEFT_BRACKET,
        KEY_RIGHT_BRACKET,
        KEY_BACK_SLASH,
        KEY_SEMICOLON,
        KEY_QUOTE,
        KEY_COMMA,
        KEY_DOT,
        KEY_SLASH,
    ]
    .contains(&key)
}
fn word_break(key: u32) -> bool {
    [
        KEY_ESC,
        KEY_TAB,
        KEY_ENTER,
        KEY_RETURN,
        KEY_LEFT,
        KEY_RIGHT,
        KEY_DOWN,
        KEY_UP,
        KEY_COMMA,
        KEY_DOT,
        KEY_SLASH,
        KEY_SEMICOLON,
        KEY_QUOTE,
        KEY_BACK_SLASH,
        KEY_MINUS,
        KEY_EQUALS,
        KEY_BACKQUOTE,
    ]
    .contains(&key)
}
fn macro_break(key: u32) -> bool {
    [
        KEY_RETURN,
        KEY_COMMA,
        KEY_DOT,
        KEY_SLASH,
        KEY_SEMICOLON,
        KEY_QUOTE,
        KEY_BACK_SLASH,
        KEY_MINUS,
        KEY_EQUALS,
    ]
    .contains(&key)
}

impl Engine {
    pub fn reset(&mut self) {
        self.len = 0;
        self.raw.clear();
        self.spelling_before = self.cfg.spelling != 0;
        self.history.clear();
        self.long_word.clear();
    }
    pub fn new_session(&mut self) {
        self.len = 0;
        self.hook.backspaces = 0;
        self.hook.count = 0;
        self.disabled = false;
        self.raw.clear();
        self.macro_handled = false;
        self.quick_handled = false;
        self.long_word.clear();
    }
    fn w(&self, i: usize) -> u32 {
        self.word.get(i).copied().unwrap_or(0)
    }
    fn ch(&self, i: usize) -> u32 {
        self.w(i) & CHAR_MASK
    }
    fn vowels(&self, grammar: bool) -> Option<(usize, usize, usize)> {
        let mut start = 0;
        let mut end = 0;
        let mut count: usize = 0;
        for i in (0..self.len).rev() {
            if consonant(self.ch(i)) {
                if count > 0 {
                    break;
                }
            } else {
                if count == 0 {
                    end = i;
                }
                if !grammar
                    && i > 0
                    && ((self.ch(i) == KEY_I && self.ch(i - 1) == KEY_G)
                        || (self.ch(i) == KEY_U && self.ch(i - 1) == KEY_Q))
                {
                    break;
                }
                start = i;
                count += 1;
            }
        }
        if start > 0 && self.ch(start) == KEY_U && self.ch(start - 1) == KEY_Q {
            start += 1;
            count = count.saturating_sub(1);
        }
        // Keep gi/qui handling compatible: the old engine uses the last i even with count zero.
        if count == 0 && self.len == 0 {
            None
        } else {
            Some((start, end, count))
        }
    }
    fn spelling(&mut self, force_vowel: bool) {
        let n = self.len.saturating_sub(usize::from(
            self.len > 0 && self.ch(self.len - 1) == KEY_RIGHT_BRACKET,
        ));
        if n == 0 {
            self.disabled = false;
            return;
        }
        let mut j = 0;
        if consonant(self.ch(0)) {
            for pattern in _consonantTable {
                let mut bad = n < pattern.len();
                j = 0;
                for (pos, &key) in pattern.iter().enumerate() {
                    j = pos;
                    if n > j
                        && (key
                            & !if self.cfg.quick_start != 0 {
                                END_CONSONANT_MASK
                            } else {
                                0
                            })
                            != self.ch(j)
                        && (key
                            & !if self.cfg.allow_consonants != 0 {
                                CONSONANT_ALLOW_MASK
                            } else {
                                0
                            })
                            != self.ch(j)
                    {
                        bad = true;
                        break;
                    }
                    j = pos + 1;
                }
                if !bad {
                    break;
                }
            }
        }
        let mut ok = j == n;
        let mut k = j;
        if self.ch(k) == KEY_U && k > 0 && k < n.saturating_sub(1) && self.ch(k - 1) == KEY_Q {
            k += 1;
            j = k;
        } else if self.len >= 2
            && self.ch(0) == KEY_G
            && self.ch(1) == KEY_I
            && consonant(self.ch(2))
        {
            k = 1;
            j = 1;
        }
        for _ in 0..3 {
            if k < n && !consonant(self.ch(k)) {
                k += 1;
            }
        }
        let mut vowel_ok = true;
        if k > j {
            vowel_ok = false;
            if k - j > 1 && force_vowel {
                for pattern in lookup(_vowelCombine, self.ch(j)) {
                    let mut end = 1;
                    let mut bad = false;
                    for (i, &key) in pattern.iter().enumerate().skip(1) {
                        end = i;
                        let at = j + i - 1;
                        if at < n && key != (self.ch(at) | (self.w(at) & (TONEW_MASK | TONE_MASK)))
                        {
                            bad = true;
                            break;
                        }
                        end = i + 1;
                    }
                    if bad
                        || (k < n && pattern[0] == 0)
                        || (j + end - 1 < n && !consonant(self.ch(j + end - 1)))
                    {
                        continue;
                    }
                    vowel_ok = true;
                    break;
                }
            } else if !consonant(self.ch(j)) {
                vowel_ok = true;
            }
            for pattern in _endConsonantTable {
                let mut bad = false;
                for (i, &key) in pattern.iter().enumerate() {
                    if n > k + i
                        && (key
                            & !if self.cfg.quick_end != 0 {
                                END_CONSONANT_MASK
                            } else {
                                0
                            })
                            != self.ch(k + i)
                    {
                        bad = true;
                        break;
                    }
                }
                if !bad && k + pattern.len() >= n {
                    ok = true;
                    break;
                }
            }
            if ok {
                let allowed =
                    |x: u32| x & MARK1_MASK != 0 || x & MARK5_MASK != 0 || x & MARK_MASK == 0;
                if self.len >= 3
                    && self.ch(self.len - 1) == KEY_H
                    && self.ch(self.len - 2) == KEY_C
                    && !allowed(self.w(self.len - 3))
                    || (self.len >= 2
                        && self.ch(self.len - 1) == KEY_T
                        && !allowed(self.w(self.len - 2)))
                {
                    ok = false;
                }
            }
        }
        self.disabled = !(ok && vowel_ok);
    }
    fn insert(&mut self, key: u32, caps: bool, check: bool) {
        let value = key | if caps { CAPS_MASK } else { 0 };
        if self.len == 32 {
            self.long_word.push(self.word[0]);
            self.word.copy_within(1..32, 0);
            self.word[31] = value;
        } else {
            self.word[self.len] = value;
            self.len += 1;
        }
        if self.cfg.spelling != 0 && check {
            self.spelling(false);
        }
        if key == KEY_D && self.len >= 2 && consonant(self.ch(self.len - 2)) {
            self.disabled = false;
        }
    }
    fn save_word(&mut self) {
        if self.hook.code != 4 {
            if self.len > 0 {
                for chunk in self.long_word.chunks(32) {
                    self.history.push(chunk.to_vec());
                }
                self.long_word.clear();
                self.history.push(self.word[..self.len].to_vec());
            }
        } else {
            for chunk in self.hook.macro_data.chunks(32) {
                self.history.push(chunk.to_vec());
            }
        }
        // ponytail: retain 256 history chunks; raise the cap if deep backward editing needs it.
        if self.history.len() > 256 {
            self.history.drain(..self.history.len() - 256);
        }
    }
    fn save_special(&mut self) {
        self.history.push(std::mem::take(&mut self.special));
    }
    fn restore_history(&mut self) {
        if let Some(last) = self.history.pop() {
            if last.first() == Some(&KEY_SPACE) {
                self.spaces = last.len();
                self.len = 0;
            } else if last.first().is_some_and(|x| punctuation(x & CHAR_MASK)) {
                self.len = 0;
                self.special = last;
                self.spelling(false);
            } else {
                self.len = last.len().min(32);
                self.word[..self.len].copy_from_slice(&last[..self.len]);
            }
        }
    }
    fn suffix_valid(&self, pattern: &[u32], mark_key: u32) -> bool {
        if self.len < pattern.len()
            || (self.len >= 2 && self.ch(self.len - 1) == KEY_U && self.ch(self.len - 2) == KEY_Q)
        {
            return false;
        }
        let start = self.len - pattern.len();
        for (i, &key) in pattern.iter().enumerate() {
            if (key
                & !if self.cfg.quick_end != 0 {
                    END_CONSONANT_MASK
                } else {
                    0
                })
                != self.ch(start + i)
            {
                return false;
            }
        }
        if pattern.len() > 1
            && [
                self.processing()[1],
                self.processing()[2],
                self.processing()[3],
            ]
            .contains(&mark_key)
            && ([KEY_C, KEY_T].contains(&pattern[1]) || pattern.get(2) == Some(&KEY_T))
        {
            return false;
        }
        !(start > 0 && self.ch(start - 1) == self.ch(start))
    }
    fn processing(&self) -> [u32; 11] {
        if self.cfg.input_type == 1 {
            [
                KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0,
            ]
        } else {
            [
                KEY_S, KEY_F, KEY_R, KEY_X, KEY_J, KEY_A, KEY_O, KEY_E, KEY_W, KEY_D, KEY_Z,
            ]
        }
    }
    fn character(&self, x: u32) -> u32 {
        character_code(x, self.cfg.code_table as usize)
    }
    fn output_suffix(&mut self, start: usize) {
        for i in start..self.len {
            self.hook.chars[self.len - 1 - i] = self.character(self.w(i));
        }
    }
    fn can_end(&self, start: usize, end: usize) -> bool {
        lookup(_vowelCombine, self.ch(start)).iter().any(|p| {
            p.iter().skip(1).enumerate().all(|(i, &x)| {
                start + i <= end
                    && x == (self.ch(start + i) | (self.w(start + i) & (TONE_MASK | TONEW_MASK)))
            }) && p[0] == 1
        })
    }
    fn mark_position(&self, start: usize, end: usize, count: usize) -> usize {
        if count == 1 {
            return end;
        }
        let mut pos = if count == 0 && self.ch(end) == KEY_I {
            end
        } else {
            start
        };
        if self.cfg.modern == 0 {
            if count == 3
                || (end + 1 < self.len && consonant(self.ch(end + 1)) && self.can_end(start, end))
            {
                pos = start + 1;
            }
            for i in start..=end {
                if (self.ch(i) == KEY_E && self.w(i) & TONE_MASK != 0)
                    || (self.ch(i) == KEY_O && self.w(i) & TONEW_MASK != 0)
                {
                    pos = i;
                    break;
                }
            }
        } else {
            pos = end;
            let a = self.ch(start);
            let b = self.ch(start + 1);
            let c = self.ch(start + 2);
            if count == 3
                && [
                    (KEY_O, KEY_A, KEY_I),
                    (KEY_U, KEY_Y, KEY_U),
                    (KEY_O, KEY_E, KEY_O),
                    (KEY_U, KEY_Y, KEY_A),
                ]
                .contains(&(a, b, c))
            {
                pos = start + 1;
            } else if [(KEY_O, KEY_I), (KEY_A, KEY_I), (KEY_U, KEY_I)].contains(&(a, b)) {
                pos = start;
            } else if end > 0 && self.ch(end - 1) == KEY_A && self.ch(end) == KEY_Y {
                pos = end - 1;
            } else if (a, b) == (KEY_U, KEY_O) {
                pos = start + 1;
            } else if [KEY_O, KEY_U].contains(&b) {
                pos = end.saturating_sub(1);
            }
            if ([KEY_I, KEY_Y].contains(&a) && self.w(start + 1) & (KEY_E | TONE_MASK) != 0)
                || (a == KEY_U && self.w(start + 1) == KEY_O | TONE_MASK)
                || (self.w(start) == KEY_U | TONEW_MASK && self.w(start + 1) == KEY_O | TONEW_MASK)
            {
                pos = if start + 2 < self.len
                    && [KEY_P, KEY_T, KEY_M, KEY_N, KEY_O, KEY_U, KEY_I, KEY_C].contains(&c)
                {
                    start + 1
                } else {
                    start
                };
            } else if a == KEY_U && self.w(start + 1) == KEY_U | TONEW_MASK {
                pos = start;
            }
            if count == 2 {
                if a == KEY_I && [KEY_A, KEY_U, KEY_O].contains(&b) {
                    pos = if start == 0 || self.ch(start - 1) != KEY_G {
                        start
                    } else {
                        start + 1
                    };
                } else if (a, b) == (KEY_U, KEY_A) {
                    if start == 0 || self.ch(start - 1) != KEY_Q {
                        if end + 1 >= self.len || !self.can_end(start, end) {
                            pos = start;
                        }
                    } else {
                        pos = start + 1;
                    }
                } else if (a, b) == (KEY_O, KEY_O) {
                    pos = end;
                }
            }
        }
        if self.w(end) & (TONE_MASK | TONEW_MASK) != 0 {
            pos = end;
        }
        pos.min(self.len.saturating_sub(1))
    }
    fn insert_mark(&mut self, mask: u32, modify: bool) -> usize {
        if modify {
            self.hook.code = 1;
        }
        self.hook.backspaces = 0;
        self.hook.count = 0;
        let Some((start, end, count)) = self.vowels(false) else {
            return 0;
        };
        let pos = self.mark_position(start, end, count);
        let duplicate = self.w(pos) & mask != 0;
        if duplicate {
            if modify {
                self.hook.code = 3;
            }
            self.disabled = true;
        }
        for i in start..self.len {
            self.word[i] &= !MARK_MASK;
        }
        if !duplicate {
            self.word[pos] |= mask;
        }
        self.output_suffix(start);
        self.hook.backspaces = (self.len - start) as u8;
        self.hook.count = self.hook.backspaces;
        pos
    }
    fn grammar(&mut self, delta: i32) {
        if self.len <= 1 || self.len >= 32 {
            return;
        }
        let Some((start, end, count)) = self.vowels(true) else {
            return;
        };
        if count == 0 {
            return;
        }
        let mut changed = false;
        if self.len >= 3 {
            for i in (2..self.len).rev() {
                if [KEY_N, KEY_C, KEY_I, KEY_M, KEY_P, KEY_T].contains(&self.ch(i))
                    && self.ch(i - 1) == KEY_O
                    && self.ch(i - 2) == KEY_U
                    && (self.w(i - 1) ^ self.w(i - 2)) & TONEW_MASK != 0
                {
                    self.word[i - 2] |= TONEW_MASK;
                    self.word[i - 1] |= TONEW_MASK;
                    changed = true;
                    break;
                }
            }
        }
        for i in start..=end {
            let mark = self.w(i) & MARK_MASK;
            if mark != 0 {
                self.word[i] &= !MARK_MASK;
                let pos = self.insert_mark(mark, false);
                if i != pos {
                    changed = true;
                }
                break;
            }
        }
        if changed {
            if self.hook.code == 0 {
                self.hook.code = 1;
            }
            self.output_suffix(start);
            self.hook.count = (self.len - start) as u8;
            self.hook.backspaces = ((self.len - start) as i32 + delta).max(0) as u8;
            self.hook.ext = 4;
        }
    }
    fn insert_double(&mut self, data: u32, is_d: bool) {
        if !is_d {
            if let Some((s, e, _)) = self.vowels(false) {
                for value in self.word.iter_mut().take(e + 1).skip(s) {
                    *value &= !TONEW_MASK;
                }
            }
        }
        self.hook.code = 1;
        self.hook.backspaces = 0;
        for i in (0..self.len).rev() {
            self.hook.backspaces += 1;
            if self.ch(i) == data {
                if self.w(i) & TONE_MASK != 0 {
                    self.hook.code = 3;
                    self.word[i] &= !TONE_MASK;
                    self.hook.chars[self.len - 1 - i] = self.word[i];
                    if is_d || data != KEY_O {
                        self.disabled = true;
                    }
                } else {
                    self.word[i] |= TONE_MASK;
                    if !is_d {
                        self.word[i] &= !TONEW_MASK;
                    }
                    self.hook.chars[self.len - 1 - i] = self.character(self.w(i));
                }
                break;
            }
            self.hook.chars[self.len - 1 - i] = self.character(self.w(i));
        }
        self.hook.count = self.hook.backspaces;
    }
    fn insert_w(&mut self) {
        let Some((s, e, count)) = self.vowels(false) else {
            return;
        };
        for value in self.word.iter_mut().take(e + 1).skip(s) {
            *value &= !TONE_MASK;
        }
        self.hook.code = 1;
        if count > 1 {
            self.hook.backspaces = (self.len - s) as u8;
            self.hook.count = self.hook.backspaces;
            let a = self.ch(s);
            let b = self.ch(s + 1);
            if (self.w(s) & TONEW_MASK != 0 && self.w(s + 1) & TONEW_MASK != 0)
                || (self.w(s) & TONEW_MASK != 0 && [KEY_I, KEY_A].contains(&b))
            {
                self.hook.code = 3;
                self.disabled = true;
                for i in s..self.len {
                    self.word[i] &= !TONEW_MASK;
                    self.hook.chars[self.len - 1 - i] =
                        self.character(self.w(i)) & !STANDALONE_MASK;
                }
            } else {
                if (a, b) == (KEY_U, KEY_O) {
                    if s >= 2 && self.w(s - 2) == KEY_T && self.w(s - 1) == KEY_H {
                        self.word[s + 1] |= TONEW_MASK;
                        if s + 2 < self.len && self.ch(s + 2) == KEY_N {
                            self.word[s] |= TONEW_MASK;
                        }
                    } else if s >= 1 && self.w(s - 1) == KEY_Q {
                        self.word[s + 1] |= TONEW_MASK;
                    } else {
                        self.word[s] |= TONEW_MASK;
                        self.word[s + 1] |= TONEW_MASK;
                    }
                } else if [
                    (KEY_U, KEY_A),
                    (KEY_U, KEY_I),
                    (KEY_U, KEY_U),
                    (KEY_O, KEY_I),
                ]
                .contains(&(a, b))
                {
                    self.word[s] |= TONEW_MASK;
                } else if [(KEY_I, KEY_O), (KEY_O, KEY_A)].contains(&(a, b)) {
                    self.word[s + 1] |= TONEW_MASK;
                } else {
                    self.disabled = true;
                    self.hook.code = 0;
                }
                self.output_suffix(s);
            }
            return;
        }
        self.hook.backspaces = 0;
        for i in (s..self.len).rev() {
            self.hook.backspaces += 1;
            if [KEY_A, KEY_U, KEY_O].contains(&self.ch(i)) {
                if self.w(i) & TONEW_MASK != 0 {
                    if self.w(i) & STANDALONE_MASK != 0 {
                        if self.ch(i) == KEY_U {
                            self.word[i] = KEY_W | (self.w(i) & CAPS_MASK);
                        } else if self.ch(i) == KEY_O {
                            self.hook.code = 3;
                            self.word[i] = KEY_O | (self.w(i) & CAPS_MASK);
                        }
                    } else {
                        self.hook.code = 3;
                        self.word[i] &= !TONEW_MASK;
                    }
                    self.hook.chars[self.len - 1 - i] = self.word[i];
                    self.disabled = true;
                } else {
                    self.word[i] = (self.word[i] | TONEW_MASK) & !TONE_MASK;
                    self.hook.chars[self.len - 1 - i] = self.character(self.w(i));
                }
            } else {
                self.hook.chars[self.len - 1 - i] = self.character(self.w(i));
            }
        }
        self.hook.count = self.hook.backspaces;
    }
    fn standalone(&mut self, data: u32, caps: bool, target: u32) {
        if self.len > 0 && self.ch(self.len - 1) == target && self.w(self.len - 1) & TONEW_MASK != 0
        {
            self.hook.code = 1;
            self.hook.backspaces = 1;
            self.hook.count = 1;
            self.word[self.len - 1] = data | if caps { CAPS_MASK } else { 0 };
            self.hook.chars[0] = self.character(self.w(self.len - 1));
            return;
        }
        let allowed = (self.len > 0 && self.ch(self.len - 1) == KEY_U && target == KEY_O)
            || self.len == 0
            || (self.len == 1 && !_standaloneWbad.contains(&self.ch(0)))
            || (self.len == 2
                && _doubleWAllowed
                    .iter()
                    .any(|p| p[0] == self.ch(0) && p[1] == self.ch(1)));
        self.insert(
            if allowed && target == KEY_O && self.len > 0 && self.ch(self.len - 1) == KEY_U {
                target
            } else {
                data
            },
            caps,
            !allowed,
        );
        if allowed {
            self.hook.code = 1;
            self.hook.backspaces = 0;
            self.hook.count = 1;
            self.hook.ext = 4;
            self.word[self.len - 1] =
                target | TONEW_MASK | STANDALONE_MASK | if caps { CAPS_MASK } else { 0 };
            self.hook.chars[0] = self.character(self.w(self.len - 1));
        }
    }
    fn main_key(&mut self, data: u32, caps: bool) {
        let p = self.processing();
        if data == p[10] {
            let (s, e, _) = self.vowels(true).unwrap_or((0, 0, 0));
            let mut changed = false;
            if self.len > 0 {
                for value in self.word.iter_mut().take(e + 1).skip(s) {
                    if *value & MARK_MASK != 0 {
                        *value &= !MARK_MASK;
                        changed = true;
                    }
                }
            }
            if changed {
                self.hook.code = 1;
                self.hook.backspaces = (self.len - s) as u8;
                self.hook.count = self.hook.backspaces;
                self.output_suffix(s);
            } else {
                self.insert(data, caps, true);
            }
            return;
        }
        if [KEY_LEFT_BRACKET, KEY_RIGHT_BRACKET].contains(&data) {
            self.standalone(
                data,
                caps,
                if data == KEY_LEFT_BRACKET {
                    KEY_O
                } else {
                    KEY_U
                },
            );
            return;
        }
        if data == p[9] {
            if _consonantD.iter().any(|pat| self.suffix_valid(pat, data))
                || (self.len >= 2
                    && self.ch(self.len - 1) == KEY_D
                    && consonant(self.ch(self.len - 2)))
            {
                self.insert_double(KEY_D, true);
            } else {
                self.insert(data, caps, true);
            }
            return;
        }
        if let Some(tone) = p[..5].iter().position(|&x| x == data) {
            if _vowelForMark
                .iter()
                .any(|(_, set)| set.iter().any(|pat| self.suffix_valid(pat, data)))
            {
                self.insert_mark(
                    [MARK1_MASK, MARK2_MASK, MARK3_MASK, MARK4_MASK, MARK5_MASK][tone],
                    true,
                );
            } else {
                self.insert(data, caps, true);
            }
            return;
        }
        let last_aeo = (0..self.len)
            .rev()
            .find(|&i| [KEY_A, KEY_O, KEY_E].contains(&self.ch(i)))
            .unwrap_or(0);
        let key = if self.cfg.input_type != 1 {
            data
        } else if [KEY_7, KEY_8].contains(&data) {
            KEY_W
        } else if data == KEY_6 {
            self.w(last_aeo)
        } else {
            data
        };
        if lookup(_vowel, key)
            .iter()
            .any(|pat| self.suffix_valid(pat, data))
        {
            if [p[5], p[6], p[7]].contains(&data) && !(self.cfg.input_type == 1 && data == KEY_7) {
                self.insert_double(key, false);
            } else if [p[8], p[7]].contains(&data) && (self.cfg.input_type == 1 || data == KEY_W) {
                let e = (0..self.len)
                    .rev()
                    .find(|&i| [KEY_A, KEY_O, KEY_U, KEY_E].contains(&self.ch(i)))
                    .unwrap_or(0);
                let skip = self.cfg.input_type == 1
                    && ((data == KEY_7
                        && self.ch(e) == KEY_A
                        && (e == 0 || self.ch(e - 1) != KEY_U))
                        || (data == KEY_8 && [KEY_O, KEY_U].contains(&self.ch(e))));
                if !skip {
                    self.insert_w();
                }
            }
        } else if data == KEY_W && self.cfg.input_type != 2 {
            self.standalone(data, caps, KEY_U);
        } else {
            self.insert(data, caps, true);
        }
    }
    fn quick_consonant(&mut self) -> bool {
        if self.len <= 1 || self.len >= 31 {
            return false;
        }
        let mut changed = false;
        if self.cfg.quick_start != 0 {
            if let Some((_, pair)) = _quickStartConsonant.iter().find(|(k, _)| *k == self.ch(0)) {
                let caps = self.w(0) & CAPS_MASK;
                let second_caps = if caps != 0 && self.w(1) & CAPS_MASK != 0 {
                    CAPS_MASK
                } else {
                    0
                };
                self.hook.code = 3;
                self.hook.backspaces = self.len as u8;
                self.hook.count = (self.len + 1) as u8;
                if self.len < 31 {
                    self.len += 1;
                }
                for i in (2..self.len).rev() {
                    self.word[i] = self.word[i - 1];
                }
                self.word[0] = pair[0] | caps;
                self.word[1] = pair[1] | second_caps;
                changed = true;
            }
        }
        if self.cfg.quick_end != 0 && self.len >= 2 && !consonant(self.ch(self.len - 2)) {
            if let Some((_, pair)) = _quickEndConsonant
                .iter()
                .find(|(k, _)| *k == self.ch(self.len - 1))
            {
                self.hook.code = 3;
                if changed {
                    self.hook.count += 1;
                } else {
                    self.hook.backspaces = 1;
                    self.hook.count = 2;
                }
                if self.len < 31 {
                    self.len += 1;
                }
                let caps = self.w(self.len - 2) & CAPS_MASK;
                self.word[self.len - 2] = pair[0] | caps;
                self.word[self.len - 1] = pair[1] | caps;
                changed = true;
            }
        }
        if changed {
            self.quick_handled = true;
            self.output_suffix(0);
        }
        changed
    }
    fn restore_invalid(&mut self, code: u8) -> bool {
        if !(0..self.len).any(|i| {
            !consonant(self.ch(i)) && self.w(i) & (MARK_MASK | TONE_MASK | TONEW_MASK) != 0
        }) {
            return false;
        }
        self.hook.code = code;
        self.hook.backspaces = self.len as u8;
        self.hook.count = self.raw.len() as u8;
        for (i, &x) in self.raw.iter().enumerate() {
            self.word[i] = x;
            self.hook.chars[self.raw.len() - 1 - i] = x;
        }
        self.len = self.raw.len();
        true
    }
    fn find_macro(&mut self) -> bool {
        self.hook.macro_key = self
            .hook
            .macro_key
            .iter()
            .map(|&x| self.character(x))
            .collect();
        if let Some(data) = self.macros.find(
            &self.hook.macro_key,
            self.cfg.code_table as usize,
            self.cfg.auto_caps_macro != 0,
        ) {
            self.hook.macro_data = data;
            true
        } else {
            false
        }
    }
    pub fn english(&mut self, state: u32, data: u32, caps: bool, control: bool) {
        self.hook.code = 0;
        if state == 2 || (control && !caps) {
            self.hook.macro_key.clear();
            self.temporary_off = false;
        } else if data == KEY_SPACE {
            if !self.macro_handled && self.find_macro() {
                self.hook.code = 4;
                self.hook.backspaces = self.hook.macro_key.len().min(255) as u8;
            }
            self.hook.macro_key.clear();
            self.temporary_off = false;
        } else if data == KEY_DELETE {
            if self.hook.macro_key.pop().is_none() {
                self.temporary_off = false;
            }
        } else if word_break(data) && !punctuation(data) {
            self.hook.macro_key.clear();
            self.temporary_off = false;
        } else if !self.temporary_off && self.hook.macro_key.len() < 255 {
            self.hook
                .macro_key
                .push(data | if caps { CAPS_MASK } else { 0 });
        }
    }
    pub fn event(&mut self, event: u32, state: u32, data: u32, caps_status: u32, control: bool) {
        let caps = caps_status == 1 || caps_status == 2;
        if (number(data) && caps_status == 1)
            || control
            || event == 1
            || word_break(data)
            || (self.len == 0 && number(data))
        {
            self.hook.code = 0;
            self.hook.backspaces = 0;
            self.hook.count = 0;
            self.hook.ext = 1;
            if self.cfg.macros != 0 && macro_break(data) && !self.macro_handled && self.find_macro()
            {
                self.hook.code = 4;
                self.hook.backspaces = self.hook.macro_key.len().min(255) as u8;
                self.macro_handled = true;
            } else if (self.cfg.quick_start != 0 || self.cfg.quick_end != 0)
                && !self.disabled
                && macro_break(data)
            {
                self.quick_consonant();
            } else if self.cfg.restore_invalid != 0 && (event == 1 || word_break(data)) {
                if !self.disabled && self.cfg.spelling != 0 {
                    self.spelling(true);
                }
                if self.disabled && !self.restore_invalid(5) {
                    self.hook.code = 0;
                }
            }
            let is_punctuation = state == 0 && punctuation(data);
            if !is_punctuation {
                self.special.clear();
                self.history.clear();
            } else {
                if self.spaces > 0 {
                    self.history.push(vec![KEY_SPACE; self.spaces.min(32)]);
                    self.spaces = 0;
                } else {
                    self.save_word();
                }
                self.special.push(data | if caps { CAPS_MASK } else { 0 });
                self.hook.ext = 3;
            }
            if self.hook.code == 0 {
                self.new_session();
                self.cfg.spelling = i32::from(self.spelling_before);
                self.temporary_off = false;
            } else if self.hook.code == 4 || self.quick_handled {
                self.len = 0;
            }
            if self.cfg.macros != 0 {
                if is_punctuation && self.hook.macro_key.len() < 255 {
                    self.hook
                        .macro_key
                        .push(data | if caps { CAPS_MASK } else { 0 });
                } else {
                    self.hook.macro_key.clear();
                }
            }
            if self.cfg.upper_first != 0 {
                self.upper_status = if data == KEY_DOT {
                    1
                } else if [KEY_ENTER, KEY_RETURN].contains(&data) {
                    2
                } else {
                    0
                };
            }
        } else if data == KEY_SPACE {
            if !self.disabled && self.cfg.spelling != 0 {
                self.spelling(true);
            }
            if self.cfg.macros != 0 && !self.macro_handled && self.find_macro() {
                self.hook.code = 4;
                self.hook.backspaces = self.hook.macro_key.len().min(255) as u8;
                self.macro_handled = true;
            } else if (self.cfg.quick_start != 0 || self.cfg.quick_end != 0)
                && !self.disabled
                && self.quick_consonant()
            {
            } else if self.cfg.restore_invalid != 0 && self.disabled && !self.macro_handled {
                if !self.restore_invalid(3) {
                    self.hook.code = 0;
                }
            } else {
                self.hook.code = 0;
            }
            self.spaces = (self.spaces + 1).min(32);
            if self.cfg.macros != 0 {
                self.hook.macro_key.clear();
            }
            if self.cfg.upper_first != 0 && self.upper_status == 1 {
                self.upper_status = 2;
            }
            if self.spaces == 1 {
                if !self.special.is_empty() {
                    self.save_special();
                } else {
                    self.save_word();
                }
            }
            self.cfg.spelling = i32::from(self.spelling_before);
            self.temporary_off = false;
        } else if data == KEY_DELETE {
            self.hook.code = 0;
            self.hook.ext = 2;
            if !self.special.is_empty() {
                self.special.pop();
                if self.special.is_empty() {
                    self.restore_history();
                }
            } else if self.spaces > 0 {
                self.spaces -= 1;
                if self.spaces == 0 {
                    self.restore_history();
                }
            } else {
                self.raw.pop();
                if self.len > 0 {
                    self.len -= 1;
                    if let Some(x) = self.long_word.pop() {
                        self.word.copy_within(0..31, 1);
                        self.word[0] = x;
                        self.len += 1;
                    }
                    if self.cfg.spelling != 0 {
                        self.spelling(false);
                    }
                }
                if self.cfg.macros != 0 {
                    self.hook.macro_key.pop();
                }
                self.hook.backspaces = 0;
                self.hook.count = 0;
                self.hook.ext = 2;
                if self.len == 0 {
                    self.new_session();
                    self.special.clear();
                    self.restore_history();
                } else {
                    self.grammar(1);
                }
            }
        } else {
            if self.temporary_off {
                self.hook.code = 0;
                self.hook.ext = 3;
                return;
            }
            if self.spaces > 0 {
                self.hook.ext = 0;
                self.new_session();
                self.history.push(vec![KEY_SPACE; self.spaces]);
                self.spaces = 0;
            } else if !self.special.is_empty() {
                self.save_special();
            }
            if self.raw.len() == 32 {
                self.raw.remove(0);
            }
            self.raw.push(data | if caps { CAPS_MASK } else { 0 });
            let special = if self.cfg.input_type == 1 {
                [
                    KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0,
                ]
                .contains(&data)
            } else {
                [
                    KEY_W, KEY_E, KEY_R, KEY_O, KEY_A, KEY_S, KEY_D, KEY_F, KEY_J, KEY_Z, KEY_X,
                ]
                .contains(&data)
                    || (self.cfg.input_type == 0
                        && [KEY_LEFT_BRACKET, KEY_RIGHT_BRACKET].contains(&data))
            };
            if !special || self.disabled {
                if self.cfg.quick_telex != 0 && self.len > 0 && self.ch(self.len - 1) == data {
                    if let Some((_, pair)) =
                        _quickTelex.iter().find(|(k, _)| *k == data && *k != KEY_U)
                    {
                        self.hook.code = 1;
                        self.hook.backspaces = 1;
                        self.hook.count = 2;
                        self.hook.chars[1] = pair[0] | if caps { CAPS_MASK } else { 0 };
                        self.hook.chars[0] = pair[1] | if caps { CAPS_MASK } else { 0 };
                        self.insert(pair[1], caps, false);
                        return;
                    }
                }
                self.hook.code = 0;
                self.hook.backspaces = 0;
                self.hook.count = 0;
                self.hook.ext = 3;
                self.insert(data, caps, true);
            } else {
                self.hook.code = 0;
                self.hook.ext = 3;
                self.main_key(data, caps);
            }
            if self.cfg.free_mark == 0 && data != self.processing()[9] {
                self.grammar(if self.hook.code == 0 { -1 } else { 0 });
            }
            if self.hook.code == 3 {
                self.insert(data, caps, true);
                self.raw.pop();
            }
            if self.cfg.macros != 0 {
                if self.hook.code == 0 {
                    if self.hook.macro_key.len() < 255 {
                        self.hook
                            .macro_key
                            .push(data | if caps { CAPS_MASK } else { 0 });
                    }
                } else if self.hook.code == 1 || self.hook.code == 3 {
                    for _ in 0..self.hook.backspaces {
                        self.hook.macro_key.pop();
                    }
                    let start = self.len.saturating_sub(self.hook.backspaces as usize);
                    for i in start..(start + self.hook.count as usize).min(32) {
                        if self.hook.macro_key.len() < 255 {
                            self.hook.macro_key.push(self.w(i));
                        }
                    }
                }
            }
            if self.cfg.upper_first != 0 {
                if self.len == 1 && self.upper_status == 2 && self.w(0) & CAPS_MASK == 0 {
                    self.hook.code = 1;
                    self.hook.backspaces = 0;
                    self.hook.count = 1;
                    self.word[0] |= CAPS_MASK;
                    self.hook.chars[0] = self.character(self.w(0));
                    if let Some(first) = self.hook.macro_key.first_mut() {
                        *first |= CAPS_MASK;
                    }
                }
                self.upper_status = 0;
            }
            if [KEY_LEFT_BRACKET, KEY_RIGHT_BRACKET].contains(&data)
                && ([KEY_LEFT_BRACKET, KEY_RIGHT_BRACKET]
                    .contains(&(self.hook.chars[0] & CHAR_MASK))
                    || [2, 3].contains(&self.cfg.input_type))
            {
                if self.len
                    > if self.hook.code == 1 {
                        self.hook.backspaces as usize
                    } else {
                        0
                    }
                {
                    self.len -= 1;
                    self.save_word();
                }
                self.len = 0;
                self.disabled = false;
                self.raw.clear();
                self.hook.ext = 3;
                self.special.push(data | if caps { CAPS_MASK } else { 0 });
            }
        }
    }
}

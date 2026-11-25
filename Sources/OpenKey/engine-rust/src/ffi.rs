use crate::{engine::Engine, text, Config};
use std::{
    panic::{catch_unwind, AssertUnwindSafe},
    slice,
};

#[repr(C)]
#[derive(Default)]
pub struct ResultState {
    pub code: u8,
    pub backspaces: u8,
    pub count: u8,
    pub ext: u8,
    pub chars: [u32; 32],
    pub spelling: i32,
}
#[repr(C)]
pub struct Buffer {
    pub data: *mut u8,
    pub len: usize,
    pub status: i32,
}
impl Buffer {
    fn fail() -> Self {
        Self {
            data: std::ptr::null_mut(),
            len: 0,
            status: 0,
        }
    }
    fn new(bytes: Vec<u8>) -> Self {
        let mut bytes = bytes.into_boxed_slice();
        let result = Self {
            data: bytes.as_mut_ptr(),
            len: bytes.len(),
            status: 1,
        };
        std::mem::forget(bytes);
        result
    }
}
/// The caller owns the handle and must serialize calls to it.
#[no_mangle]
pub extern "C" fn ok_engine_create() -> *mut Engine {
    Box::into_raw(Box::new(Engine::default()))
}
/// # Safety
/// `engine` must be null or a live handle returned by `ok_engine_create`, freed once.
#[no_mangle]
pub unsafe extern "C" fn ok_engine_destroy(engine: *mut Engine) {
    if !engine.is_null() {
        unsafe {
            drop(Box::from_raw(engine));
        }
    }
}
/// # Safety
/// All non-null pointers must be live, aligned, and exclusively accessible for this call.
#[no_mangle]
pub unsafe extern "C" fn ok_engine_process(
    engine: *mut Engine,
    config: *const Config,
    event: u32,
    state: u32,
    key: u32,
    caps: u32,
    control: i32,
    english: i32,
    out: *mut ResultState,
) -> i32 {
    if engine.is_null() || config.is_null() || out.is_null() {
        return 0;
    }
    let config = unsafe { *config };
    if !config.valid() || event > 1 || state > 3 || key > 256 || caps > 2 {
        unsafe {
            *out = ResultState::default();
        }
        return 0;
    }
    let engine = unsafe { &mut *engine };
    let result = catch_unwind(AssertUnwindSafe(|| {
        engine.cfg = config;
        if english != 0 {
            engine.english(state, key, caps != 0, control != 0);
        } else {
            engine.event(event, state, key, caps, control != 0);
        }
        let h = &engine.hook;
        unsafe {
            *out = ResultState {
                code: h.code,
                backspaces: h.backspaces,
                count: h.count,
                ext: h.ext,
                chars: h.chars,
                spelling: engine.cfg.spelling,
            };
        }
    }));
    if result.is_err() {
        engine.new_session();
        unsafe {
            *out = ResultState::default();
        }
        return 0;
    }
    1
}
/// # Safety
/// Handle and configuration must be live and exclusively accessible.
#[no_mangle]
pub unsafe extern "C" fn ok_engine_reset(
    engine: *mut Engine,
    config: *const Config,
    full: i32,
) -> i32 {
    if engine.is_null() || config.is_null() {
        return 0;
    }
    let config = unsafe { *config };
    if !config.valid() {
        return 0;
    }
    let engine = unsafe { &mut *engine };
    engine.cfg = config;
    if full != 0 {
        engine.reset();
    } else {
        engine.new_session();
    }
    1
}
/// # Safety
/// Handle must be live and exclusively accessible. Action 0 changes spelling baseline; 1 pauses composition.
#[no_mangle]
pub unsafe extern "C" fn ok_engine_option(engine: *mut Engine, action: u32, value: i32) {
    if let Some(engine) = unsafe { engine.as_mut() } {
        match action {
            0 => engine.spelling_before = value != 0,
            1 => engine.temporary_off = value != 0,
            _ => {}
        }
    }
}
#[no_mangle]
pub extern "C" fn ok_character_code(value: u32, code: u32) -> u32 {
    text::character_code(value, code as usize)
}
#[no_mangle]
pub extern "C" fn ok_key_character(value: u32) -> u32 {
    text::key_character(value)
}

/// # Safety
/// Handle is exclusive. `input` must cover `len` readable bytes (or be null when len is zero).
/// Returned buffer must be released with `ok_buffer_free`, including empty successful results.
#[no_mangle]
pub unsafe extern "C" fn ok_engine_command(
    engine: *mut Engine,
    operation: u32,
    input: *const u8,
    len: usize,
    code: u32,
    flags: u32,
) -> Buffer {
    if engine.is_null() || len > 64 * 1024 * 1024 || (input.is_null() && len != 0) || code >= 5 {
        return Buffer::fail();
    }
    let engine = unsafe { &mut *engine };
    let input = if len == 0 {
        &[]
    } else {
        unsafe { slice::from_raw_parts(input, len) }
    };
    let result = catch_unwind(AssertUnwindSafe(|| -> Option<Vec<u8>> {
        let string = || std::str::from_utf8(input).ok();
        match operation {
            0 => {
                if input.is_empty() {
                    engine.macros = text::Macros::default();
                    Some(vec![])
                } else {
                    engine.macros.load(input).then(Vec::new)
                }
            }
            1 | 5 => Some(engine.macros.save(code as usize)),
            2 => {
                let (key, value) = string()?.split_once('\0')?;
                engine.macros.add(key, value).then(Vec::new)
            }
            3 => engine.macros.0.remove(string()?).map(|_| vec![]),
            4 => engine.macros.0.contains_key(string()?).then(Vec::new),
            6 => engine.macros.import(string()?, flags != 0).then(Vec::new),
            7 => Some(engine.macros.export().into_bytes()),
            8 => {
                let next = if input.is_empty() {
                    Default::default()
                } else {
                    text::load_apps(input)?
                };
                engine.apps = next;
                Some(vec![])
            }
            9 => Some(text::save_apps(&engine.apps)),
            10 => {
                let name = string()?;
                if name.is_empty() || name.len() > 255 || flags > 9 {
                    return None;
                }
                let value = if let Some(value) = engine.apps.get(name) {
                    i32::from(*value)
                } else {
                    if engine.apps.len() >= 65535 {
                        return None;
                    }
                    engine.apps.insert(name.to_string(), flags as u8);
                    -1
                };
                Some(value.to_le_bytes().to_vec())
            }
            11 => {
                let name = string()?;
                if name.is_empty()
                    || name.len() > 255
                    || flags > 9
                    || engine.apps.len() >= 65535 && !engine.apps.contains_key(name)
                {
                    return None;
                }
                engine.apps.insert(name.to_string(), flags as u8);
                Some(vec![])
            }
            12 => {
                let from = (flags >> 24) as usize;
                Some(text::convert(string()?, from, code as usize, flags & 0xffffff).into_bytes())
            }
            13 => {
                if input.len() % 4 != 0 {
                    return None;
                }
                let key: Vec<u32> = input
                    .as_chunks::<4>()
                    .0
                    .iter()
                    .map(|x| u32::from_le_bytes(*x))
                    .collect();
                engine
                    .macros
                    .find(&key, code as usize, flags != 0)
                    .map(|data| data.into_iter().flat_map(u32::to_le_bytes).collect())
            }
            14 => Some(
                engine
                    .hook
                    .macro_data
                    .iter()
                    .flat_map(|x| x.to_le_bytes())
                    .collect(),
            ),
            15 => Some(
                text::encode(string()?, code as usize)
                    .into_iter()
                    .flat_map(u32::to_le_bytes)
                    .collect(),
            ),
            _ => None,
        }
    }));
    match result {
        Ok(Some(bytes)) => Buffer::new(bytes),
        _ => Buffer::fail(),
    }
}
/// # Safety
/// Buffer must be exactly an unmodified return value from `ok_engine_command`, freed once.
#[no_mangle]
pub unsafe extern "C" fn ok_buffer_free(buffer: Buffer) {
    if !buffer.data.is_null() {
        unsafe {
            drop(Box::from_raw(std::ptr::slice_from_raw_parts_mut(
                buffer.data,
                buffer.len,
            )));
        }
    }
}

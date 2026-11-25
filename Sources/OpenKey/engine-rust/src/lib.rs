//! Vietnamese composition core for the macOS OpenKey bridge. GPL-3.0-only.
#![deny(unsafe_op_in_unsafe_fn)]
mod engine;
mod ffi;
#[allow(dead_code)]
mod keys;
#[allow(non_upper_case_globals, clippy::type_complexity)]
mod tables;
mod text;
pub use ffi::*;

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct Config {
    pub input_type: i32,
    pub code_table: i32,
    pub spelling: i32,
    pub quick_telex: i32,
    pub modern: i32,
    pub restore_invalid: i32,
    pub macros: i32,
    pub auto_caps_macro: i32,
    pub free_mark: i32,
    pub upper_first: i32,
    pub allow_consonants: i32,
    pub quick_start: i32,
    pub quick_end: i32,
}
impl Config {
    fn valid(&self) -> bool {
        (0..4).contains(&self.input_type) && (0..5).contains(&self.code_table)
    }
}

#[cfg(test)]
mod tests;

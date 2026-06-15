//
//  EnglishDetect.h
//  OpenKey
//
//  Lightweight English-vs-Vietnamese word detection used to skip diacritics
//  when an English word is being typed in Vietnamese (Telex) mode.
//
//  Both dictionaries are keyed by the *raw keystroke string* (lowercase a-z):
//   - English dictionary: common English words.
//   - Vietnamese dictionary: the Telex spelling of valid Vietnamese words
//     (e.g. "cu3"-style is NOT used; Telex "cur" -> "củ").
//  This lets the engine compare what was typed against both sets directly,
//  without having to materialise the toned output.
//

#ifndef EnglishDetect_h
#define EnglishDetect_h

#include <string>
#include "DataType.h"

/**
 * Load the English word list from a memory buffer (tokens separated by any
 * non-letter byte, e.g. newlines). Safe to call again to replace the data.
 */
void initEnglishDict(const Byte* pData, const int& size);

/**
 * Load the Vietnamese word list (Telex spellings) from a memory buffer.
 */
void initVietByTelexDict(const Byte* pData, const int& size);

/**
 * True once the English dictionary has at least one entry.
 */
bool isEnglishDictReady(void);

/**
 * Exact match: `word` is a complete English word.
 */
bool isEnglishWord(const std::string& word);

/**
 * `word` is a strict prefix of some longer English word (word itself excluded).
 */
bool isEnglishPrefix(const std::string& word);

/**
 * Exact match: `word` (raw Telex keystrokes) spells a valid Vietnamese word.
 */
bool isVietByTelex(const std::string& word);

/**
 * `word` is a prefix of some Vietnamese Telex spelling (word itself included).
 */
bool isVietByTelexPrefix(const std::string& word);

#endif /* EnglishDetect_h */

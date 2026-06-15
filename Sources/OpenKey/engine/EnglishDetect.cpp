//
//  EnglishDetect.cpp
//  OpenKey
//

#include "EnglishDetect.h"

#include <vector>
#include <algorithm>

using namespace std;

static vector<string> _engDict;       // sorted, unique
static vector<string> _vietDict;      // sorted, unique (Telex spellings)

// Parse a raw byte buffer into a sorted/unique lowercase word list.
// Tokens are split on any non a-z/A-Z byte; uppercase is folded to lowercase.
static void buildDict(const Byte* pData, const int& size, vector<string>& out) {
    out.clear();
    if (pData == NULL || size <= 0)
        return;
    string cur;
    cur.reserve(32);
    for (int i = 0; i <= size; i++) {
        char c = (i < size) ? (char)pData[i] : '\n'; // force flush of last token
        if (c >= 'A' && c <= 'Z')
            c = (char)(c - 'A' + 'a');
        if (c >= 'a' && c <= 'z') {
            cur.push_back(c);
        } else {
            if (!cur.empty()) {
                out.push_back(cur);
                cur.clear();
            }
        }
    }
    sort(out.begin(), out.end());
    out.erase(unique(out.begin(), out.end()), out.end());
}

void initEnglishDict(const Byte* pData, const int& size) {
    buildDict(pData, size, _engDict);
}

void initVietByTelexDict(const Byte* pData, const int& size) {
    buildDict(pData, size, _vietDict);
}

bool isEnglishDictReady(void) {
    return !_engDict.empty();
}

static bool dictContains(const vector<string>& dict, const string& w) {
    return binary_search(dict.begin(), dict.end(), w);
}

// True if some entry has `w` as a strict prefix (longer than w).
static bool dictHasLongerPrefix(const vector<string>& dict, const string& w) {
    if (w.empty())
        return false;
    vector<string>::const_iterator it = lower_bound(dict.begin(), dict.end(), w);
    for (; it != dict.end(); ++it) {
        if (it->size() < w.size() || it->compare(0, w.size(), w) != 0)
            break; // no longer entries start with w
        if (it->size() > w.size())
            return true;
    }
    return false;
}

// True if some entry has `w` as a prefix (including an exact match).
static bool dictHasPrefix(const vector<string>& dict, const string& w) {
    if (w.empty())
        return false;
    vector<string>::const_iterator it = lower_bound(dict.begin(), dict.end(), w);
    return it != dict.end() && it->size() >= w.size() && it->compare(0, w.size(), w) == 0;
}

bool isEnglishWord(const string& word) {
    return dictContains(_engDict, word);
}

bool isEnglishPrefix(const string& word) {
    return dictHasLongerPrefix(_engDict, word);
}

bool isVietByTelex(const string& word) {
    return dictContains(_vietDict, word);
}

bool isVietByTelexPrefix(const string& word) {
    return dictHasPrefix(_vietDict, word);
}

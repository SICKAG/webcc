#ifndef WEBCC_STRING_H_
#define WEBCC_STRING_H_

#include <string>
#include <string_view>
#include <vector>

namespace webcc {

// Generates randomly an ASCII string in the given length.
std::string RandomAsciiString(std::size_t length);

// Convert string to size_t.
// Just a wrapper of std::stoul.
bool ToSizeT(const std::string& str, int base, std::size_t* size);

// Trim spaces (or any of the given chars).
void Trim(std::string_view& sv, const char* spaces = " \t");

inline std::string_view Unquote(std::string_view sv) {
  Trim(sv, "\"");
  return sv;
}

// Split string without copy.
// `compress_token` is the same as boost::token_compress_on for boost::split.
void Split(std::string_view input, char delim, bool compress_token,
           std::vector<std::string_view>* output);

// Split a key-value string.
// E.g., split "Connection: Keep-Alive".
bool SplitKV(std::string_view input, char delim, bool trim_spaces,
             std::string_view* key, std::string_view* value);

// Split a key-value string.
// E.g., split "Connection: Keep-Alive".
bool SplitKV(std::string_view input, char delim, bool trim_spaces,
             std::string* key, std::string* value);

// TODO
#if (defined(_WIN32) || defined(_WIN64))
std::string Utf16To8(const std::wstring& utf16_string);
std::wstring Utf8To16(const std::string& utf8_string);
#endif

}  // namespace webcc

#endif  // WEBCC_STRING_H_

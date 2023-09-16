#include <cstdio>
#include <iostream>
#include <memory>
#include <bitset>
#include <netinet/in.h>
#include <sstream>
#include <type_traits>
using namespace std;


std::string UrlDecode(const std::string& str, bool space_as_plus)
{
    std::ostringstream decoded;
    char               decoded_char;
    int                hex_value;

    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%') {
            if (i + 2 < str.length() && std::isxdigit(str[i + 1]) && std::isxdigit(str[i + 2])) {
                // Decode a percent-encoded character
                std::istringstream hex_str(str.substr(i + 1, 2));
                hex_str >> std::hex >> hex_value;
                decoded_char = static_cast<char>(hex_value);
                decoded << decoded_char;
                i += 2;   // Skip the two hex digits
            }
            else {
                // Malformed percent encoding, just keep the '%' character
                decoded << '%';
            }
        }
        else if (space_as_plus && str[i] == '+') {
            // Decode '+' as space if space_as_plus is true
            decoded << ' ';
        }
        else {
            // Keep the character as is
            decoded << str[i];
        }
    }

    return decoded.str();
}


int main() {
    std::string str = "http://zh.wikipedia.org/wiki/%E6%98%A5%E8%8A%82";
    std::cout << UrlDecode(str,true) << std::endl;

    return 0;
}
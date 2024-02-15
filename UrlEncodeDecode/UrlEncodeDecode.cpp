#include "UrlEncodeDecode.h"

std::string url_decode(const std::string& encoded)
{
	std::string res;
	std::istringstream iss(encoded);
	char ch;

	while (iss.get(ch)) {
		if (ch == '%') {
			int hex;
			iss >> std::hex >> hex;
			res += static_cast<char>(hex);
		}
		else {
			res += ch;
		}
	}

	return res;
}

std::string url_encode(const std::string& decoded)
{
	std::ostringstream escaped;
	escaped.fill('0');
	escaped << std::hex;
	for (auto i = decoded.begin(); i != decoded.end(); ++i) {
		std::string::value_type c = (*i);
		// Keep alphanumeric and other accepted characters intact
		if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == ':' || c == '/') {
			escaped << c;
			continue;
		}
		// Any other characters are percent-encoded
		escaped << std::uppercase;
		escaped << '%' << std::setw(2) << int((unsigned char)c);
		escaped << std::nouppercase;
	}
	return escaped.str();
}

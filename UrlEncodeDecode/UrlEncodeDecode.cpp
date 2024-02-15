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

std::string url_encode(const std::string& decoted)
{
	std::string new_str = "";
	char c;
	int ic;
	const char* chars = decoted.c_str();
	char bufHex[10];
	int len = strlen(chars);

	for (int i = 0; i < len; i++) {
		c = chars[i];
		ic = c;
		// uncomment this if you want to encode spaces with +
		/*if (c==' ') new_str += '+';
		else */if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') new_str += c;
		else {
			sprintf(bufHex, "%X", c);
			if (ic < 16)
				new_str += "%0";
			else
				new_str += "%";
			new_str += bufHex;
		}
	}
	return new_str;
}

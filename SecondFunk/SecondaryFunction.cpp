#include "SecondaryFunction.h"

#if _WIN32		// Windows
const int errorsetmodeout = _setmode(_fileno(stdout), _O_U16TEXT);
const int errorsetmodeinp = _setmode(_fileno(stdin), _O_U16TEXT);
const int errorsetmodeerr = _setmode(_fileno(stderr), _O_U16TEXT);
#endif

// цвет в консоли
void consoleCol(COLOR color)
{
#ifdef _WIN32
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
#elif __GNUC__
	std::wcout << color;
#endif
}

void setRuLocale()
{
	setlocale(LC_ALL, "ru_RU.utf8");
}

void consoleClear()
{
#ifdef _WIN32
	std::system("cls");
#elif __GNUC__
	std::system("clear");
#endif
}

// Convert an ANSI string to a wide Unicode String
// https://gist.github.com/rosasurfer/33f0beb4b10ff8a8c53d943116f8a872#file-conversion-cpp
std::wstring ansi2wideUtf(const std::string& str)
{
#ifdef _WIN32
	int size_needed = MultiByteToWideChar(CP_ACP, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_ACP, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
#elif __GNUC__
	// convert UTF-8 string to wstring
	std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    return myconv.from_bytes(str);
#endif
}

// Convert a wide Unicode string to an UTF8 string
std::string wideUtf2utf8(const std::wstring& wstr)
{
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

// Convert an UTF8 string to a wide Unicode String
std::wstring utf82wideUtf(const std::string &str)
{
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

// Convert an wide Unicode string to ANSI string
std::string wideUtf2ansi(const std::wstring &wstr)
{
	int size_needed = WideCharToMultiByte(CP_ACP, 0, &wstr[0], -1, NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_ACP, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

std::wostream& operator<<(std::wostream& out, const std::string str)
{
	out << utf82wideUtf(str);
	return out;
}

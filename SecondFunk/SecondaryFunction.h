#pragma once

#include <iostream>

#if __GNUC__ >= 4	// Linux
#include <codecvt>
#include <locale>

using COLOR = const char*;
namespace col
{
	const COLOR cancel("\033[0m");		//отменить цвет

	const COLOR black("\033[30m");		//черный
	const COLOR red("\033[31m");		//красный
	const COLOR green("\033[32m");		//зеленый
	const COLOR yellow("\033[33m");		//желтый
	const COLOR blue("\033[34m");		//синий
	const COLOR magenta("\033[35m");	//фиолетовый
	const COLOR cyan("\033[36m");		//голубой
	const COLOR white("\033[37m");		//белый
	const COLOR br_black("\033[90m");	//серый
	const COLOR br_red("\033[91m");		//ярк. красный
	const COLOR br_green("\033[92m");	//ярк. зеленый
	const COLOR br_yellow("\033[93m");	//ярк. желтый
	const COLOR br_blue("\033[94m");	//ярк. синий
	const COLOR br_magenta("\033[95m");	//ярк. фиолетовый
	const COLOR br_cyan("\033[96m");	//ярк. голубой
	const COLOR br_white("\033[97m");	//ярк. белый
}

#elif _WIN32		// Windows
#include <winsock2.h>	// для корректной компиляции с Boost
#include <Windows.h>
#include <io.h>
#include <fcntl.h>

extern const int errorsetmodeout;
extern const int errorsetmodeinp;
extern const int errorsetmodeerr;

using COLOR = WORD;
namespace col
{
	const COLOR cancel(7);		//белый

	const COLOR black(0);		//черный
	const COLOR blue(1);		//синий
	const COLOR green(2);		//зеленый
	const COLOR cyan(3);		//голубой
	const COLOR red(4);			//красный
	const COLOR magenta(5);		//фиолетовый
	const COLOR yellow(6);		//желтый
	const COLOR white(7);		//белый
	const COLOR br_black(8);	//серый
	const COLOR br_blue(9);		//ярк. синий
	const COLOR br_green(10);	//ярк. зеленый
	const COLOR br_cyan(11);	//ярк. голубой
	const COLOR br_red(12);		//ярк. красный
	const COLOR br_magenta(13);	//ярк. фиолетовый
	const COLOR br_yellow(14);	//ярк. желтый
	const COLOR br_white(15);	//ярк. белый
}

#else
#error "-= Unknow OS =-"
#endif

// цвет в консоли
void consoleCol(COLOR);

// получение текущего положения курсора
COORD getConsolePos();

// установка положения курсора
void setConsolePos(const COORD);

void setRuLocale();

void consoleClear();

// Convert an ANSI string to a wide Unicode String
std::wstring ansi2wideUtf(const std::string&);

// Convert a wide Unicode string to an UTF8 string
std::string wideUtf2utf8(const std::wstring& wstr);

// Convert an UTF8 string to a wide Unicode String
std::wstring utf82wideUtf(const std::string& str);

// Convert an wide Unicode string to ANSI string
std::string wideUtf2ansi(const std::wstring& wstr);

// перегружаю оператор << для wcout (чтобы выводить std::string)
std::wostream& operator<< (std::wostream& out, const std::string str);

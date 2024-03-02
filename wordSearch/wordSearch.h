#pragma once

#include <iostream>
#include <regex>
#include <mutex>
#include <boost/locale.hpp> // перед "SecondaryFunction.h"
#include "SecondaryFunction.h"
#include "../Types.h"

using Link = struct {
	std::string link_str;
	unsigned int recLevel;
};
using LinkList = std::list<Link>;

namespace WordSearch
{
	std::pair<WordMap, LinkList> getWordLink(
		std::wstring page, unsigned int recLevel);
};

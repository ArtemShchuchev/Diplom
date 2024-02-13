#pragma once

#include <iostream>
#include <regex>
#include <boost/locale.hpp> // перед "SecondaryFunction.h"
#include "SecondaryFunction.h"
#include "../Types.h"

using Link = struct {
	std::string link_str;
	unsigned int recLevel;
};
using LinkList = std::list<Link>;


class WordSearch
{
private:
	static const std::wregex
		space_reg,
		body_reg,
		url_reg,
		title_reg,
		token_reg,
		punct_reg,
		number_reg;

public:
	std::pair<WordMap, LinkList> getWordLink(std::wstring page, unsigned int recLevel);
};

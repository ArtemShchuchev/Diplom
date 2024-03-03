#include "wordSearch.h"

const std::wregex
    title_reg{ LR"(<title>(.+)< ?/ ?tit)", std::regex::icase },
    url_reg{ LR"!!(<a href="([^"]+)")!!", std::regex::icase },
    token_reg{ LR"(<[^>]*>)" },
    word_reg{ LR"([^[:alpha:]]?([[:alpha:]]+)[^[:alpha:]]?)" };

std::pair<WordMap, LinkList> WordSearch::getWordLink(
    std::wstring page, unsigned int recLevel)
{
    static std::mutex wregexLock;
    LinkList links;

    // Ищу title
    std::wstring title;
    std::unique_lock<std::mutex> ul_parse(wregexLock);
    auto itTitle = std::wsregex_token_iterator(page.begin(), page.end(), title_reg, 1);
    if (itTitle != std::wsregex_token_iterator{}) title = *itTitle;
    ul_parse.unlock();
    
    // Ищу body, если нашел, то и ссылки
    auto startBody(page.find(L"<bo"));
    auto endBody(page.rfind(L"</bo"));
    if (startBody != std::wstring::npos && endBody != std::wstring::npos) {
        page = { (page.begin() + startBody),
            (page.begin() + endBody + 7) };
        
        // Ищу ссылки
        if (recLevel > 1)
        {
            --recLevel; // следующая глубина погружения
            ul_parse.lock();
            auto it_start(std::wsregex_token_iterator{ page.begin(), page.end(), url_reg, 1 });
            auto it_end(std::wsregex_token_iterator{});
            for (auto it(it_start); it != it_end; ++it)
            {
                std::wstring link_ws(*it);
                std::string link_str(wideUtf2utf8(link_ws));
                links.push_back({ link_str, recLevel });
            }
            ul_parse.unlock();
        }
    }
    else {
        page.clear();
    }

    // добавил к body, title
    page += std::move(title);
    
    // Убрал токены
    ul_parse.lock();
    page = std::regex_replace(page, token_reg, L" ");
    
    // Строку в нижний регистр
    // Create system default locale
    boost::locale::generator gen;
    std::locale loc = gen("");
    // Make it system global
    std::locale::global(loc);
    page = boost::locale::to_lower(page);

    // Ищу слова
    auto it_start = std::wsregex_token_iterator{ page.begin(), page.end(), word_reg, 1 };
    auto it_end = std::wsregex_token_iterator{};
    WordMap wordmap;
    for (auto it(it_start); it != it_end; ++it) {
        if ((*it).length() > 2 && (*it).length() < 33) {
            ++wordmap[*it];
        }
    }
    ul_parse.unlock();

    return { wordmap, links };
}

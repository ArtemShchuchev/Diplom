#pragma once
#include <iostream>
#include <pqxx/pqxx>
#include <exception>
#include <memory>
#include <vector>
#include <unordered_set>
#include "SecondaryFunction.h"
#include "../Types.h"

struct ConnectData {
	std::string host;
	unsigned port;
	std::string dbname;
	std::string username;
	std::string password;
};
struct IdWordAm {
	int id;
	std::string word;
	int amount;
};
using idWordAm_vec = std::vector<IdWordAm>;

class Clientdb
{
private:
	std::unique_ptr<pqxx::connection> connect;

	void createTable();
	bool is_open();
	void delFromLinkWords(const int linkId);
	void deleteNotUseWord();
	int createLink(const std::string& link);
	void getIdWord(idWordAm_vec& idWordAm);

public:
	Clientdb(const ConnectData& data);
	std::wstring dbname();
	int addLink(const std::string& link);
	void deleteLink(const int id);
	int getIdLink(const std::string& link);
	idWordAm_vec addWords(const WordMap wordAmount);
	void addLinkWords(const int id, const idWordAm_vec& idWordAm);
	std::unordered_map<std::string, unsigned> getLinkAmount(
		const std::vector<std::string>& words);
	std::unordered_set<std::string> getLinks();
};

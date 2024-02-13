#include "Clientdb.h"

Clientdb::Clientdb(const ConnectData& data)
{
	connect = std::make_unique<pqxx::connection>(
		"host=" + data.host +
		" port=" + std::to_string(data.port) +
		" dbname=" + data.dbname +
		" user=" + data.username +
		" password=" + data.password);
	
	if (!is_open()) throw std::runtime_error("Не удалось подключиться к БД");
	createTable();
}

bool Clientdb::is_open()
{
	return connect->is_open();
}

void Clientdb::deleteLink(const int id)
{
	const std::string DEL_START_STR{
		"delete from links "
		"where id = '" };
	const std::string DEL_END_STR{ "';" };

	pqxx::work tx{ *connect };
	tx.exec(DEL_START_STR + std::to_string(id) + DEL_END_STR);
	tx.commit();
}

void Clientdb::delFromLinkWords(const int linkId)
{
	const std::string DEL_START_STR{
		"delete from link_word "
		"where link_id = '" };
	const std::string DEL_END_STR{ "';" };

	pqxx::work tx{ *connect };
	tx.exec(DEL_START_STR + std::to_string(linkId) + DEL_END_STR);
	tx.commit();
}

void Clientdb::deleteNotUseWord()
{
	const std::string DEL_STR{
		"delete from words "
		"where id <> all(select distinct word_id from link_word);" };

	pqxx::work tx{ *connect };
	tx.exec(DEL_STR);
	tx.commit();
}

int Clientdb::createLink(const std::string& link)
{
	const std::string CREATE_START_STR{
		"INSERT into links (link) "
		"VALUES('" };
	const std::string CREATE_END_STR{ "') returning id;" };

	pqxx::work tx{ *connect };
	auto row = tx.exec1(CREATE_START_STR + tx.esc(link) + CREATE_END_STR);
	tx.commit();

	return row[0].as<int>();
}

void Clientdb::getIdWord(idWordAm_vec& idWordAm)
{
	const std::string SQL_REQUEST{
		"select id from words "
		"where word = $1;" };

	connect->prepare("get_id", SQL_REQUEST);

	pqxx::work tx{ *connect };
	for (auto& struc : idWordAm)
	{
		if (struc.id) continue;
		auto row = tx.exec_prepared("get_id", tx.esc(struc.word));
		auto option = row[0][0].as<std::optional<int>>();
		struc.id = option.has_value() ? *option : 0;
	}
	tx.commit();
}

std::wstring Clientdb::dbname()
{
	return utf82wideUtf(connect->dbname());
}

int Clientdb::addLink(const std::string& link)
{
	/*
	int id = getIdLink(link);
	if (id)
	{
		deleteLink(id);
		deleteNotUseWord();
	}
	return createLink(link);
	*/
	int id = getIdLink(link);
	if (id == 0) id = createLink(link);
	else delFromLinkWords(id);
	return id;
}

int Clientdb::getIdLink(const std::string& link)
{
	const std::string SEL_START_STR{
		"select id from links "
		"where link = '" };
	const std::string SEL_END_STR{ "';" };

	pqxx::work tx{ *connect };
	auto row = tx.exec(SEL_START_STR + tx.esc(link) + SEL_END_STR);
	tx.commit();

	auto id_opt = row[0][0].as<std::optional<int>>();
	int id = id_opt.has_value() ? *id_opt : 0;

	return id;
}

idWordAm_vec Clientdb::addWords(const WordMap wordAmount)
{
	const std::string SQL_REQUEST{
		"INSERT into words (word) "
		"VALUES($1) on conflict (word) do nothing returning id;" };
	bool wordExist(false);

	connect->prepare("insert_words", SQL_REQUEST);

	idWordAm_vec idWordAm(wordAmount.size());
	auto it = idWordAm.begin();
	pqxx::work tx{ *connect };
	for (auto& [w, amount] : wordAmount)
	{
		std::string word = wideUtf2utf8(w);
		auto row = tx.exec_prepared("insert_words", tx.esc(word));
		auto option = row[0][0].as<std::optional<int>>();
		int id_word(option.has_value() ? *option : 0);
		if (id_word == 0) wordExist = true;
		*it = { id_word, word, amount };
		++it;
	}
	tx.commit();

	if (wordExist) getIdWord(idWordAm);

	return idWordAm;
}

void Clientdb::addLinkWords(const int id, const idWordAm_vec& idWordAm)
{
	const std::string SQL_REQUEST{
		"INSERT into link_word (link_id, word_id, amount) "
		"VALUES($1, $2, $3);" };

	connect->prepare("insert_link_word", SQL_REQUEST);

	pqxx::work tx{ *connect };
	for (auto& struc : idWordAm)
	{
		tx.exec_prepared("insert_link_word", id, struc.id, struc.amount);
	}
	tx.commit();
}

std::unordered_map<std::string, unsigned> Clientdb::getLinkAmount(
	const std::vector<std::string>& words)
{
	const std::string SQL_REQUEST{
		"SELECT l.link, lw.amount FROM link_word lw "
		"JOIN links l ON l.id = lw.link_id "
		"JOIN words w ON w.id = lw.word_id "
		"WHERE w.word = $1;" };

	connect->prepare("get_link_amount", SQL_REQUEST);

	std::unordered_map<std::string, unsigned> LinkAmount;
	pqxx::work tx{ *connect };
	for (auto& w : words)
	{
		auto row = tx.exec_prepared("get_link_amount", tx.esc(w));
		
		for (const auto& next : row) {
			auto optionLink = next[0].as<std::optional<std::string>>();
			auto optionAmount = next[1].as<std::optional<unsigned>>();

			if (optionLink.has_value()) {
				unsigned am = optionAmount.has_value() ? *optionAmount : 0;
				LinkAmount[*optionLink] += am;
			}
		}
	}
	tx.commit();

	return LinkAmount;
}

void Clientdb::createTable()
{
	const std::string CREATE_STR{ "CREATE TABLE IF NOT EXISTS " };
	const std::string DOC_STR{
		"links "
		"(id serial primary key, "
		"link varchar unique);" };
	const std::string WORD_STR{
		"words "
		"(id serial primary key, "
		"word varchar(32) unique);" };
	const std::string DOC_WORD_STR{
		"link_word "
		"(link_id integer not null references links on update cascade on delete cascade, "
		"word_id integer not null references words on update cascade on delete cascade, "
		"amount	integer not null);" };

	pqxx::work tx{ *connect };
	tx.exec(CREATE_STR + DOC_STR);
	tx.exec(CREATE_STR + WORD_STR);
	tx.exec(CREATE_STR + DOC_WORD_STR);
	tx.commit();
}

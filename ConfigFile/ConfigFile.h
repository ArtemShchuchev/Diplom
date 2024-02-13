#pragma once

#include <iostream>
#include <fstream>
#include <boost/property_tree/ini_parser.hpp>

class ConfigFile
{
private:
	boost::property_tree::ptree config;
	std::string fileName_;

public:
	ConfigFile(std::string_view fileName);
	
	template<typename Type>
	Type getConfig(const std::string& category, const std::string& var);

	template<typename Type>
	void setConfig(const std::string& category, const Type& var);
};



template<typename Type>
inline Type ConfigFile::getConfig(const std::string& category, const std::string& var)
{
	std::ifstream fileIn(fileName_);
	Type variable{};
	if (fileIn.is_open())
	{
		boost::property_tree::read_ini(fileIn, config);
		variable = config.get<Type>(category + '.' + var);
		fileIn.close();
	}
	else throw std::runtime_error("не могу открыть файл!");
	return variable;
}

template<typename Type>
inline void ConfigFile::setConfig(const std::string& category, const Type& var)
{
	std::ofstream fileOut(fileName_);
	if (fileOut.is_open())
	{
		config.put(category, var);
		boost::property_tree::write_ini(fileOut, config);
		fileOut.close();
	}
	else throw std::runtime_error("не могу открыть файл!");
}

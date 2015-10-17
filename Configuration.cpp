#include <fstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "Configuration.hpp"

using namespace config;

namespace config {
    struct ConfigurationImpl {
        boost::property_tree::ptree ptree;
    };

    template<typename TYPE>
    TYPE get(boost::property_tree::ptree tree, const std::string &property) {
        try {
            return tree.get<TYPE>(property);
        } catch (boost::property_tree::ptree_bad_path) {
            throw ConfigException(std::string(typeid(TYPE).name()) + " key \'" + property + "\' not found");
        }
    }
}

config::Configuration::Configuration(const std::string fileName)
        : impl(new ConfigurationImpl()) {
    boost::property_tree::read_ini(fileName, impl->ptree);
}

config::Configuration::Configuration(std::vector<std::string> const fileNames)
        : impl(new ConfigurationImpl()) {
    for (auto &name : fileNames) {
        boost::property_tree::read_ini(name, impl->ptree);
    }
}

config::Configuration::~Configuration() {
    delete impl;
}

int config::Configuration::getInt(const std::string &property) {
    return get<int>(impl->ptree, property);
}

bool config::Configuration::getBool(const std::string &property) {
    return get<bool>(impl->ptree, property);
}

std::string config::Configuration::getString(const std::string &property) {
    return get<std::string>(impl->ptree, property);
}



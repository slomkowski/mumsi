#include "IncomingConnectionValidator.hpp"

#include <boost/algorithm/string.hpp>

using namespace std;

sip::IncomingConnectionValidator::IncomingConnectionValidator(std::string validUriExpression)
        : validUriExpression(validUriExpression),
          logger(log4cpp::Category::getInstance("IncomingConnectionValidator")) {

    vector<string> separateUris;
    boost::split(separateUris, validUriExpression, boost::is_any_of("\t "));
    for (auto &uri : separateUris) {
        boost::replace_all(uri, ".", "\\.");
        boost::replace_all(uri, "*", "\\w*");
        uriRegexVec.push_back(regex(uri));
    }
}

bool sip::IncomingConnectionValidator::validateUri(std::string uri) {
    regex addressRegex("[\"\\w ]*<sip:([\\w\\.]+@[\\w\\.]+)>");

    smatch s;

    if (not regex_match(uri, s, addressRegex)) {
        logger.warn("URI has invalid format: %s", uri.c_str());
        return false;
    }

    string rawAddress = s[1].str();

    for (auto &reg : uriRegexVec) {
        if (regex_match(rawAddress, s, reg)) {
            logger.info("URI %s is valid.", rawAddress.c_str());
            return true;
        }
    }

    logger.info("URI %s not valid.", rawAddress.c_str());

    return false;
}

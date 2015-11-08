#include "IncomingConnectionValidator.hpp"

sip::IncomingConnectionValidator::IncomingConnectionValidator(std::string validUriExpression)
        : validUriExpression(validUriExpression),
          logger(log4cpp::Category::getInstance("IncomingConnectionValidator")) {

}

bool sip::IncomingConnectionValidator::validateUri(std::string uri) {
    return true;
}

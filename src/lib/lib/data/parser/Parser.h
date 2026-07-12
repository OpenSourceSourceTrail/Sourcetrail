#pragma once
// STL
#include <memory>

class ParserClient;

class Parser {
public:
  Parser(std::shared_ptr<ParserClient> client);
  virtual ~Parser();

protected:
  std::shared_ptr<ParserClient> m_client;
};
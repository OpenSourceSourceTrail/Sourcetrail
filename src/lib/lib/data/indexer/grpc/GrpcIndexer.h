#pragma once
#include <string>

#include "GlobalId.hpp"

class GrpcIndexer {
public:
  GrpcIndexer(std::string engineEndpoint, Id processId);

  void work();

private:
  std::string mEngineEndpoint;
  Id mProcessId;
};

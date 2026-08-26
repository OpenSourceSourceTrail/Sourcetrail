#include "IndexerCxx.h"
// internal
#include "CxxParser.h"
#include "FileRegister.h"

void IndexerCxx::doIndex(std::shared_ptr<IndexerCommandCxx> indexerCommand,
                         std::shared_ptr<ParserClientImpl> parserClient,
                         std::shared_ptr<IndexerStateInfo> indexerStateInfo) {
  CxxParser parser(parserClient,
                   std::make_shared<FileRegister>(
                       indexerCommand->getSourceFilePath(), indexerCommand->getIndexedPaths(), indexerCommand->getExcludeFilters()),
                   indexerStateInfo);

  parser.buildIndex(indexerCommand);
}

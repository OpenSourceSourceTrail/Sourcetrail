#ifndef CXX_DIAGNOSTIC_CONSUMER
#define CXX_DIAGNOSTIC_CONSUMER

#include <clang/Basic/Diagnostic.h>

#include "FilePath.h"

class CanonicalFilePathCache;
class ParserClient;

/// Collects clang diagnostics into the project's error list.
///
/// This deliberately does not derive from clang::TextDiagnosticPrinter: the indexer must stay silent on stdout and
/// stderr, so diagnostics are only ever forwarded to the ParserClient and never formatted to a stream.
class CxxDiagnosticConsumer : public clang::DiagnosticConsumer {
public:
  CxxDiagnosticConsumer(std::shared_ptr<ParserClient> client,
                        std::shared_ptr<CanonicalFilePathCache> canonicalFilePathCache,
                        const FilePath& sourceFilePath);

  void HandleDiagnostic(clang::DiagnosticsEngine::Level level, const clang::Diagnostic& info) override;

private:
  std::shared_ptr<ParserClient> m_client;
  std::shared_ptr<CanonicalFilePathCache> m_canonicalFilePathCache;

  const FilePath m_sourceFilePath;
};

#endif    // CXX_DIAGNOSTIC_CONSUMER

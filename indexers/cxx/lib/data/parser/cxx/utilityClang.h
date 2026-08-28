#pragma once
// clang
#include <clang/AST/Decl.h>
// internal
#include "data/parser/AccessKind.h"
#include "data/parser/SymbolKind.h"

struct ParseLocation;
struct ParseLocation;
class CanonicalFilePathCache;
class FilePath;

namespace clang {
class SourceRange;
class Preprocessor;
class SourceManager;
class TypeLoc;
}    // namespace clang

namespace utility {
/**
 * Location of a TypeLoc's own name token, excluding any nested-name-specifier.
 *
 * Clang 21 removed ElaboratedType and folded the qualifier into the concrete TypeLocs, so for
 * `test::TestStruct` TypeLoc::getBeginLoc() now points at `test` rather than at `TestStruct`.
 * Sourcetrail records the name token, so it has to ask for it explicitly. Falls back to
 * getBeginLoc() for TypeLocs that carry no separate name location (builtins, pointers, ...).
 */
clang::SourceLocation getTypeNameLoc(clang::TypeLoc typeLoc);

template <typename T>
const T* getFirstDecl(const T* decl);
bool isImplicit(const clang::Decl* d);
AccessKind convertAccessSpecifier(clang::AccessSpecifier access);
SymbolKind convertTagKind(const clang::TagTypeKind tagKind);
bool isLocalVariable(const clang::VarDecl* d);
bool isLocalVariable(const clang::ValueDecl* d);
bool isParameter(const clang::VarDecl* d);
bool isParameter(const clang::ValueDecl* d);
SymbolKind getSymbolKind(const clang::VarDecl* d);
std::wstring getFileNameOfFileEntry(const clang::FileEntry* entry);

ParseLocation getParseLocation(const clang::SourceLocation& sourceLocation,
                               const clang::SourceManager& sourceManager,
                               clang::Preprocessor* preprocessor,
                               std::shared_ptr<CanonicalFilePathCache> canonicalFilePathCache);

ParseLocation getParseLocation(const clang::SourceRange& sourceRange,
                               const clang::SourceManager& sourceManager,
                               clang::Preprocessor* preprocessor,
                               std::shared_ptr<CanonicalFilePathCache> canonicalFilePathCache);
}    // namespace utility

template <typename T>
const T* utility::getFirstDecl(const T* decl) {
  const clang::Decl* ret = decl;
  {
    const clang::Decl* prev = ret;
    while(prev) {
      ret = prev;
      prev = prev->getPreviousDecl();
    }
  }
  return clang::dyn_cast_or_null<T>(ret);
}

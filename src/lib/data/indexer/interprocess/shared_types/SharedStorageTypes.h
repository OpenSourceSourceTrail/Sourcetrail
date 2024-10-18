#pragma once

#include "GlobalId.hpp"
#include "SharedMemory.h"
#include "StorageComponentAccess.h"
#include "StorageEdge.h"
#include "StorageError.h"
#include "StorageFile.h"
#include "StorageLocalSymbol.h"
#include "StorageNode.h"
#include "StorageOccurrence.h"
#include "StorageSourceLocation.h"
#include "StorageSymbol.h"
#include "utilityString.h"

// macro creating SharedStorageType from StorageType
// - arguments: StorageType & SharedStorageType
// - defines: conversion functions toShared() & fromShared()

#define CONVERT_STORAGE_TYPE_TO_SHARED_TYPE(__type__, __shared_type__)                                                           \
  typedef __type__ __shared_type__;                                                                                              \
                                                                                                                                 \
  inline const __shared_type__& toShared(const __type__& instance, [[maybe_unused]] SharedMemory::Allocator* allocator) {        \
    return instance;                                                                                                             \
  }                                                                                                                              \
                                                                                                                                 \
  inline const __type__& fromShared(const __shared_type__& instance) {                                                           \
    return instance;                                                                                                             \
  }

CONVERT_STORAGE_TYPE_TO_SHARED_TYPE(StorageEdge, SharedStorageEdge)
CONVERT_STORAGE_TYPE_TO_SHARED_TYPE(StorageSymbol, SharedStorageSymbol)
CONVERT_STORAGE_TYPE_TO_SHARED_TYPE(StorageSourceLocation, SharedStorageSourceLocation)
CONVERT_STORAGE_TYPE_TO_SHARED_TYPE(StorageOccurrence, SharedStorageOccurrence)
CONVERT_STORAGE_TYPE_TO_SHARED_TYPE(StorageComponentAccess, SharedStorageComponentAccess)


struct SharedStorageNode {
  SharedStorageNode(Id id_, int type_, const std::string& serializedName_, SharedMemory::Allocator* allocator)
      : id(id_), type(type_), serializedName(serializedName_.c_str(), allocator) {}

  Id id;
  int type;
  SharedMemory::String serializedName;
};

inline SharedStorageNode toShared(const StorageNode& node, SharedMemory::Allocator* allocator) {
  return SharedStorageNode(node.id, node.type, utility::encodeToUtf8(node.serializedName), allocator);
}

inline StorageNode fromShared(const SharedStorageNode& node) {
  return StorageNode(node.id, node.type, utility::decodeFromUtf8(node.serializedName.c_str()));
}


struct SharedStorageFile {
  SharedStorageFile(Id id_,
                    const std::string& filePath_,
                    const std::string& languageIdentifier_,
                    bool indexed_,
                    bool complete_,
                    SharedMemory::Allocator* allocator)
      : id(id_)
      , filePath(filePath_.c_str(), allocator)
      , languageIdentifier(languageIdentifier_.c_str(), allocator)
      , indexed(indexed_)
      , complete(complete_) {}

  Id id;
  SharedMemory::String filePath;
  SharedMemory::String languageIdentifier;
  bool indexed;
  bool complete;
};

inline SharedStorageFile toShared(const StorageFile& file, SharedMemory::Allocator* allocator) {
  return SharedStorageFile(file.id,
                           utility::encodeToUtf8(file.filePath),
                           utility::encodeToUtf8(file.languageIdentifier),
                           file.indexed,
                           file.complete,
                           allocator);
}

inline StorageFile fromShared(const SharedStorageFile& file) {
  return StorageFile(file.id,
                     utility::decodeFromUtf8(file.filePath.c_str()),
                     utility::decodeFromUtf8(file.languageIdentifier.c_str()),
                     "",
                     file.indexed,
                     file.complete);
}


struct SharedStorageLocalSymbol {
  SharedStorageLocalSymbol(Id id_, const std::string& name_, SharedMemory::Allocator* allocator)
      : id(id_), name(name_.c_str(), allocator) {}

  Id id;
  SharedMemory::String name;
};

inline SharedStorageLocalSymbol toShared(const StorageLocalSymbol& symbol, SharedMemory::Allocator* allocator) {
  return SharedStorageLocalSymbol(symbol.id, utility::encodeToUtf8(symbol.name), allocator);
}

inline StorageLocalSymbol fromShared(const SharedStorageLocalSymbol& symbol) {
  return StorageLocalSymbol(symbol.id, utility::decodeFromUtf8(symbol.name.c_str()));
}


struct SharedStorageError {
  SharedStorageError(Id id_,
                     const std::string& message_,
                     const std::string& translationUnit_,
                     bool fatal_,
                     bool indexed_,
                     SharedMemory::Allocator* allocator)
      : id(id_)
      , message(message_.c_str(), allocator)
      , translationUnit(translationUnit_.c_str(), allocator)
      , fatal(fatal_)
      , indexed(indexed_) {}

  Id id;
  SharedMemory::String message;
  SharedMemory::String translationUnit;
  bool fatal;
  bool indexed;
};

inline SharedStorageError toShared(const StorageError& error, SharedMemory::Allocator* allocator) {
  return SharedStorageError(error.id,
                            utility::encodeToUtf8(error.message),
                            utility::encodeToUtf8(error.translationUnit),
                            error.fatal,
                            error.indexed,
                            allocator);
}

inline StorageError fromShared(const SharedStorageError& error) {
  return StorageError(error.id,
                      utility::decodeFromUtf8(error.message.c_str()),
                      utility::decodeFromUtf8(error.translationUnit.c_str()),
                      error.fatal,
                      error.indexed);
}
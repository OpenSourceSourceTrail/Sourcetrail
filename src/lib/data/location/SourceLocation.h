#pragma once
// STL
#include <memory>
#include <ostream>
#include <string>
#include <vector>
// internal
#include "GlobalId.hpp"
#include "LocationType.h"

class FilePath;
class SourceLocationFile;

/**
 * @brief A location in source code
 * It's defined by line and colmun
 *
 * Start and End are SourceLocations
 * ┌─────────────────────────────────────┐
 * │                                     │
 * │  Start(line,column)                 │
 * │  isStart=True,other=null            │
 * │        x───────────────────┐        │
 * │        │                   │        │
 * │        └───────────────────x        │
 * │                   End(line,colmun)  │
 * │                     isStart=False   │
 * │                     other=start     │
 * └─────────────────────────────────────┘
 */
class SourceLocation final {
public:
  using Ptr = std::shared_ptr<SourceLocation>;

  SourceLocation(SourceLocationFile* pFile,
                 LocationType type,
                 Id locationId,
                 std::vector<Id> tokenIds,
                 size_t lineNumber,
                 size_t columnNumber,
                 bool isStart);

  [[deprecated]] SourceLocation(SourceLocation* pOther, size_t lineNumber, size_t columnNumber);
  static Ptr createEndSourceLocation(SourceLocation* pOther, size_t lineNumber, size_t columnNumber);

  [[deprecated]] SourceLocation(const SourceLocation* pOther, SourceLocationFile* pFile);
  static Ptr createSourceLocationFromAnother(const SourceLocation* pOther, SourceLocationFile* pFile);

  ~SourceLocation();

  bool operator==(const SourceLocation& rhs) const;

  bool operator<(const SourceLocation& rhs) const;

  bool operator>(const SourceLocation& rhs) const;

  [[nodiscard]] bool contains(const SourceLocation& other) const;

  /**
   * @name getters
   * @{ */
  [[nodiscard]] SourceLocationFile* getSourceLocationFile() const;

  [[nodiscard]] Id getLocationId() const;

  [[nodiscard]] const std::vector<Id>& getTokenIds() const;

  [[nodiscard]] LocationType getType() const;

  [[nodiscard]] size_t getColumnNumber() const;

  [[nodiscard]] size_t getLineNumber() const;

  [[nodiscard]] const FilePath& getFilePath() const;
  /**  @} */

  /**
   * @name getter/setter OtherLocation
   * @{ */
  [[nodiscard]] const SourceLocation* getOtherLocation() const;

  void setOtherLocation(SourceLocation* other);
  /**  @} */

  [[nodiscard]] const SourceLocation* getStartLocation() const;

  [[nodiscard]] const SourceLocation* getEndLocation() const;

  /**
   * @name Check which LocationType is used
   * @{ */
  [[nodiscard]] bool isStartLocation() const;

  [[nodiscard]] bool isEndLocation() const;

  [[nodiscard]] bool isScopeLocation() const;

  [[nodiscard]] bool isFullTextSearchMatch() const;
  /**  @} */

private:
  SourceLocationFile* m_file;

  LocationType m_type;

  const Id m_locationId;
  const std::vector<Id> m_tokenIds;

  const size_t m_lineNumber;
  const size_t m_columnNumber;

  SourceLocation* m_other;
  const bool m_isStart;
};

/**
 * @brief Serialize SourceLocation
 *
 * @param ostream output stream
 * @param location SourceLocation
 *
 * @return return output stream
 */
std::wostream& operator<<(std::wostream& ostream, const SourceLocation& location);
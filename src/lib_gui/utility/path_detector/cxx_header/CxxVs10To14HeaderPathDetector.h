#pragma once
#include "ApplicationArchitectureType.h"
#include "PathDetector.h"

class CxxVs10To14HeaderPathDetector : public PathDetector {
public:
  enum VisualStudioType { VISUAL_STUDIO_2010, VISUAL_STUDIO_2012, VISUAL_STUDIO_2013, VISUAL_STUDIO_2015 };

  CxxVs10To14HeaderPathDetector(VisualStudioType type, bool isExpress, ApplicationArchitectureType architecture);

private:
  std::vector<FilePath> doGetPaths() const override;

  FilePath getVsInstallPathUsingRegistry() const;

  const int m_version;
  const bool m_isExpress;
  const ApplicationArchitectureType m_architecture;
};

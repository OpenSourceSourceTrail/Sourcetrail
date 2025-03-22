#include "CxxVs10To14HeaderPathDetector.h"

#include <string>

#include <QSettings>
#include <QSysInfo>

#include "FilePath.h"
#include "logging.h"
#include "utility.h"
#include "utilityCxxHeaderDetection.h"

namespace {
constexpr int VisualStudio2010 = 10;
constexpr int VisualStudio2012 = 11;
constexpr int VisualStudio2013 = 12;
constexpr int VisualStudio2015 = 14;

constexpr int visualStudioTypeToVersion(const CxxVs10To14HeaderPathDetector::VisualStudioType type) {
  switch(type) {
  case CxxVs10To14HeaderPathDetector::VISUAL_STUDIO_2010:
    return VisualStudio2010;
  case CxxVs10To14HeaderPathDetector::VISUAL_STUDIO_2012:
    return VisualStudio2012;
  case CxxVs10To14HeaderPathDetector::VISUAL_STUDIO_2013:
    return VisualStudio2013;
  case CxxVs10To14HeaderPathDetector::VISUAL_STUDIO_2015:
    return VisualStudio2015;
  }
  return 0;
}

constexpr std::string visualStudioTypeToString(const CxxVs10To14HeaderPathDetector::VisualStudioType type) {
  std::string ret = "Visual Studio";
  switch(type) {
  case CxxVs10To14HeaderPathDetector::VISUAL_STUDIO_2010:
    return ret + " 2010";
  case CxxVs10To14HeaderPathDetector::VISUAL_STUDIO_2012:
    return ret + " 2012";
  case CxxVs10To14HeaderPathDetector::VISUAL_STUDIO_2013:
    return ret + " 2013";
  case CxxVs10To14HeaderPathDetector::VISUAL_STUDIO_2015:
    return ret + " 2015";
  }
  return ret;
}
}    // namespace

CxxVs10To14HeaderPathDetector::CxxVs10To14HeaderPathDetector(VisualStudioType type,
                                                             bool isExpress,
                                                             ApplicationArchitectureType architecture)
    : PathDetector(visualStudioTypeToString(type) + (isExpress ? " Express" : "") +
                   (architecture == ApplicationArchitectureType::X86_64 ? " 64 Bit" : ""))
    , m_version(visualStudioTypeToVersion(type))
    , m_isExpress(isExpress)
    , m_architecture(architecture) {}

std::vector<FilePath> CxxVs10To14HeaderPathDetector::doGetPaths() const {
  const FilePath vsInstallPath = getVsInstallPathUsingRegistry();

  // vc++ headers
  std::vector<FilePath> headerSearchPaths;
  if(vsInstallPath.exists()) {
    for(const auto subdirectory : {L"vc/include", L"vc/atlmfc/include"}) {
      FilePath headerSearchPath = vsInstallPath.getConcatenated(subdirectory);
      if(headerSearchPath.exists()) {
        headerSearchPaths.push_back(headerSearchPath.makeCanonical());
      }
    }
  }

  if(!headerSearchPaths.empty()) {
    utility::append(headerSearchPaths, utility::getWindowsSdkHeaderSearchPaths(m_architecture));
  }

  return headerSearchPaths;
}

FilePath CxxVs10To14HeaderPathDetector::getVsInstallPathUsingRegistry() const {
  QString key = "HKEY_LOCAL_MACHINE\\SOFTWARE\\";
  if(m_architecture == ApplicationArchitectureType::X86_32) {
    key += "Wow6432Node\\";
  }
  key += "Microsoft\\";
  key += (m_isExpress ? QStringLiteral("VCExpress") : QStringLiteral("VisualStudio"));
  key += "\\" + QString::number(m_version) + ".0";

  const QSettings expressKey(key, QSettings::NativeFormat);    // NativeFormat means from Registry on Windows.
  const QString value = expressKey.value("InstallDir").toString() + "../../";

  FilePath path(value.toStdWString());
  if(path.exists()) {
    LOG_INFO(L"Found working registry key for VS install path: " + key.toStdWString());
    return path;
  }

  return {};
}

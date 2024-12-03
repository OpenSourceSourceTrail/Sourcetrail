#include "utilityPathDetection.h"

#include "language_packages.h"
#include "logging.h"
#include "utilityApp.h"

#if BUILD_CXX_LANGUAGE_PACKAGE
#  include "CxxFrameworkPathDetector.h"
#  include "CxxHeaderPathDetector.h"
#  include "CxxVs10To14HeaderPathDetector.h"
#  include "CxxVs15HeaderPathDetector.h"
#endif    // BUILD_CXX_LANGUAGE_PACKAGE

std::shared_ptr<CombinedPathDetector> utility::getCxxVsHeaderPathDetector() {
  std::shared_ptr<CombinedPathDetector> combinedDetector = std::make_shared<CombinedPathDetector>();

  if(utility::getOsType() != OsType::Windows) {
    return combinedDetector;
  }

#if BUILD_CXX_LANGUAGE_PACKAGE
  combinedDetector->addDetector(std::make_shared<CxxVs15HeaderPathDetector>());

  combinedDetector->addDetector(std::make_shared<CxxVs10To14HeaderPathDetector>(
      CxxVs10To14HeaderPathDetector::VISUAL_STUDIO_2015, false, ApplicationArchitectureType::X86_32));
  combinedDetector->addDetector(std::make_shared<CxxVs10To14HeaderPathDetector>(
      CxxVs10To14HeaderPathDetector::VISUAL_STUDIO_2015, false, ApplicationArchitectureType::X86_64));
  combinedDetector->addDetector(std::make_shared<CxxVs10To14HeaderPathDetector>(
      CxxVs10To14HeaderPathDetector::VISUAL_STUDIO_2015, true, ApplicationArchitectureType::X86_32));
  combinedDetector->addDetector(std::make_shared<CxxVs10To14HeaderPathDetector>(
      CxxVs10To14HeaderPathDetector::VISUAL_STUDIO_2015, true, ApplicationArchitectureType::X86_64));
  combinedDetector->addDetector(std::make_shared<CxxVs10To14HeaderPathDetector>(
      CxxVs10To14HeaderPathDetector::VISUAL_STUDIO_2013, false, ApplicationArchitectureType::X86_32));
  combinedDetector->addDetector(std::make_shared<CxxVs10To14HeaderPathDetector>(
      CxxVs10To14HeaderPathDetector::VISUAL_STUDIO_2013, false, ApplicationArchitectureType::X86_64));
  combinedDetector->addDetector(std::make_shared<CxxVs10To14HeaderPathDetector>(
      CxxVs10To14HeaderPathDetector::VISUAL_STUDIO_2013, true, ApplicationArchitectureType::X86_32));
  combinedDetector->addDetector(std::make_shared<CxxVs10To14HeaderPathDetector>(
      CxxVs10To14HeaderPathDetector::VISUAL_STUDIO_2013, true, ApplicationArchitectureType::X86_64));
  combinedDetector->addDetector(std::make_shared<CxxVs10To14HeaderPathDetector>(
      CxxVs10To14HeaderPathDetector::VISUAL_STUDIO_2012, false, ApplicationArchitectureType::X86_32));
  combinedDetector->addDetector(std::make_shared<CxxVs10To14HeaderPathDetector>(
      CxxVs10To14HeaderPathDetector::VISUAL_STUDIO_2012, false, ApplicationArchitectureType::X86_64));
  combinedDetector->addDetector(std::make_shared<CxxVs10To14HeaderPathDetector>(
      CxxVs10To14HeaderPathDetector::VISUAL_STUDIO_2012, true, ApplicationArchitectureType::X86_32));
  combinedDetector->addDetector(std::make_shared<CxxVs10To14HeaderPathDetector>(
      CxxVs10To14HeaderPathDetector::VISUAL_STUDIO_2012, true, ApplicationArchitectureType::X86_64));
  combinedDetector->addDetector(std::make_shared<CxxVs10To14HeaderPathDetector>(
      CxxVs10To14HeaderPathDetector::VISUAL_STUDIO_2010, false, ApplicationArchitectureType::X86_32));
  combinedDetector->addDetector(std::make_shared<CxxVs10To14HeaderPathDetector>(
      CxxVs10To14HeaderPathDetector::VISUAL_STUDIO_2010, false, ApplicationArchitectureType::X86_64));
  combinedDetector->addDetector(std::make_shared<CxxVs10To14HeaderPathDetector>(
      CxxVs10To14HeaderPathDetector::VISUAL_STUDIO_2010, true, ApplicationArchitectureType::X86_32));
  combinedDetector->addDetector(std::make_shared<CxxVs10To14HeaderPathDetector>(
      CxxVs10To14HeaderPathDetector::VISUAL_STUDIO_2010, true, ApplicationArchitectureType::X86_64));
#endif    // BUILD_CXX_LANGUAGE_PACKAGE

  return combinedDetector;
}

std::shared_ptr<CombinedPathDetector> utility::getCxxHeaderPathDetector() {
  std::shared_ptr<CombinedPathDetector> combinedDetector = getCxxVsHeaderPathDetector();
#if BUILD_CXX_LANGUAGE_PACKAGE
  combinedDetector->addDetector(std::make_shared<CxxHeaderPathDetector>("clang"));
  combinedDetector->addDetector(std::make_shared<CxxHeaderPathDetector>("gcc"));
#endif    // BUILD_CXX_LANGUAGE_PACKAGE
  return combinedDetector;
}

std::shared_ptr<CombinedPathDetector> utility::getCxxFrameworkPathDetector() {
  std::shared_ptr<CombinedPathDetector> combinedDetector = std::make_shared<CombinedPathDetector>();
#if BUILD_CXX_LANGUAGE_PACKAGE
  combinedDetector->addDetector(std::make_shared<CxxFrameworkPathDetector>("clang"));
  combinedDetector->addDetector(std::make_shared<CxxFrameworkPathDetector>("gcc"));
#endif    // BUILD_CXX_LANGUAGE_PACKAGE
  return combinedDetector;
}

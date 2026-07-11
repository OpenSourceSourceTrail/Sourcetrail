#include "QtProjectWizardContentGroup.h"

#include <range/v3/algorithm/any_of.hpp>

#include <QGridLayout>

QtProjectWizardContentGroup::QtProjectWizardContentGroup(QtProjectWizardWindow* window) : QtProjectWizardContent(window) {}

void QtProjectWizardContentGroup::addContent(QtProjectWizardContent* content) {
  m_contents.push_back(content);
}

void QtProjectWizardContentGroup::addSpace() {
  m_contents.push_back(nullptr);
}

bool QtProjectWizardContentGroup::hasContents() const {
  return ranges::cpp20::any_of(m_contents, [](QtProjectWizardContent* content) { return content != nullptr; });
}

void QtProjectWizardContentGroup::populate(QGridLayout* layout, int& row) {
  layout->setRowMinimumHeight(row++, 10);

  for(QtProjectWizardContent* content : m_contents) {
    if(content != nullptr) {
      content->populate(layout, row);
    } else {
      layout->setRowMinimumHeight(row++, 15);
    }
  }

  layout->setRowMinimumHeight(row, 10);
  layout->setRowStretch(row, 1);
}

void QtProjectWizardContentGroup::load() {
  for(QtProjectWizardContent* content : m_contents) {
    if(content != nullptr) {
      content->load();
    }
  }
}

void QtProjectWizardContentGroup::save() {
  for(QtProjectWizardContent* content : m_contents) {
    if(content != nullptr) {
      content->save();
    }
  }
}

void QtProjectWizardContentGroup::refresh() {
  for(QtProjectWizardContent* content : m_contents) {
    if(content != nullptr) {
      content->refresh();
    }
  }
}

bool QtProjectWizardContentGroup::check() {
  for(QtProjectWizardContent* content : m_contents) {
    if((content != nullptr) && !content->check()) {
      return false;
    }
  }

  return true;
}

#pragma once
#include <QObject>

class QtContextMenuTestSuite : public QObject {
  Q_OBJECT
public:
private slots:
  void testingStructure();

  void testingEnableUndo();

  void testingEnableRedo();
};
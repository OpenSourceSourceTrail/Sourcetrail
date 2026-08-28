#pragma once
#include <QWidget>

#include "component/controller/helper/ControllerProxy.h"
#include "component/view/ErrorView.h"
#include "qt/utility/QtThreadedFunctor.h"

class QBoxLayout;
class QCheckBox;
class QLabel;
class QPushButton;
class QStandardItemModel;
class QtHelpButton;
class QtSelfRefreshIconButton;
class QtTable;
class ErrorController;
struct ErrorFilter;

class QtErrorView final
    : public QWidget
    , public ErrorView {
  Q_OBJECT

public:
  explicit QtErrorView(ViewLayout* viewLayout);

  ~QtErrorView() override;

  // View implementation
  void createWidgetWrapper() override;
  void refreshView() override;

  // ErrorView implementation
  void clear() override;

  void addErrors(const std::vector<ErrorInfo>& errors, const ErrorCountInfo& errorCount, bool scrollTo) override;
  void setErrorId(Id errorId) override;

  [[nodiscard]] ErrorFilter getErrorFilter() const override;
  void setErrorFilter(const ErrorFilter& filter) override;

  void showErrorHelpMessage() override;

private slots:
  void errorFilterChanged(int i = 0);

private:
  enum class Column : uint8_t {
    ID = 0,
    Type = 1,
    Message = 2,
    File = 3,
    Line = 4,
    Indexed = 5,
    TranslationUnit = 6,
    ColumnMax = TranslationUnit
  };

  void setStyleSheet() const;

  void addErrorToTable(const ErrorInfo& error);

  QCheckBox* createFilterCheckbox(const QString& name, bool checked, QBoxLayout* layout);
  bool isShownError(const ErrorInfo& error);

  static QIcon s_errorIcon;

  ControllerProxy<ErrorController> m_controllerProxy;
  QtThreadedLambdaFunctor m_onQtThread;

  ErrorFilter m_errorFilter;

  QLabel* m_errorLabel = nullptr;
  QLabel* m_allLabel = nullptr;
  QPushButton* m_allButton = nullptr;

  QCheckBox* m_showErrors = nullptr;
  QCheckBox* m_showFatals = nullptr;
  QCheckBox* m_showNonIndexedErrors = nullptr;
  QCheckBox* m_showNonIndexedFatals = nullptr;

  QtHelpButton* m_helpButton = nullptr;
  QtSelfRefreshIconButton* m_editButton = nullptr;

  QStandardItemModel* m_model = nullptr;
  QtTable* m_table = nullptr;
};
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <vector>
#include <set>
#include <string>
#include <memory>
#include <functional>
#include <variant>

#include "../BaseLibraries/Expression/function_pool.h"

#include <QtGui>
#include <QtCore>



#include <QGLWidget>
#include <QDialog>

class CViewWidget;
class CScene;

class CWidgetsContainer
{
  public:
  using widget_ptr_t=std::variant<QLabel*,QLineEdit*,
                                  QComboBox*,QPushButton*>;
  using layout_t=std::vector<std::vector<widget_ptr_t>>;
  private:
  std::vector<std::vector<widget_ptr_t>> m_widgets;
  void m_Init();
  bool m_CheckAccess(int,int)const;
  protected:
  QGridLayout* m_layout;
  CWidgetsContainer(const std::vector<std::vector<widget_ptr_t>>&);
  CWidgetsContainer(std::vector<std::vector<widget_ptr_t>>&&);
  public:
  QWidget& Widget(int i,int j);
  const QWidget& Widget(int i,int j)const;
  template<class T>
  T& Widget(int i,int j)
  {
    assert(m_CheckAccess(i,j));
    return *std::get<T*>(m_widgets[i][j]);
  }
  template<class T>
  const T& Widget(int i,int j)const
  {
    assert(m_CheckAccess(i,j));
    return *std::get<T*>(m_widgets[i][j]);
  }
  std::string Text(int i,int j)const;
  QString Qtext(int i,int j)const;
  void SetText(int i,int j,const std::string&);
  void SetText(int i,int j,const QString&);
  int Rows()const{return m_widgets.size();}
  int Cols(int i)const{return m_widgets[i].size();}
};

class CBaseDialog:public QDialog,public CWidgetsContainer
{
  Q_OBJECT
  public:
  using preprocessor_t=std::function<std::string(const CBaseDialog&)>;
  private:
  preprocessor_t m_ok_ftr;
  void m_Init();
  layout_t m_Layout(layout_t);
  private slots:
  void m_ok_slot();
  public :
  CBaseDialog(const std::vector<std::vector<widget_ptr_t>>&,
              QWidget*p=nullptr);
  CBaseDialog(std::vector<std::vector<widget_ptr_t>>&&,
              QWidget*p=nullptr);
  void SetPreprocessor(preprocessor_t p){m_ok_ftr=p;}
};

class CFlagButton:public QPushButton
{
  Q_OBJECT
  const int m_iden;
  private slots:void m_toggled_slot(bool);
  signals:
  void m_toggled_sig(int,bool);
  public:
  explicit CFlagButton(int iden,bool,QWidget*w=nullptr);
};

class CMainWindow:public QMainWindow
{
  Q_OBJECT
  struct previous_dg_t
  {
    std::vector<std::string> m_edit;
    void (CMainWindow::*m_call)()=nullptr;
    void SetEmpty(int i)
    {
      m_edit.clear();
      m_edit.resize(i);
      m_call=nullptr;
    }
  } m_previous_dg;
  private slots:
  //Save/Load
  void m_LoadFromJson();
  void m_SaveToJson();
  // 3D plots
  void m_3D_cartesian_dialog();
  void m_3D_spherical_dialog();
  void m_3D_polar_dialog();
  void m_3D_parametric_dialog();
  void m_3D_cartesian_edit();
  void m_3D_spherical_edit();
  void m_3D_polar_edit();
  void m_3D_parametric_edit();
  // toolbars
  void m_3D_toolbar_slot(int,bool);
  void m_Clear();
  //2d plots
  void m_2D_cartesian_dialog();
  void m_2D_polar_dialog();
  void m_2D_parametric_dialog();
  //Constants
  void m_ConstantsDialog();
  void m_BuiltInConstants();
  //Functions
  void m_FunctionsDialog();
  void m_BuiltInFunctions();
  //Settings
  void m_2D_settings();
  void m_3D_settings();
  // Context menu
  void m_ContextMenuDialog();
  // Calculate dialog
  void m_CalculateDialog();
  private:
  std::unique_ptr<QToolBar> m_2D_toolbar;
  std::unique_ptr<QToolBar> m_3D_toolbar;
  QLabel*m_statusbar;
  QMenu* m_context_menu;
  QMenu* m_CreateFileMenu();
  QMenu* m_CreateDrawMenu();
  QMenu* m_CreateConstantsMenu();
  QMenu* m_CreateFunctionsMenu();
  QMenu* m_CreateSettingsMenu();
  QMenu* m_CreateContextMenu();

  void m_SwitchTo2D();
  void m_SwitchTo3D();

  virtual void contextMenuEvent(QContextMenuEvent*)override;
  virtual void mouseDoubleClickEvent(QMouseEvent*)override;
  public:
  inline static const std::set<std::string> Keywords=
  {
    "x","y","s","t","r","phi","time"
  };
  CMainWindow();
  void CreateMenu();
  void CreateToolBars();
  void CreateViewWidget();
  CViewWidget*ViewWidget();
  CScene&Scene();
  ~CMainWindow();
};


class CInfoTableDialog:public QDialog
{
  struct model_t:public QAbstractListModel
  {
    std::function<std::string(int,int)> m_fn_data;
    int m_row,m_col;
    int rowCount(const QModelIndex &parent = QModelIndex())const override
    {return m_row;}
    int columnCount(const QModelIndex &parent = QModelIndex())const override
    {return m_col;}
    QVariant data(const QModelIndex &index, int role) const override;
  };
  model_t m_model;
  QTableView*    m_view;
  public:
  enum button_t{ok_id=1,cancel_id=2};
  explicit CInfoTableDialog(int flags,
                            std::pair<int,int>,
                            std::function<std::string(int,int)>);
};

// Custom constants

class CConstantTable:public QAbstractTableModel
{
  Q_OBJECT
  using CFunction=CFunctionPool::CFunction;
  using CConstant=CFunctionPool::CConstant;
  CFunctionPool&     m_pool;
  QTableView*        m_view=nullptr;
  public slots:
  void AddButtonClicked();
  void RemoveButtonClicked();
  void EditButtonClicked();
  public:
  explicit CConstantTable(CFunctionPool&);

  int rowCount(const QModelIndex &parent = QModelIndex())const override;
  int columnCount(const QModelIndex &parent = QModelIndex())const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
  void Repaint();
  void ResetRecord(const QModelIndex&, float);
  void RemoveRecord(const QModelIndex&);
  bool Insert(const std::string&str,double val);
  void SetView(QTableView*view);

};

class CEditConstantsWidget:public QDialog
{
  QTableView *m_view;
  CConstantTable m_table;
  public:
  explicit CEditConstantsWidget(CFunctionPool&);
};

// Custom functions

class CFunctionsTable:public QAbstractTableModel
{
  Q_OBJECT
  using CFunction=CFunctionPool::CFunction;
  using CConstant=CFunctionPool::CConstant;
  using parse_error_t=expr::parse_error_t;

  QTableView*    m_view=nullptr;
  CFunctionPool&m_pool;
  public:
  CFunctionsTable(CFunctionPool&pool);
  void SetView(QTableView*view);
  std::string MakeArgsString(int i)const;
  int rowCount(const QModelIndex &parent = QModelIndex())const override;
  int columnCount(const QModelIndex &parent = QModelIndex())const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
  void Repaint();
  expr::parse_error_t ResetFunction(int i,
                              const std::vector<std::string>&args,
                              const std::string&body);
  void RemoveFunction(const QModelIndex&);

  expr::parse_error_t AddFunction(const std::string&name,
                    const std::vector<std::string>&args,
                    const std::string&body);
  public slots:
  void AddButtonClicked();
  void EditButtonClicked();
  void RemoveButtonClicked();
};

class CEditFunctionsWidget:public QDialog
{
  QTableView *m_view;
  CFunctionsTable m_table;
  public:
  explicit CEditFunctionsWidget(CFunctionPool&);
};

class CCalculateDialog:public QDialog,public CWidgetsContainer
{
  Q_OBJECT
  layout_t m_MakeWidgets();
  private slots:void Calculate();
  public:
  CCalculateDialog();
};

#endif // MAINWINDOW_H

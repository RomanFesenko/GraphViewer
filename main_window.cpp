#include <iostream>
#include <math.h>
#include <optional>
#include <ranges>
#include <regex>
#include <type_traits>
#include <fstream>

#include <QFileDialog>

#include "view_widget.h"
#include "main_window.h"
#include "json_convert.h"



namespace rgs=std::ranges;

const std::regex reg_identifier=
std::regex("[_[:alpha:]][_[:alnum:]]*");

static QTime glTime;
static CDataPool glDataPool;
static CPlot2D glPlot2D;
static CRenderer glPlot3D;


static float Now()
{
  return glTime.elapsed()/1000.0f;
}

std::vector<int>
ParseInts(const CBaseDialog&dg,const std::vector<int>&fields,
          std::string &str)
{
  const std::regex reg=std::regex("-?[[:digit:]]+");
  std::vector<int> ints(*rgs::max_element(fields)+1);
  for(int i:fields)
  {
      auto text=dg.Text(i,1);
      if(!std::regex_match(text.begin(),text.end(),reg))
      {
          str=dg.Text(i,0)+" must be integer";
          return {};
      }
      ints[i]=std::stoi(text);
  }
  str.clear();
  return ints;
}

std::vector<float>
ParseFloats(const CBaseDialog&dg,
            const std::vector<int>&fields,
            std::string &str,const CDataPool&pool)
{
  //const std::regex reg=std::regex("-?[[:digit:]]+(\\.[[:digit:]]*)?");
  std::vector<float> floats(*rgs::max_element(fields)+1);
  for(int i:fields)
  {
      auto text=dg.Text(i,1);
      auto expr=pool.CreateFunction({},text);
      if(!expr)
      {
          str="For "+dg.Text(i,0)+" is not valid expression:"+expr.error().detail();
          return {};
      }
      floats[i]=expr();
  }
  str.clear();
  return floats;
}

std::string ParseArgs(const std::string&str,
                      std::vector<std::string>&args)
{
  args.clear();
  for(auto curr=str.begin();curr<str.end();)// valid,because string - ContiguousContainer
  {
     auto next=std::find(curr,str.end(),',');
     if(!std::regex_match(curr,next,reg_identifier))
     {
         return "'"+std::string(curr,next)+"' "+"is not correct identifier";
     }
     args.push_back(std::string(curr,next));
     curr=next+1;
  }
  return {};
}

CFlagButton::CFlagButton(int iden, bool check, QWidget*parent):
QPushButton(parent),
m_iden(iden)
{
  setCheckable(true);
  setChecked(check);
  connect(this,SIGNAL(toggled(bool)),
          this,SLOT(m_toggled_slot(bool)));
}

void CFlagButton::m_toggled_slot(bool check)
{
  emit m_toggled_sig(m_iden,check);
}

auto LinesEditDg(const std::vector<QString>&strs)
{
  assert(!strs.empty());
  CBaseDialog::layout_t wts;
  for(const auto&str:strs)
  {
      wts.push_back({new QLabel(str),new QLineEdit()});
  }
  return std::make_unique<CBaseDialog>(std::move(wts));
}


/////////////////////////////////////////////////
// CBaseDialog- container for an arbitrary set of
// input widgets,with 'Ok','Cancel' buttons and
// custom input processing functions
/////////////////////////////////////////////////

void CBaseDialog::m_Init()
{
  assert(m_widgets.size()>0);
  setFont(QFont("Times",13,QFont::Normal));
  QPushButton*pcmdOk=new QPushButton("&Ok");
  QPushButton*pcmdCancel = new QPushButton("&Cancel");

  connect(pcmdOk, SIGNAL(clicked()), SLOT(m_ok_slot()));
  connect(pcmdCancel, SIGNAL(clicked()), SLOT(reject()));

  QGridLayout* ptopLayout = new QGridLayout;
  for(std::size_t i=0;i<m_widgets.size();++i)
  {
      auto cols=m_widgets[0].size();
      assert(cols>0);
      for(std::size_t j=0;j<cols;++j)
      {
          ptopLayout->addWidget(&Widget(i,j),i,j);
      }
  }
  ptopLayout->addWidget(pcmdOk,m_widgets.size(),0);
  ptopLayout->addWidget(pcmdCancel,m_widgets.size(), 1);
  setLayout(ptopLayout);
}

CBaseDialog::CBaseDialog(const layout_t&wts,QWidget*p):
QDialog(p),
m_widgets(wts)
{
  m_Init();
}

CBaseDialog::CBaseDialog(layout_t&&wts,QWidget*p):
QDialog(p),
m_widgets(std::move(wts))
{
  m_Init();
}

bool CBaseDialog::m_CheckAccess(int i,int j)const
{
  return i>=0&&i<m_widgets.size()&&j>=0&&j<m_widgets[i].size();
}

void CBaseDialog::m_ok_slot()
{
   if(!m_ok_ftr)
   {
       accept();
       return;
   }
   auto str= m_ok_ftr(*this);
   if(!str.empty())
   {
       QMessageBox::information(nullptr,
                                "Input error",
                                QString::fromStdString(str));
   }
   else
   {
       accept();
   }
}


QWidget& CBaseDialog::Widget(int i,int j)
{
  assert(m_CheckAccess(i,j));
  auto vis=[](auto wt)->QWidget*
  {
      return static_cast<QWidget*>(wt);
  };
  return *std::visit(vis,m_widgets[i][j]);
}

const QWidget& CBaseDialog::Widget(int i,int j) const
{
  assert(m_CheckAccess(i,j));
  auto vis=[](auto wt)->const QWidget*
  {
      return static_cast<const QWidget*>(wt);
  };
  return *std::visit(vis,m_widgets[i][j]);
}

void CBaseDialog::SetText(int i,int j,const std::string&str)
{
  assert(m_CheckAccess(i,j));
  auto vis=[&str](auto wt)
  {
      using T = decltype(wt);
      if constexpr (std::is_same_v<T,QComboBox*>) assert(false);
      else  {wt->setText(QString::fromStdString(str));}
  };
  std::visit(vis,m_widgets[i][j]);
}

void CBaseDialog::SetText(int i,int j,const QString&str)
{
  assert(m_CheckAccess(i,j));
  auto vis=[&str](auto wt)
  {
      using T =decltype(wt);
      if constexpr (std::is_same_v<T,QComboBox*>) assert(false);
      else  wt->setText(str);
  };
  std::visit(vis,m_widgets[i][j]);
}

std::string CBaseDialog::Text(int i,int j)const
{
  assert(m_CheckAccess(i,j));
  auto vis=[](auto wt)->std::string
  {
      using T = std::decay_t<decltype(wt)>;
      if constexpr (std::is_same_v<T,QComboBox*>)
      {
        assert(false);return "";
      }
      else{return wt->text().toStdString();}
  };
  return std::visit(vis,m_widgets[i][j]);
}

QString CBaseDialog::Qtext(int i,int j)const
{
  assert(m_CheckAccess(i,j));
  auto vis=[](auto wt)->QString
  {
      using T = std::decay_t<decltype(wt)>;
      if constexpr (std::is_same_v<T,QComboBox*>)
      {
        assert(false);return "";
      }
      else{return wt->text();}
  };
  return std::visit(vis,m_widgets[i][j]);
}

////////////////////////////////////////
//      CInfoTableDialog              //
////////////////////////////////////////

QVariant
CInfoTableDialog::model_t::data(const QModelIndex &index,int role)const
{
  if(!m_fn_data) return {};
  int i=index.row();
  int j=index.column();
  if(i<m_row&&j<m_col&&role==Qt::DisplayRole)
  {
     return  QString::fromStdString(m_fn_data(i,j));
  }
  return {};
}

CInfoTableDialog::
CInfoTableDialog(int flags,
                 std::pair<int,int> size,
                 std::function<std::string(int,int)> data)
{
  assert((flags|ok_id)||(flags|cancel_id));
  m_model.m_row=size.first;
  m_model.m_col=size.second;
  m_model.m_fn_data=data;
  m_view=new QTableView;m_view->setModel(&m_model);

  QBoxLayout*thisLayout=new QBoxLayout(QBoxLayout::TopToBottom);
  thisLayout->addWidget(m_view);
  QBoxLayout*ltButtons=new QBoxLayout(QBoxLayout::LeftToRight);

  if(flags&ok_id)
  {
      QPushButton*ok=new QPushButton("Ok");
      QObject::connect(ok,SIGNAL(clicked()),
                       this,SLOT(accept()));
      ltButtons->addWidget(ok);
  }
  if(flags&cancel_id)
  {
      QPushButton*cancel=new QPushButton("Cancel");
      QObject::connect(cancel,SIGNAL(clicked()),
                       this,SLOT(reject()));
      ltButtons->addWidget(cancel);
  }
  thisLayout->addLayout(ltButtons);
  setLayout(thisLayout);
}

///////////////////////////////////////////
//          CMainWindow                  //
///////////////////////////////////////////

CMainWindow::CMainWindow()
{
  setFont(QFont("Times",13,QFont::Normal));
  CreateViewWidget();
  CreateMenu();
  CreateToolBars();
  setMinimumHeight(400);
  setMinimumWidth(400);
  std::cout<<"CMainWindow create\n";
}

QMenu* CMainWindow::m_CreateFileMenu()
{
  QMenu*pmnuFile =new QMenu("File");
  QAction* act = new QAction("Load from JSON",nullptr);
  connect(act,SIGNAL(triggered()),SLOT(m_LoadFromJson()));
  pmnuFile->addAction(act);
  act = new QAction("Save to JSON",nullptr);
  connect(act,SIGNAL(triggered()),SLOT(m_SaveToJson()));
  pmnuFile->addAction(act);
  return pmnuFile;
}

QMenu* CMainWindow::m_CreateDrawMenu()
{
  QMenu*pmnuDraw =new QMenu("Draw");
  QMenu*pmnuPlot2D =new QMenu("2D Plot");
  QMenu*pmnuPlot3D =new QMenu("3D Plot");

  //2d plots
  QAction* act = new QAction("Cartesian",nullptr);
  connect(act,SIGNAL(triggered()),SLOT(m_2D_cartesian_dialog()));
  pmnuPlot2D->addAction(act);

  act = new QAction("Polar",nullptr);
  connect(act,SIGNAL(triggered()),SLOT(m_2D_polar_dialog()));
  pmnuPlot2D->addAction(act);

  act = new QAction("Parametric",nullptr);
  connect(act,SIGNAL(triggered()),SLOT(m_2D_parametric_dialog()));
  pmnuPlot2D->addAction(act);

  //3d plots
  act=new QAction("Cartesian",nullptr);
  connect(act,SIGNAL(triggered()),SLOT(m_3D_cartesian_dialog()));
  pmnuPlot3D->addAction(act);

  act=new QAction("Sperical",nullptr);
  connect(act,SIGNAL(triggered()),SLOT(m_3D_spherical_dialog()));
  pmnuPlot3D->addAction(act);

  act=new QAction("Polar",nullptr);
  connect(act,SIGNAL(triggered()),SLOT(m_3D_polar_dialog()));
  pmnuPlot3D->addAction(act);

  act=new QAction("Parametric",nullptr);
  connect(act,SIGNAL(triggered()),SLOT(m_3D_parametric_dialog()));
  pmnuPlot3D->addAction(act);

  pmnuDraw->addMenu(pmnuPlot2D);
  pmnuDraw->addMenu(pmnuPlot3D);
  return pmnuDraw;
}

QMenu* CMainWindow::m_CreateConstantsMenu()
{
  QMenu*pmnuConsts =new QMenu("Constants");

  QAction* act = new QAction("Custom constants",nullptr);
  connect(act,SIGNAL(triggered()),
          this,SLOT(m_ConstantsDialog()));
  pmnuConsts->addAction(act);

  act = new QAction("Builtin constants",nullptr);
  connect(act,SIGNAL(triggered()),
          this,SLOT(m_BuiltInConstants()));
  pmnuConsts->addAction(act);

  return pmnuConsts;
}

QMenu* CMainWindow::m_CreateFunctionsMenu()
{
  QMenu*pmnuFns =new QMenu("Functions");

  QAction* act = new QAction("Custom functions",nullptr);
  connect(act,SIGNAL(triggered()),
          this,SLOT(m_FunctionsDialog()));
  pmnuFns->addAction(act);

  act = new QAction("Builtin functions",nullptr);
  connect(act,SIGNAL(triggered()),
          this,SLOT(m_BuiltInFunctions()));
  pmnuFns->addAction(act);

  return pmnuFns;
}

QMenu* CMainWindow::m_CreateSettingsMenu()
{
  QMenu*pmnuSett =new QMenu("Settings");

  QAction* act = new QAction("2D plots",nullptr);
  connect(act,SIGNAL(triggered()),
          this,SLOT(m_2D_settings()));
  pmnuSett->addAction(act);

  act = new QAction("3D plots",nullptr);
  connect(act,SIGNAL(triggered()),
          this,SLOT(m_3D_settings()));
  pmnuSett->addAction(act);

  return pmnuSett;
}

void CMainWindow::contextMenuEvent(QContextMenuEvent* pe)
{
  CViewWidget*view=static_cast<CViewWidget*>(centralWidget());
  if(view->ViewType()==CViewWidget::ogl_view_id) return;
  m_context_menu->exec(pe->globalPos());
}

QMenu* CMainWindow::m_CreateContextMenu()
{
  QMenu*pmnu =new QMenu();

  QAction* act = new QAction("Resize plot",nullptr);
  connect(act,SIGNAL(triggered()),
          this,SLOT(m_ContextMenuDialog()));
  pmnu->addAction(act);

  return pmnu;
}

void CMainWindow::CreateMenu()
{
   //Add actions to menu...
   menuBar()->addMenu(m_CreateFileMenu());
   menuBar()->addMenu(m_CreateDrawMenu());
   menuBar()->addMenu(m_CreateFunctionsMenu());
   menuBar()->addMenu(m_CreateConstantsMenu());
   menuBar()->addMenu(m_CreateSettingsMenu());
   m_context_menu=m_CreateContextMenu();
}

void CMainWindow::CreateViewWidget()
{
  m_statusbar=new QLabel();
  m_statusbar->setMaximumHeight(15);
  statusBar()->addWidget(m_statusbar);
  CViewWidget* view=new CViewWidget(glPlot2D,glPlot3D,m_statusbar);
  setCentralWidget(view);
}

void CMainWindow::m_SwitchTo2D()
{
  CViewWidget*view=static_cast<CViewWidget*>(centralWidget());
  if(view->ViewType()==CViewWidget::qt_view_id) return;
  auto all=findChildren<QToolBar*>();
  assert(all.size()==1&&all[0]==m_3D_toolbar.get());
  removeToolBar(all[0]);
  all[0]->setParent(nullptr);
  assert(findChildren<QToolBar*>().empty());
  addToolBar(Qt::LeftToolBarArea,m_2D_toolbar.get());
  m_2D_toolbar->show();
  view->SwitchTo2D();
}

void CMainWindow::m_SwitchTo3D()
{
  CViewWidget*view=static_cast<CViewWidget*>(centralWidget());
  if(view->ViewType()==CViewWidget::ogl_view_id) return;
  auto all=findChildren<QToolBar*>();
  assert(all.size()==1&&all[0]==m_2D_toolbar.get());
  removeToolBar(all[0]);
  all[0]->setParent(nullptr);
  assert(findChildren<QToolBar*>().empty());
  addToolBar(Qt::LeftToolBarArea,m_3D_toolbar.get());
  m_3D_toolbar->show();
  view->SwitchTo3D();
}

void CMainWindow::m_Clear()
{
  CViewWidget*view=static_cast<CViewWidget*>(centralWidget());
  view->Clear();
}

CMainWindow::~CMainWindow()
{
  m_2D_toolbar=nullptr;// not necessary
  m_3D_toolbar=nullptr;// not necessary
  assert(findChildren<QToolBar*>().empty());
}

// Save/Load

void CMainWindow::m_LoadFromJson()
{
  QString str=QFileDialog::getOpenFileName(0,"Load from JSON","","*.json");
  if(str.isEmpty()) return;
  std::fstream file(str.toStdString());
  if(!file)
  {
      QMessageBox::information(nullptr,"File error","Can't open "+str);
      return;
  }
  m_Clear();
  char c;
  std::string content;
  while(file.get(c)) content.push_back(c);
  auto err=FromJson(content,glDataPool);
  if(!err.empty())
  {
      QMessageBox::information(nullptr,
                               "JSON error",
                               "JSON:"+QString::fromStdString(err));
  }
}

void CMainWindow::m_SaveToJson()
{
  QString str=QFileDialog::getSaveFileName(0,"Save to JSON","","*.json");
  if(str.isEmpty()) return;
  if(!str.endsWith(".json")) str+=".json";
  std::ofstream file(str.toStdString());
  if(!file)
  {
      QMessageBox::information(nullptr,"File error","Can't create "+str);
      return;
  }
  std::string  json_str;
  ToJson(json_str,glDataPool);
  file<<json_str;
}

//3D plots
void CMainWindow::m_3D_cartesian_dialog()
{
  m_previous_dg.SetEmpty(5);
  m_3D_cartesian_edit();
}

void CMainWindow::m_3D_cartesian_edit()
{
  std::vector<QString> v_str
  ({"z(x,y,time):","min x:","max x:","min y:","max y:"});
  auto dg=LinesEditDg(v_str);
  assert(m_previous_dg.m_edit.size()==v_str.size());
  for(int i=0;i<v_str.size();++i)dg->SetText(i,1,m_previous_dg.m_edit[i]);
  auto preprocess=[this,&v_str](const CBaseDialog&dg)
  ->std::string
  {
      std::string mess;
      auto floats=ParseFloats(dg,{1,2,3,4},mess,glDataPool);
      if(!mess.empty()) return mess;
      if(floats[1]>=floats[2])return "min x must be less max x";
      if(floats[3]>=floats[4])return "min y must be less max y";
      auto f=glDataPool.CreateFunction({"x","y","time"},dg.Text(0,1));
      if(!f) return  "Error:"+f.error().detail();
      auto graph_1=CPlot3D::make_cartesian_dyn(f,{floats[1],floats[2]},
                                               {floats[3],floats[4]});
      graph_1.m_is_dynamic=f.is_variable_used(2);
      glPlot3D.SetGraph(graph_1,Now());
      for(int i=0;i<v_str.size();++i) m_previous_dg.m_edit[i]=dg.Text(i,1);
      m_previous_dg.m_call=&CMainWindow::m_3D_cartesian_edit;
      return {};
  };
  dg->SetPreprocessor(preprocess);
  if(dg->exec()==QDialog::Accepted) m_SwitchTo3D();
}

void CMainWindow::m_3D_spherical_dialog()
{
  m_previous_dg.SetEmpty(1);
  m_3D_spherical_edit();
}

void CMainWindow::m_3D_spherical_edit()
{
  std::vector<QString> v_str({"ro(t,phi,time):"});
  auto dg=LinesEditDg(v_str);
  assert(m_previous_dg.m_edit.size()==v_str.size());
  dg->SetText(0,1,m_previous_dg.m_edit[0]);
  auto preprocess=[this](const CBaseDialog&dg)
  ->std::string
  {
      auto f=glDataPool.CreateFunction({"t","phi","time"},dg.Text(0,1));
      if(!f) return  "Error:"+f.error().detail();
      auto graph_1=CPlot3D::make_spherical_dyn(f);
      graph_1.m_is_dynamic=f.is_variable_used(2);
      glPlot3D.SetGraph(graph_1,Now());
      m_previous_dg.m_edit[0]=dg.Text(0,1);
      m_previous_dg.m_call=&CMainWindow::m_3D_spherical_edit;
      return {};
  };
  dg->SetPreprocessor(preprocess);
  if(dg->exec()==QDialog::Accepted) m_SwitchTo3D();
}

void CMainWindow::m_3D_polar_dialog()
{
  m_previous_dg.SetEmpty(2);
  m_3D_polar_edit();
}

void CMainWindow::m_3D_polar_edit()
{
  std::vector<QString> v_str({"z(r,phi,time):","max r:"});
  auto dg=LinesEditDg(v_str);
  assert(m_previous_dg.m_edit.size()==v_str.size());
  for(int i=0;i<v_str.size();++i)dg->SetText(i,1,m_previous_dg.m_edit[i]);
  auto preprocess=[this,&v_str](const CBaseDialog&dg)
  ->std::string
  {
      std::string str;
      auto floats=ParseFloats(dg,{1},str,glDataPool);
      if(!str.empty()) return str;
      if(floats[1]<=0)return "max r must be greater 0";
      auto f=glDataPool.CreateFunction({"r","phi","time"},dg.Text(0,1));
      if(!f) return  "Error:"+f.error().detail();
      auto graph_1=CPlot3D::make_polar_dyn(f,floats[1]);
      graph_1.m_is_dynamic=f.is_variable_used(2);
      glPlot3D.SetGraph(graph_1,Now());
      for(int i=0;i<v_str.size();++i) m_previous_dg.m_edit[i]=dg.Text(i,1);
      m_previous_dg.m_call=&CMainWindow::m_3D_polar_edit;
      return {};
  };
  dg->SetPreprocessor(preprocess);
  if(dg->exec()==QDialog::Accepted) m_SwitchTo3D();
}

void CMainWindow::m_3D_parametric_dialog()
{
  m_previous_dg.SetEmpty(7);
  m_3D_parametric_edit();
}

void CMainWindow::m_3D_parametric_edit()
{
  std::vector<QString> v_str
  ({"x(s,t,time):",
    "y(s,t,time):",
    "z(s,t,time):",
    "min s:","max s:","min t:","max t:"});
  auto dg=LinesEditDg(v_str);
  assert(m_previous_dg.m_edit.size()==v_str.size());
  for(int i=0;i<v_str.size();++i)dg->SetText(i,1,m_previous_dg.m_edit[i]);
  auto preprocess=[this,&v_str](const CBaseDialog&dg)
  ->std::string
  {
      std::string str;
      auto floats=ParseFloats(dg,{3,4,5,6},str,glDataPool);
      if(!str.empty()) return str;
      if(floats[3]>=floats[4])return "min s must be less max s";
      if(floats[5]>=floats[6])return "min t must be less max t";
      auto _x=glDataPool.CreateFunction({"s","t","time"},dg.Text(0,1));
      if(!_x) return  "Error in x(...):"+_x.error().detail();
      auto _y=glDataPool.CreateFunction({"s","t","time"},dg.Text(1,1));
      if(!_y) return  "Error in y(...):"+_y.error().detail();
      auto _z=glDataPool.CreateFunction({"s","t","time"},dg.Text(2,1));
      if(!_z) return  "Error in z(...):"+_z.error().detail();
      auto graph_1=CPlot3D::make_parametric_dyn(_x,_y,_z,
                                                {floats[3],floats[4]},
                                                {floats[5],floats[6]});
      graph_1.m_is_dynamic=_x.is_variable_used(2)||
                           _y.is_variable_used(2)||
                           _z.is_variable_used(2);
      glPlot3D.SetGraph(graph_1,Now());
      for(int i=0;i<v_str.size();++i) m_previous_dg.m_edit[i]=dg.Text(i,1);
      m_previous_dg.m_call=&CMainWindow::m_3D_parametric_edit;
      return {};
  };
  dg->SetPreprocessor(preprocess);
  if(dg->exec()==QDialog::Accepted) m_SwitchTo3D();
}

//2D plots

void CMainWindow::m_2D_cartesian_dialog()
{
  std::vector<QString> v_str({"y(x,time):","min x:","max x:"});
  auto dg=LinesEditDg(v_str);
  auto preprocess=[](const CBaseDialog&dg)
  ->std::string
  {
      std::string str;
      auto floats=ParseFloats(dg,{1,2},str,glDataPool);
      if(!str.empty()) return str;
      if(floats[1]>=floats[2])return "min x must be less max x";
      auto f=glDataPool.CreateFunction({"x","time"},dg.Text(0,1));
      if(!f) return "Error:"+f.error().detail();
      auto graph_1=CPlot2D::make_cartesian_dyn(f,{floats[1],floats[2]});
      graph_1.m_is_dynamic=f.is_variable_used(1);
      glPlot2D.AddPlot(graph_1);
      return {};
  };
  dg->SetPreprocessor(preprocess);
  if(dg->exec()==QDialog::Accepted) m_SwitchTo2D();

}
void CMainWindow::m_2D_polar_dialog()
{
  std::vector<QString> v_str({"r(phi,time):"});
  auto dg=LinesEditDg(v_str);
  auto preprocess=[](const CBaseDialog&dg)
  ->std::string
  {
      auto f=glDataPool.CreateFunction({"phi","time"},dg.Text(0,1));
      if(!f) return  "Error:"+f.error().detail();
      auto graph_1=CPlot2D::make_polar_dyn(f);
      graph_1.m_is_dynamic=f.is_variable_used(1);
      glPlot2D.AddPlot(graph_1);
      return {};
  };
  dg->SetPreprocessor(preprocess);
  if(dg->exec()==QDialog::Accepted) m_SwitchTo2D();
}

void CMainWindow::m_2D_parametric_dialog()
{
  std::vector<QString> v_str({"x(s,time):",
                              "y(s,time):",
                              "s min:","s max:"});
  auto dg=LinesEditDg(v_str);
  auto preprocess=[](const CBaseDialog&dg)
  ->std::string
  {
      std::string str;
      auto floats=ParseFloats(dg,{2,3},str,glDataPool);
      if(!str.empty()) return str;
      if(floats[2]>=floats[3])return "min s must be less max s";
      auto _x=glDataPool.CreateFunction({"s","time"},dg.Text(0,1));
      if(!_x) return  "Error in x(...):"+_x.error().detail();
      auto _y=glDataPool.CreateFunction({"s","time"},dg.Text(1,1));
      if(!_y) return  "Error in y(...):"+_y.error().detail();
      auto graph_1=CPlot2D::make_parametric_dyn(_x,_y,{floats[2],floats[3]});
      graph_1.m_is_dynamic=_x.is_variable_used(1)||
                           _y.is_variable_used(1);
      glPlot2D.AddPlot(graph_1);
      return {};
  };
  dg->SetPreprocessor(preprocess);
  if(dg->exec()==QDialog::Accepted) m_SwitchTo2D();
}

//Toolbar of main window

void CMainWindow::CreateToolBars()
{
  using rt=rendering_traits_t;
  m_3D_toolbar= std::make_unique<QToolBar>("3D toolbar");
  int flags[7]={rt::colored_surfase_id,
                rt::specular_surface_id,
                rt::mesh_id,
                rt::close_box_id,
                rt::x_levels_id,
                rt::y_levels_id,
                rt::z_levels_id,};
  QString hints[7]={
          "Colored surface",
          "Specular surface",
          "Mesh",
          "Close box",
          "Level lines at x",
          "Level lines at y",
          "Level lines at z"
  };

  QString strs[7]={"C","S","M","B","X","Y","Z"};

  for(int i=0;i<7;++i)
  {
      CFlagButton*b=new
      CFlagButton(flags[i],glPlot3D.IsFlag(flags[i]));
      connect(b,SIGNAL(m_toggled_sig(int,bool)),
                SLOT(m_3D_toolbar_slot(int,bool)));
      b->setMaximumHeight(45);
      b->setMaximumWidth(45);
      b->setText(strs[i]);
      b->setToolTip(hints[i]);
      m_3D_toolbar->addWidget(b);
  }
  QPushButton*clear=new QPushButton("E");
  clear->setToolTip("Clear");
  clear->setMaximumHeight(45);
  clear->setMaximumWidth(45);
  connect(clear,SIGNAL(clicked()),
          this, SLOT(m_Clear()));
  m_3D_toolbar->addWidget(clear);

  m_2D_toolbar= std::make_unique<QToolBar>("2D toolbar");
  clear=new QPushButton("E");
  clear->setToolTip("Clear");
  clear->setMaximumHeight(45);
  clear->setMaximumWidth(45);
  connect(clear,SIGNAL(clicked()),
          this, SLOT(m_Clear()));
  m_2D_toolbar->addWidget(clear);

  auto cw=static_cast<CViewWidget*>(centralWidget());
  assert(cw);
  if(cw->ViewType()==CViewWidget::qt_view_id)
  {
      addToolBar(Qt::LeftToolBarArea,m_2D_toolbar.get());
  }
  else
  {
      addToolBar(Qt::LeftToolBarArea,m_3D_toolbar.get());
  }
}

void CMainWindow::m_3D_toolbar_slot(int i,bool b)
{
  assert(b!=glPlot3D.IsFlag(i));
  if(b) glPlot3D.SetFlag(i,Now());
  else glPlot3D.DropFlag(i,Now());
}


void CMainWindow::m_ConstantsDialog()
{
  auto dg_ptr=std::make_unique<CEditConstantsWidget>(glDataPool);
  auto res=dg_ptr->exec();
  assert(res==QDialog::Rejected);
}

void CMainWindow::m_BuiltInConstants()
{
  std::vector<typename string_map_t<float>::const_iterator> its;
  auto&map=CFunctionPool::fp_expression_t::m_builtin_constant;
  for(auto iter=map.begin();iter!=map.end();++iter)
    its.push_back(iter);
  auto data=[&its](int r,int c)
  {
      if(c==0)      return its[r]->first;
      else if(c==1) return std::to_string(its[r]->second);
      else assert(false);
  };
  auto dg_ptr=std::make_unique<CInfoTableDialog>
              (CInfoTableDialog::ok_id,
               std::pair(its.size(),2),
               data);
  auto res=dg_ptr->exec();
  assert(res==QDialog::Accepted);
}

void CMainWindow::m_FunctionsDialog()
{
  auto dg_ptr=std::make_unique<CEditFunctionsWidget>(glDataPool);
  auto res=dg_ptr->exec();
  assert(res==QDialog::Rejected);
}

void CMainWindow::m_BuiltInFunctions()
{
  std::vector<typename builtin_function_map_t<float>::const_iterator> its;
  auto&map=CFunctionPool::fp_expression_t::m_builtin_function;
  for(auto iter=map.begin();iter!=map.end();++iter)
    its.push_back(iter);
  auto data=[&its](int r,int c)
  {
      assert(c==0);
      return its[r]->first;
  };
  auto dg_ptr=std::make_unique<CInfoTableDialog>
              (CInfoTableDialog::ok_id,
               std::pair(its.size(),1),
               data);
  auto res=dg_ptr->exec();
  assert(res==QDialog::Accepted);
}

void CMainWindow::m_2D_settings()
{
  auto vw=static_cast<CViewWidget*>(centralWidget());
  int pts=CPlot2D::traits_t::m_num_points;
  int grid=vw->m_plot.m_num_hor_grid;
  auto pts_lab=new QLabel("Number of points:");
  auto pts_le=new QLineEdit(QString::number(pts));
  auto grid_lab=new QLabel("Number of grid lines:");
  auto grid_le=new QLineEdit(QString::number(grid));
  auto scaling_lab=new QLabel("Scaling:");
  auto scaling_le=new QComboBox();
  scaling_le->addItem("Accomodate",CPlot2D::accomodate_id);
  scaling_le->addItem("Proportional",CPlot2D::save_ratio_id);
  int index=(vw->m_plot.m_scaling==CPlot2D::accomodate_id)? 0:1;
  scaling_le->setCurrentIndex(index);
  auto dg_ptr=std::make_unique<CBaseDialog>
              (CBaseDialog::layout_t{{pts_lab,pts_le},
                                     {grid_lab,grid_le},
                                     {scaling_lab,scaling_le}});
  auto preprocess=[this,&vw](const CBaseDialog&dg)
  ->std::string
  {
      bool is_valid;
      int pts=dg.Qtext(0,1).toInt(&is_valid);
      if(!is_valid||pts<0)
      {
          return "Number of points must be integer greater 0";
      }
      int grid=dg.Qtext(1,1).toInt(&is_valid);
      if(!is_valid||grid<0)
      {
          return "Number of grid lines must be integer greater 0";
      }
      CPlot2D::traits_t::m_num_points=pts;
      vw->m_plot.SetGridLines(grid,grid);
      int ind=dg.Widget<QComboBox>(2,1).currentIndex();
      if(ind==0)vw->m_plot.m_scaling=CPlot2D::accomodate_id;
      else      vw->m_plot.m_scaling=CPlot2D::save_ratio_id;
      return {};
  };
  dg_ptr->SetPreprocessor(preprocess);
  dg_ptr->exec();
}

void CMainWindow::m_3D_settings()
{
  std::vector<QString> v_str={"Number of grid lines:",
                              "Number of levels lines:",
                              "Camera speed:",
                              "Camera rotation speed:"};
  auto dg_ptr=LinesEditDg(v_str);
  auto vw=static_cast<CViewWidget*>(centralWidget());
  auto rt=vw->m_3d_renderer.RenderingTraits();
  dg_ptr->SetText(0,1,std::to_string(rt.m_num_s_range));
  dg_ptr->SetText(1,1,std::to_string(rt.m_level_lines));
  dg_ptr->SetText(2,1,std::to_string(vw->m_3d_renderer.m_move_velocity));
  dg_ptr->SetText(3,1,std::to_string(vw->m_3d_renderer.ViewVelocity()));
  auto preprocess=[this,&vw,&rt](const CBaseDialog&dg)
  ->std::string
  {
      std::string str;
      auto ints=ParseInts(dg,{0,1},str);
      if(!str.empty()) return str;
      if(ints[0]<10||ints[0]>100)
      {
          return "Acceptable range for number of grid lines:{10,100}";
      }
      if(ints[1]<0||ints[1]>40)
      {
          return "Acceptable range for number of levels lines:{0,40}";
      }
      auto floats=ParseFloats(dg,{2,3},str,glDataPool);
      if(floats[2]<0)
      {
          return "Camera speed must be greater 0";
      }
      if(floats[3]<0)
      {
          return "Camera rotation speed must be greater 0";
      }
      vw->m_3d_renderer.m_move_velocity=floats[2];
      vw->m_3d_renderer.SetViewVelocity(floats[3]);
      if(rt.m_num_s_range!=ints[0]||
         rt.m_level_lines!=ints[1])
      {
          rt.m_num_s_range=rt.m_num_t_range=ints[0];
          rt.m_level_lines=ints[1];
          vw->m_3d_renderer.SetRenderingTraits(rt,Now());
      }
      return {};
  };
  dg_ptr->SetPreprocessor(preprocess);
  dg_ptr->exec();
}

void CMainWindow::m_ContextMenuDialog()
{
  std::vector<QString> v_str({"min x:","max x:","min y:","max y:"});
  auto dg=LinesEditDg(v_str);
  auto preprocess=[this](const CBaseDialog&dg)
  ->std::string
  {
      std::string str;
      using point_t=Eigen::Vector2f;
      using box_t=Eigen::AlignedBox<float,2>;
      auto floats=ParseFloats(dg,{0,1,2,3},str,glDataPool);
      if(!str.empty()) return str;
      if(floats[0]>=floats[1])return "min x must be less max x";
      if(floats[2]>=floats[3])return "min y must be less max y";
      auto vw=static_cast<CViewWidget*>(centralWidget());
      vw->m_plot.ResetBox(box_t(point_t(floats[0],floats[2]),
                                point_t(floats[1],floats[3])));
      return {};
  };
  dg->SetPreprocessor(preprocess);
  dg->exec();
}

void CMainWindow::mouseDoubleClickEvent(QMouseEvent*)
{
  auto vw=static_cast<CViewWidget*>(centralWidget());
  if(vw->ViewType()==CViewWidget::qt_view_id&&
     vw->m_3d_renderer.Empty()) return;
  assert(m_previous_dg.m_call);
  (this->*(m_previous_dg.m_call))();
}

/////////////////////////////////////////////
//                CConstantTable          ///
/////////////////////////////////////////////

CConstantTable::CConstantTable(CDataPool&data):
m_data(data),
m_constant(data.m_constant_pool)
{

}

int CConstantTable::rowCount(const QModelIndex &parent)const
{
  return m_constant.Size();
}

int CConstantTable::columnCount(const QModelIndex &parent)const
{
  return 2;
}

QVariant CConstantTable::data(const QModelIndex &index, int role) const
{
  std::size_t i=index.row();
  std::size_t j=index.column();
  if(i<m_constant.Size()&&role==Qt::DisplayRole)
    {
      if(j==0)
        {
          return QString::fromStdString(m_constant[i].name());
        }
      else if(j==1)
        {
          return m_constant[i].m_value;
        }
      else{}
    }
  return QVariant{};
}

QVariant CConstantTable::headerData(int section, Qt::Orientation orientation, int role)const
{
  static QString hor_headers[2]={"Name","Value"};
  if(role!=Qt::DisplayRole) return {};
  if(orientation==Qt::Horizontal)
    {
      return hor_headers[section];
    }
  else
    {
      return QString::number(section);
    }
}

void CConstantTable::Repaint()
{
  // dataChanged - save count of visible rows and cols
  //emit dataChanged(index(0,0,{}),index(m_constant.Size()-1,1,{}));
  beginResetModel();
  endResetModel();
}

bool CConstantTable::Insert(const std::string &str, double val)
{
  auto ins=m_data.RegisterConstant(str,val);
  if(!ins) return false;
  Repaint();
  return true;
}

void CConstantTable::SetView(QTableView*view)
{
  m_view=view;
  m_view->setModel(this);
}

void CConstantTable::RemoveRecord(const QModelIndex&mi)
{
  std::size_t r=mi.row();
  assert(r<m_constant.Size());
  m_data.SetDependencyContext(&m_constant[r]);
  if(m_data.GetContext().empty())
  {
      bool b=m_data.DeleteDependencyContext();
      assert(b);
      Repaint();
      return;
  }
  if(m_data.IsExternalContext())
  {
      QMessageBox::information(nullptr,
                               "Input error",
                               "This constant using in plot");
      return;
  }
  auto data=[this](int row,int col)
  {
      assert(col==0);
      return m_data.GetContext()[row]->name();
  };
  using info_t=CInfoTableDialog;
  auto dg=std::make_unique<info_t>
          (info_t::ok_id|info_t::cancel_id,
           std::pair{m_data.GetContext().size(),1},
           data);
  dg->setWindowTitle("Dependencies for delete");
  if(dg->exec()==QDialog::Accepted)
  {
      m_data.DeleteDependencyContext();
      Repaint();
  }
}

void CConstantTable::ResetRecord(const QModelIndex&mi,
                           float vt)
{
  std::size_t r=mi.row();
  assert(r<m_constant.Size());
  const_cast<CConstantPool::constant_t&>(m_constant[r]).m_value=vt;
  Repaint();
}

// CConstantTable controls

void CConstantTable::AddButtonClicked()
{
  std::vector<QString> v_str({"Name:","Value"});
  auto dg_ptr=LinesEditDg(v_str);
  auto preprocess=[this](const CBaseDialog&dg)
  ->std::string
  {
      const auto&kw=CMainWindow::Keywords;
      std::string str;
      auto floats=ParseFloats(dg,{1},str,glDataPool);
      if(!str.empty()) return str;
      auto iden=dg.Text(0,1);
      if(!std::regex_match(iden.begin(),iden.end(),
                         reg_identifier))
      {
          return "'"+iden+"'"+" is not correct identifier";
      }
      if(kw.find(iden)!=kw.end())
      {
          return "'"+iden+"'"+" is special argument";
      }
      if(!Insert(iden,floats[1]))
      {
          return "Name conflict detected";
      }
      return {};
  };
  dg_ptr->SetPreprocessor(preprocess);
  dg_ptr->exec();
}

void CConstantTable::RemoveButtonClicked()
{
  auto ixs=m_view->selectionModel()->selectedIndexes();
  assert(ixs.size()<=1);
  if(ixs.size()==0) return;
  RemoveRecord(ixs.front());
}

void CConstantTable::EditButtonClicked()
{
  auto ixs=m_view->selectionModel()->selectedIndexes();
  assert(ixs.size()<=1);
  if(ixs.size()==0) return;
  auto i=ixs.front().row();
  assert(i<rowCount());
  std::vector<QString> v_str({"Value"});
  auto dg_ptr=LinesEditDg(v_str);
  auto preprocess=[this,&ixs](const CBaseDialog&dg)
  ->std::string
  {
      std::string str;
      auto floats=ParseFloats(dg,{0},str,glDataPool);
      if(!str.empty()) return str;
      ResetRecord(ixs.front(),floats[0]);
      return {};
  };
  dg_ptr->SetText(0,1,std::to_string(m_constant[i].m_value));
  dg_ptr->SetPreprocessor(preprocess);
  dg_ptr->exec();

}

CEditConstantsWidget::CEditConstantsWidget(CDataPool&pool):
m_table(pool)
{
  m_view=new QTableView;
  m_table.SetView(m_view);
  setWindowTitle("Custom constants");

  QBoxLayout*thisLayout=new QBoxLayout(QBoxLayout::TopToBottom);
  thisLayout->addWidget(m_view);
  QBoxLayout* ltButtons = new QBoxLayout(QBoxLayout::LeftToRight);

  QPushButton*add=new QPushButton("Add");
  QObject::connect(add,SIGNAL(clicked()),
                   &m_table,SLOT(AddButtonClicked()));
  QPushButton*edit=new QPushButton("Edit");
  QObject::connect(edit,SIGNAL(clicked()),
                   &m_table,SLOT(EditButtonClicked()));
  QPushButton*remove=new QPushButton("Remove");
  QObject::connect(remove,SIGNAL(clicked()),
                   &m_table,SLOT(RemoveButtonClicked()));
  QPushButton*close=new QPushButton("Close");
  QObject::connect(close,SIGNAL(clicked()),
                   this,SLOT(reject()));

  ltButtons->addWidget(add);
  ltButtons->addWidget(edit);
  ltButtons->addWidget(remove);
  ltButtons->addWidget(close);

  thisLayout->addLayout(ltButtons);
  setLayout(thisLayout);
}

//////////////////////////////////////////////////
/// CFunctionsTable,CEditFunctionsWidget        //
//////////////////////////////////////////////////

CFunctionsTable::CFunctionsTable(CDataPool&pool):
m_pool(pool),
m_functions(m_pool.Functions())
{
}

int CFunctionsTable::rowCount(const QModelIndex&)const
{
  return m_functions.Size();
}

int CFunctionsTable::columnCount(const QModelIndex&)const
{
  return 3;
}

QVariant CFunctionsTable::headerData(int sec,Qt::Orientation orien,int role)const
{
  static QString hor_headers[3]={"Name","Args","Function"};
  if(role!=Qt::DisplayRole) return {};
  if(orien==Qt::Horizontal)
  {
      return hor_headers[sec];
  }
  else
  {
      return QString::number(sec);
  }
}

std::string CFunctionsTable::MakeArgsString(int i)const
{
  std::string res;
  for(const auto&arg:m_functions[i].args())
  {
      res+=arg;
      res+=",";
  }
  if(!res.empty()) res.pop_back();
  return res;
}

void CFunctionsTable::SetView(QTableView*view)
{
  m_view=view;
  m_view->setModel(this);
}

QVariant CFunctionsTable::data(const QModelIndex &index, int role) const
{
  std::size_t i=index.row();
  std::size_t j=index.column();
  if(i<m_functions.Size()&&role==Qt::DisplayRole)
  {
    if(j==0)
    {
         return QString::fromStdString(m_functions[i].name());
    }
    else if(j==1)
    {
        return QString::fromStdString(MakeArgsString(i));
    }
    else if(j==2)
    {
        return QString::fromStdString(m_functions[i].body());
    }
    else{}
  }
  return {};
}

void CFunctionsTable::Repaint()
{
  beginResetModel();
  endResetModel();
}

parse_error_t CFunctionsTable::ResetFunction(
                          int i,
                          const std::vector<std::string>&args,
                          const std::string&body)
{
  auto& f=const_cast<CFunctionPool::function_t&>(m_functions[i]);
  parse_error_t per=m_pool.ReparseFunction(f,args,body);
  if(per.is_valid()) Repaint();
  return per;
}

void CFunctionsTable::RemoveFunction(const QModelIndex&mi)
{
  int r=mi.row();
  m_pool.SetDependencyContext(&m_functions[r]);
  if(m_pool.GetContext().empty())
  {
      bool res=m_pool.DeleteDependencyContext();
      assert(res);
      Repaint();
      return;
  }
  if(m_pool.IsExternalContext())
  {
      QMessageBox::information(nullptr,
                               "Input error",
                               "This function using in plot");
      return;
  }
  //need check if plot use constant
  auto data=[this](int row,int col)
  {
      assert(col==0);
      return m_pool.GetContext()[row]->name();
  };
  using info_t=CInfoTableDialog;
  auto dg=std::make_unique<info_t>
          (info_t::ok_id|info_t::cancel_id,
           std::pair{m_pool.GetContext().size(),1},
           data);
  dg->setWindowTitle("Dependencies for delete");
  if(dg->exec()==QDialog::Accepted)
  {
      m_pool.DeleteDependencyContext();
      Repaint();
  }
}

parse_error_t CFunctionsTable::AddFunction(
                     const std::string&name,
                     const std::vector<std::string>&args,
                     const std::string&body)
{
  auto reg_res=m_pool.RegisterFunction(name,args,body);
  if(reg_res) Repaint();
  return reg_res.error();
}

// CFunctionTable controls

void CFunctionsTable::AddButtonClicked()
{
  std::vector<QString> v_str({"Name:","Args:","Body:"});
  auto dg_ptr=LinesEditDg(v_str);
  auto preprocess=[this](const CBaseDialog&dg)
  ->std::string
  {
      const auto&kw=CMainWindow::Keywords;
      auto iden=dg.Text(0,1);
      if(!std::regex_match(iden.begin(),iden.end(),
                         reg_identifier))
      {
          return "'"+iden+"'"+" is not correct identifier";
      }
      if(kw.find(iden)!=kw.end()) return "'"+iden+"'"+" is special argument";
      std::vector<std::string> args;
      std::string mess=ParseArgs(dg.Text(1,1),args);
      if(!mess.empty())
      {
          return mess;
      }
      parse_error_t pe=AddFunction(iden,args,dg.Text(2,1));
      if(!pe.is_valid()) return "Parse error:"+pe.detail();
      return {};
  };
  dg_ptr->SetPreprocessor(preprocess);
  dg_ptr->exec();
}

void CFunctionsTable::EditButtonClicked()
{
  auto ixs=m_view->selectionModel()->selectedIndexes();
  assert(ixs.size()<=1);
  if(ixs.size()==0) return;
  auto i=ixs.front().row();
  assert(i<rowCount());
  std::vector<QString> v_str({"Name:","Args:","Body:"});
  auto dg_ptr=LinesEditDg(v_str);
  dg_ptr->SetText(0,1,m_functions[i].name());
  dg_ptr->Widget<QLineEdit>(0,1).setReadOnly(true);
  dg_ptr->SetText(1,1,MakeArgsString(i));
  dg_ptr->SetText(2,1,m_functions[i].body());
  auto preprocess=[this,i](const CBaseDialog&dg)
  ->std::string
  {
      std::vector<std::string> args;
      std::string mess=ParseArgs(dg.Text(1,1),args);
      if(!mess.empty())
      {
          return mess;
      }
      if(args.size()!=m_functions[i].args().size())
      {
          return "The number of function arguments must not change";
      }
      parse_error_t pe=ResetFunction(i,args,dg.Text(2,1));
      if(!pe.is_valid()) return "Parse error:"+pe.detail();
      return {};
  };
  dg_ptr->SetPreprocessor(preprocess);
  dg_ptr->exec();
}

void CFunctionsTable::RemoveButtonClicked()
{
  auto ixs=m_view->selectionModel()->selectedIndexes();
  assert(ixs.size()<=1);
  if(ixs.size()==0) return;
  assert(ixs.front().row()<rowCount());
  RemoveFunction(ixs.front());
}

CEditFunctionsWidget::CEditFunctionsWidget(CDataPool&pool):
m_table(pool)
{
  m_view=new QTableView;
  m_table.SetView(m_view);
  setWindowTitle("Custom functions");

  QBoxLayout*thisLayout=new QBoxLayout(QBoxLayout::TopToBottom);
  thisLayout->addWidget(m_view);
  QBoxLayout* ltButtons = new QBoxLayout(QBoxLayout::LeftToRight);

  QPushButton*add=new QPushButton("Add");
  QObject::connect(add,SIGNAL(clicked()),
                   &m_table,SLOT(AddButtonClicked()));
  QPushButton*edit=new QPushButton("Edit");
  QObject::connect(edit,SIGNAL(clicked()),
                   &m_table,SLOT(EditButtonClicked()));
  QPushButton*remove=new QPushButton("Remove");
  QObject::connect(remove,SIGNAL(clicked()),
                   &m_table,SLOT(RemoveButtonClicked()));
  QPushButton*close=new QPushButton("Close");
  QObject::connect(close,SIGNAL(clicked()),
                   this,SLOT(reject()));

  ltButtons->addWidget(add);
  ltButtons->addWidget(edit);
  ltButtons->addWidget(remove);
  ltButtons->addWidget(close);

  thisLayout->addLayout(ltButtons);
  setLayout(thisLayout);
}










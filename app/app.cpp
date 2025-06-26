/*
Copyright(C) 2019-2025 Adrian Mansell

This program is free software : you can redistribute it and /or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.If not, see < https://www.gnu.org/licenses/>.
*/
#include <exception>
#include <iostream>
#include <utility>

#include <QCoreApplication>
#include <QTabWidget>

#include "app.h"
#include "dxf.h"
#include "hpgl.h"
#include "tabs.h"
#include "wing.h"

App::App()
   : QTabW(new QTabWidget) {
   setCentralWidget(&QTabW);
   connect(&QTabW, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

   fileToolBar.reset(addToolBar(tr("File")));

   checkCreateDefaultDirectory();
   createActions();
   createStatusBar();

   // JSON configuration for the gui
   json config = loadConfigJson();
   createGenericTabs(config);

   createPreviewTab(planv, plans, plangvz, planrl, &planIdx, "Plan");
   createPreviewTab(partv, parts, partgvz, partrl, &partIdx, "Parts");
   createFormer1Tab();
   QApplication::restoreOverrideCursor();

   // Open associated file
   if (QApplication::arguments().size() > 1) {
      const QString filename = QApplication::arguments().at(1);

      QFileInfo fi(filename);
      // Check it has a .acad extension
      if ((fi.suffix() == FILE_SUFFIX)) {
         // Split into path and filename
         currPath = fi.path();
         currFile = fi.fileName();
         openCore();
      }
      else {
         dbg::fatal(SS("File is not an ACAD file"), SS("Expected file extension ") + SS(FILE_SUFFIX));
      }
   }
}

void App::checkCreateDefaultDirectory() {
   currPath = QDir::currentPath().append("/examples");
}

void App::clearTabs() {
   for (auto it = tabMap.begin(); it != tabMap.end(); ++it) {
      it->second->ClearData();
   }
}

void App::createActions() {
   fileToolBar->setMovable(false);
   const QIcon newIcon = QIcon(":/images/new.png");
   QAction* newAct = new QAction(newIcon, tr("&New"), this);
   newAct->setShortcuts(QKeySequence::New);
   newAct->setStatusTip(tr("Create a new file"));
   connect(newAct, &QAction::triggered, this, &App::newFile);
   fileToolBar->addAction(newAct);

   const QIcon openIcon = QIcon(":/images/open.png");
   QAction* openAct = new QAction(openIcon, tr("&Open..."), this);
   openAct->setShortcuts(QKeySequence::Open);
   openAct->setStatusTip(tr("Open an existing file"));
   connect(openAct, &QAction::triggered, this, &App::open);
   fileToolBar->addAction(openAct);

   const QIcon saveIcon = QIcon(":/images/save.png");
   QAction* saveAct = new QAction(saveIcon, tr("&Save"), this);
   saveAct->setShortcuts(QKeySequence::Save);
   saveAct->setStatusTip(tr("Save the document to disk"));
   connect(saveAct, &QAction::triggered, this, &App::save);
   fileToolBar->addAction(saveAct);

   const QIcon saveAsIcon = QIcon(":images/save_as.png");
   QAction* saveAsAct = new QAction(saveAsIcon, tr("Save &As..."), this);
   saveAsAct->setShortcuts(QKeySequence::SaveAs);
   saveAsAct->setStatusTip(tr("Save the document under a new name"));
   connect(saveAsAct, &QAction::triggered, this, &App::saveAs);
   fileToolBar->addAction(saveAsAct);

   const QIcon buildIconHpgl = QIcon(":images/export_hpgl.png");
   QAction* buildActHpgl = new QAction(buildIconHpgl, tr("Build wing and export design files to HPGL"), this);
   saveAsAct->setStatusTip(tr("Build wing and export design files to HPGL"));
   connect(buildActHpgl, &QAction::triggered, this, &App::buildHpgl);
   fileToolBar->addAction(buildActHpgl);

   const QIcon buildIconDxf = QIcon(":images/export_dxf.png");
   QAction* buildActDxf = new QAction(buildIconDxf, tr("Build wing and export design files to DXF"), this);
   saveAsAct->setStatusTip(tr("Build wing and export design files to DXF"));
   connect(buildActDxf, &QAction::triggered, this, &App::buildDxf);
   fileToolBar->addAction(buildActDxf);

   draftCb.setChecked(true);
   connect(&draftCb, SIGNAL(clicked(bool)), this, SLOT(draftEvent(bool)));
   fileToolBar->addWidget(&draftCb);

   DBGLVL1("Toolbar and actions created");
}

void App::createGenericTabs(const json& cfg) {
   if (cfg.count("tabs")) {
      // LOOP THROUGH TABS
      json tbs = cfg["tabs"];
      for (json::iterator tb = tbs.begin(); tb != tbs.end(); ++tb) {
         // Create the tab
         std::string tabkey = tb->at("key");
         GenericTab* tab = new GenericTab(&QTabW, *tb);
         tabMap.emplace(tabkey, tab);

         QTabW.setTabToolTip(QTabW.count() - 1, QString::fromStdString(tb->at("help")));
         DBGLVL1("Created generic tab: %s", tabkey.c_str());

         // Add entry parts
         if (tb->count("entry_parts")) {
            json eps = tb->at("entry_parts");
            for (json::iterator ep = eps.begin(); ep != eps.end(); ++ep) {
               json entry = *ep;
               tab->AddEntry(entry);
            }
         }
      }
   }
}

void App::createPreviewTab(
   QGraphicsView& view,
   QGraphicsScene& scene,
   std::unique_ptr<zoomer>& zm,
   std::unique_ptr<ruler>& rl,
   int* idx,
   const char* title) {
   zm = std::unique_ptr<zoomer>(new zoomer(&view));
   zm->setModifiers(Qt::NoModifier);
   view.setDragMode(QGraphicsView::ScrollHandDrag);

   QTabW.addTab(&view, QString(title));
   *idx = QTabW.count() - 1;
   view.setScene(&scene);
   rl = std::unique_ptr<ruler>(new ruler);
   scene.installEventFilter(rl.get());
   connect(rl.get(), SIGNAL(userMessage(std::string)), this, SLOT(showStatusBarMsg(std::string)));

   rescalePreviews = true;

   DBGLVL1("Created preview tab");
}

void App::createFormer1Tab() {
   frm1gvz = new zoomer(&frm1v);
   frm1gvz->setModifiers(Qt::NoModifier);
   frm1v.setDragMode(QGraphicsView::ScrollHandDrag);

   frm1LHBWSpin.setMinimum(1);
   frm1LHBWSpin.setMaximum(100.0);
   frm1LHBWSpin.setSingleStep(1.0);
   frm1LHBWSpin.setValue(20.0);
   frm1LHBWSpin.setMaximumWidth(120);
   frm1LHBWSpin.setDecimals(0);
   frm1LHBWSpin.setMaximumWidth(120);

   frm1GirdOwSpin.setMinimum(0.1);
   frm1GirdOwSpin.setMaximum(100.0);
   frm1GirdOwSpin.setSingleStep(0.1);
   frm1GirdOwSpin.setValue(2.0);
   frm1GirdOwSpin.setMaximumWidth(120);
   frm1GirdOwSpin.setDecimals(1);
   frm1GirdOwLabel.setMaximumWidth(120);

   frm1GirdIwSpin.setMinimum(0.1);
   frm1GirdIwSpin.setMaximum(100.0);
   frm1GirdIwSpin.setSingleStep(0.1);
   frm1GirdIwSpin.setValue(2.0);
   frm1GirdIwSpin.setMaximumWidth(120);
   frm1GirdIwSpin.setDecimals(1);
   frm1GirdIwLabel.setMaximumWidth(120);

   frm1GirdBwSpin.setMinimum(0.1);
   frm1GirdBwSpin.setMaximum(100.0);
   frm1GirdBwSpin.setSingleStep(0.1);
   frm1GirdBwSpin.setValue(2.0);
   frm1GirdBwSpin.setMaximumWidth(120);
   frm1GirdBwSpin.setDecimals(1);
   frm1GirdBwLabel.setMaximumWidth(120);

   frm1GirdAsSpin.setMinimum(5.0);
   frm1GirdAsSpin.setMaximum(200.0);
   frm1GirdAsSpin.setSingleStep(1.0);
   frm1GirdAsSpin.setValue(30.0);
   frm1GirdAsSpin.setMaximumWidth(120);
   frm1GirdAsSpin.setDecimals(0);
   frm1GirdAsLabel.setMaximumWidth(120);

   frm1GirdMaSpin.setMinimum(5.0);
   frm1GirdMaSpin.setMaximum(60.0);
   frm1GirdMaSpin.setSingleStep(5);
   frm1GirdMaSpin.setValue(30.0);
   frm1GirdMaSpin.setMaximumWidth(120);
   frm1GirdMaSpin.setDecimals(0);
   frm1GirdMaLabel.setMaximumWidth(120);

   frm1SplitAtYSpin.setMinimum(-LARGE);
   frm1SplitAtYSpin.setMaximum(LARGE);
   frm1SplitAtYSpin.setMaximumWidth(120);
   frm1SplitAtYSpin.setDecimals(1);
   frm1SplitAtYLabel.setMaximumWidth(120);

   frm1ImpFile.setMaximumWidth(150);
   frm1ImpFile.setToolTip(tr("Import a HPGL file"));
   connect(&frm1ImpFile, &QPushButton::released, this, &App::former1Import);

   frm1ExpFile.setMaximumWidth(150);
   frm1ExpFile.setToolTip(tr("Export processed file"));
   connect(&frm1ExpFile, &QPushButton::released, this, &App::former1Export);

   frm1Process.setMaximumWidth(150);
   frm1Process.setToolTip(tr("Process the imported formers according to the configured settings"));
   connect(&frm1Process, &QPushButton::released, this, &App::former1Execute);

   frm1LiteEnabled.setChecked(false);
   frm1LiteNotchDet.setChecked(true);
   frm1GirdEnabled.setChecked(false);
   frm1GirdShowConst.setChecked(false);
   frm1Hsplit.setChecked(false);

   frm1GirdCompass.addItem(QString("South"));
   frm1GirdCompass.addItem(QString("North"));
   frm1GirdCompass.addItem(QString("East"));
   frm1GirdCompass.addItem(QString("West"));
   frm1GirdCompass.addItem(QString("Notches"));
   frm1GirdCompass.setToolTip(QString("Select compass point at which to start the girdering"));

   frm1ActionButtons.addWidget(&frm1ImpFile);
   frm1ActionButtons.addWidget(&frm1ExpFile);
   frm1ActionButtons.addWidget(&frm1Process);
   frm1ActionFrame.setFrameStyle(QFrame::StyledPanel);
   frm1ActionFrame.setLineWidth(2);
   frm1ActionFrame.setLayout(&frm1ActionButtons);

   frm1LiteButtons.addWidget(&frm1LiteEnabled);
   frm1LiteButtons.addWidget(&frm1LiteNotchDet);
   frm1LiteButtons.addStretch(0);
   frm1LiteValues.addWidget(&frm1LHBWLabel);
   frm1LiteValues.addWidget(&frm1LHBWSpin);
   frm1LiteValues.addStretch(0);
   frm1LiteVbox.addLayout(&frm1LiteButtons);
   frm1LiteVbox.addLayout(&frm1LiteValues);
   frm1LiteVbox.addWidget(&frm1Progress);
   frm1LiteFrame.setFrameStyle(QFrame::StyledPanel);
   frm1LiteFrame.setLineWidth(2);
   frm1LiteFrame.setLayout(&frm1LiteVbox);

   frm1GirdButtons.addWidget(&frm1GirdEnabled);
   frm1GirdButtons.addWidget(&frm1GirdShowConst);
   frm1GirdButtons.addWidget(&frm1GirdCompassLabel);
   frm1GirdButtons.addWidget(&frm1GirdCompass);
   frm1GirdButtons.addStretch(0);
   frm1GirdValues.addWidget(&frm1GirdAsLabel);
   frm1GirdValues.addWidget(&frm1GirdAsSpin);
   frm1GirdValues.addWidget(&frm1GirdOwLabel);
   frm1GirdValues.addWidget(&frm1GirdOwSpin);
   frm1GirdValues.addWidget(&frm1GirdIwLabel);
   frm1GirdValues.addWidget(&frm1GirdIwSpin);
   frm1GirdValues.addWidget(&frm1GirdBwLabel);
   frm1GirdValues.addWidget(&frm1GirdBwSpin);
   frm1GirdValues.addWidget(&frm1GirdMaLabel);
   frm1GirdValues.addWidget(&frm1GirdMaSpin);
   frm1GirdValues.addStretch(0);
   frm1GirdVbox.addLayout(&frm1GirdButtons);
   frm1GirdVbox.addLayout(&frm1GirdValues);
   frm1GirdFrame.setFrameStyle(QFrame::StyledPanel);
   frm1GirdFrame.setLineWidth(2);
   frm1GirdFrame.setLayout(&frm1GirdVbox);

   frm1SplitButtons.addWidget(&frm1Hsplit);
   frm1SplitButtons.addWidget(&frm1Vsplit);
   frm1SplitButtons.addStretch(0);
   frm1SplitValues.addWidget(&frm1SplitAtYLabel);
   frm1SplitValues.addWidget(&frm1SplitAtYSpin);
   frm1SplitValues.addStretch(0);
   frm1SplitVbox.addLayout(&frm1SplitButtons);
   frm1SplitVbox.addLayout(&frm1SplitValues);
   frm1SplitFrame.setFrameStyle(QFrame::StyledPanel);
   frm1SplitFrame.setLineWidth(2);
   frm1SplitFrame.setLayout(&frm1SplitVbox);

   frm1MenuBar.addWidget(&frm1ActionFrame);
   frm1MenuBar.addWidget(&frm1LiteFrame);
   frm1MenuBar.addWidget(&frm1GirdFrame);
   frm1MenuBar.addWidget(&frm1SplitFrame);
   frm1MenuBar.addStretch(0);

   frm1Layout.addLayout(&frm1MenuBar);
   frm1Layout.addWidget(&frm1v);
   QWidget* qw = new QWidget(this);
   qw->setLayout(&frm1Layout);

   QTabW.addTab(qw, tr("Former"));
   frm1idx = QTabW.count() - 1;
   frm1v.setScene(&frm1s);
   frm1s.installEventFilter(&frm1rl);
   connect(&frm1rl, SIGNAL(userMessage(std::string)), this, SLOT(showStatusBarMsg(std::string)));

   DBGLVL1("Created Former tab");
}

void App::createStatusBar() {
   statusBar()->showMessage(tr("Ready"));
   DBGLVL1("Created status bar");
}

QString App::currentFileName() {
   QString fn = currPath;
   fn.append("/");
   fn.append(currFile);
   return fn;
}

bool App::getFileName(bool saveNotOpen) {
   // Get a filename using standard dialogue
   QString filename;
   if (saveNotOpen)
      filename = QFileDialog::getSaveFileName(this, tr("Save As"), currPath, tr(FILE_FILTER));
   else
      filename = QFileDialog::getOpenFileName(this, tr("Open File"), currPath, tr(FILE_FILTER));

   if (filename.isEmpty())
      return false;

   QFileInfo fi(filename);
   if (saveNotOpen) {
      // Enforce it has a .acad extension
      if (fi.suffix().isEmpty() || (fi.suffix() != FILE_SUFFIX)) {
         filename.append(FILE_EXTENSION);
         fi.setFile(filename);
      }
   }

   // Split into path and filename
   currPath = fi.path();
   currFile = fi.fileName();

   return true;
}

json App::loadConfigJson() {
   QDir path = { QCoreApplication::applicationDirPath().append("/config.json") };
   std::string strpath = path.absolutePath().toStdString();
   std::ifstream cfgFile(strpath);
   if (!cfgFile.is_open())
      dbg::fatal(SS("Unable to open configuration file"), SS("Expected to find file ") + strpath);
   return json::parse(cfgFile);
}

void App::needsSaving() {
   if (!GenericTab::getModelChangedSave())
      return;

   const QMessageBox::StandardButton ret = QMessageBox::warning(this, tr("ACAD"),
      tr("Save changes to the model?"),
      QMessageBox::Save | QMessageBox::Discard);

   if (ret == QMessageBox::Save)
      save();

   GenericTab::setModelChangedSave(false);
   return;
}

void App::updatePreview(QGraphicsView& view, QGraphicsScene& scene, obj& object) {
   scene.clear();
   QPen prvpen(Qt::black, 0.3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

   // Add all line elements from the object into the scene
   for (auto ln = object.begin(); ln != object.end(); ln++)
      scene.addLine(ln->get_S0().x, -ln->get_S0().y, ln->get_S1().x, -ln->get_S1().y, prvpen);

   // Add corner frames
   coord_t tl = { 0.0, -10.0 };
   coord_t br = { 10.0, 0.0 };
   if (object.size()) {
      tl = coord_t{ object.find_extremity(LEFT) - 20, -(object.find_extremity(UP) + 20) };
      br = coord_t{ object.find_extremity(RIGHT) + 20, -(object.find_extremity(DOWN) - 20) };
   }
   scene.addLine(tl.x, tl.y + 5, tl.x, tl.y);
   scene.addLine(tl.x, tl.y, tl.x + 5, tl.y);
   scene.addLine(br.x, br.y - 5, br.x, br.y);
   scene.addLine(br.x, br.y, br.x - 5, br.y);

   if (rescalePreviews)
      view.fitInView(scene.sceneRect(), Qt::KeepAspectRatio);
}

void App::closeEvent(QCloseEvent* event) {
   needsSaving();
   event->accept();
}

void App::draftEvent(bool state) {
   GenericTab::setModelChangedPrvw(true);
   DBGLVL1("Draft preview mode set to: %d", (int)(state));
}

void App::openCore() {
   clearTabs();

   // Open the file
   QFile fd(currentFileName());
   if (!fd.open(QIODevice::ReadOnly)) {
      dbg::alert(SS("Unable to open file for reading:"), fd.fileName().toStdString());
      return;
   }
   DBGLVL1("File opened (load): %s", fd.fileName().toStdString().c_str());
   QDataStream sv(&fd);
   sv.setVersion(QT_STREAM_VERSION);

   // Check the ACAD version
   QString str;
   sv >> str;
   if (str != VERSION)
      dbg::alert(SS("This file is from a different version of ACAD; default values may be used."),
         SS("File version is ") + str.toStdString() + ", application version is " + VERSION);

   // Parse the data chunks in the file
   while (!sv.atEnd()) {
      sv >> str;
      if (str == "TAB") {
         // Recognised the word TAB, so complete the transaction
         sv.commitTransaction();

         // Read in the key
         sv >> str;
         if (tabMap.count(str.toStdString())) {
            tabMap.at(str.toStdString())->load(sv);
            DBGLVL1("Loaded tab %s", str.toStdString().c_str());
         }
         else {
            DBGLVL1("Tab named %s is not supported in this version of ACAD, it will be ignored.",
               str.toStdString().c_str());
            // Dump the contents of the unknown TAB
            int exRows, exCols;
            sv >> exRows; // Rows
            sv >> exCols; // Cols
            for (int exR = 0; exR < exRows; exR++)
               for (int exC = 0; exC < exCols; exC++) {
                  QStandardItem QsiTmp;
                  sv >> QsiTmp;
               }
         }
      }
      else
         DBGLVL1("Unrecognised data chunk type %s - skipping on", str.toStdString().c_str());
   }

   GenericTab::setModelChangedSave(false);
   GenericTab::setModelChangedPrvw(true);
   rescalePreviews = true;
   plans.clear();
   parts.clear();
   QWidget::setWindowTitle(currFile);
}

void App::open() {
   // Preparation: save changes; get filename; clear data
   needsSaving();
   if (!getFileName(false)) {
      DBGLVL1("Ignoring empty/faulty filename (open)");
      return;
   }

   openCore();
}

void App::newFile() {
   needsSaving();
   currFile.clear();
   clearTabs();
   GenericTab::setModelChangedSave(false);
   GenericTab::setModelChangedPrvw(true);
   rescalePreviews = true;
   plans.clear();
   parts.clear();
   QWidget::setWindowTitle(QString("ACAD"));
}

void App::save() {
   if (currFile.isEmpty())
      if (!getFileName(true)) {
         DBGLVL1("Ignoring empty/faulty filename (open)");
         return;
      }

   QFile fd(currentFileName());
   if (!fd.open(QIODevice::WriteOnly)) {
      dbg::alert(SS("Unable to open file for writing:"), fd.fileName().toStdString());
      return;
   }
   DBGLVL1("File opened (save): %s", fd.fileName().toStdString().c_str());

   QDataStream sv(&fd);
   sv.setVersion(QT_STREAM_VERSION);

   sv << QString(VERSION);

   for (auto it = tabMap.begin(); it != tabMap.end(); ++it) {
      sv << QString("TAB");
      it->second->save(sv);
      DBGLVL1("Saved tab %s", it->second->GetKey().c_str());
   }

   fd.close();

   GenericTab::setModelChangedSave(false);
   QWidget::setWindowTitle(currFile);
}

void App::saveAs() {
   if (getFileName(true))
      save();
}

void App::showStatusBarMsg(std::string msg) {
   QStatusBar* sb = statusBar();
   sb->showMessage(QString::fromStdString(msg), 1000000);
}

void App::tabChanged(int tabIdx) {
   if ((tabIdx == planIdx) || (tabIdx == partIdx)) {
      // Preview tabs have a different cursor
      QApplication::restoreOverrideCursor();
      QApplication::setOverrideCursor(Qt::CrossCursor);

      // If the model data has changed, rebuild and redraw the previews
      if (GenericTab::getModelChangedPrvw()) {
         DBGLVL1("Preview tab selected with getModelChangedPrvw == true");
         Wing w = {};
         if (draftCb.isChecked())
            buildWingModel(w, true);
         else
            buildWingModel(w);
         updatePreview(planv, plans, w.getPlan());
         updatePreview(partv, parts, w.getParts());
         GenericTab::setModelChangedPrvw(false);
         rescalePreviews = false;
      }
   }
   else {
      // Preview tabs have a different cursor - needs to be reverted
      QApplication::restoreOverrideCursor();
   }
}

void App::former1Import() {
   QString filename = QFileDialog::getOpenFileName(this, tr("Import HPGL File"), currPath, tr("HPGL Files (*.plt)"));

   if (filename.isEmpty())
      return;

   // Split into path and filename
   QFileInfo fi(filename);
   currPath = fi.path();

   // Import
   frm1State = frm1States::EMPTY;
   FILE* fp = fopen(fi.absoluteFilePath().toStdString().c_str(), "r");
   if (!fp) {
      dbg::alert(tr("Unable to open file.").toStdString());
      frm1Result.del();
      frm1Import.del();
   }
   else {
      frm1Result = importHpglFile(fp);
      frm1Import = frm1Result;
      fclose(fp);
      if (frm1Result.empty()) {
         dbg::alert(tr("File import failed (empty object).").toStdString());
         return;
      }
      frm1State = frm1States::IMPORTED;
   }

   updateFormer1Preview();
}

void App::former1Export() {
   // Check we have something to export
   if (frm1State != frm1States::PROCESSED) {
      dbg::alert(SS("Nothing to export!"));
      return;
   }

   // Get a filename using standard dialogue
   QString filename;
   filename = QFileDialog::getSaveFileName(this, tr("Export As"), currPath, tr("HPGL Files (*.plt);;DXF Files (*.dxf)"));
   if (filename.isEmpty())
      return;

   QFileInfo fi(filename);
   // No extension? Default to DXF.
   if (fi.suffix().isEmpty())
      filename.append(".dxf");
   else if ((fi.suffix() != "plt") && (fi.suffix() != "dxf")) {
      dbg::alert(SS("Please choose either a .plt or .dxf file."));
      return;
   }

   fi.setFile(filename);
   FILE* fd = fopen(fi.absoluteFilePath().toStdString().c_str(), "w");
   if (!fd) {
      dbg::alert(SS("Unable to open file for writing:"), fi.absoluteFilePath().toStdString());
      return;
   }
   DBGLVL1("File opened (export): %s", fi.absoluteFilePath().toStdString().c_str());

   if (fi.suffix() == "plt") {
      exportObjHpglFile(fd, frm1Result);
   }
   else if (fi.suffix() == "dxf") {
      dxf_export dxf = {};
      obj tmp = frm1Result;
      tmp.move_origin_to(coord_t{ 0.0, 0.0 });
      dxf.add_object(tmp);
      dxf.write(fd);
   }

   fclose(fd);
   return;
}

void App::updateFormer1Preview() {
   frm1s.clear();
   updatePreview(frm1v, frm1s, frm1Result);
}

void App::former1Execute() {
   switch (frm1State) {
   case frm1States::EMPTY:
      dbg::alert(SS("Please import a drawing to process."));
      return;
   case frm1States::PROCESSED:
      frm1Result = frm1Import;
      frm1State = frm1States::IMPORTED;
      updateFormer1Preview();
      break;
   default:
      break;
   }

   if (frm1GirdEnabled.isChecked() && !frm1LiteEnabled.isChecked()) {
      dbg::alert(SS("Please enable lightening to use girdering tool."));
      return;
   }

   // Parse the direction compass box
   direction_e stDir;
   bool anchorAtNotches = false;
   switch (frm1GirdCompass.currentIndex()) {
   case 0:
      stDir = DOWN;
      break;
   case 1:
      stDir = UP;
      break;
   case 2:
      stDir = RIGHT;
      break;
   case 3:
      stDir = LEFT;
      break;
   case 4:
      anchorAtNotches = true;
      stDir = DOWN;
      break;
   default:
      dbg::alert(SS("Unknown compass direction, default to South."));
      stDir = DOWN;
      break;
   }

   // Separate out each closed path ready for processing
   std::list<obj> closed = {};
   std::list<obj> open = {};
   frm1Result.make_path(SNAP_LEN, closed, open);
   frm1Progress.setMaximum(LiteEngine::progressBarSteps * closed.size());
   frm1Progress.setMinimum(0);
   frm1Progress.reset();

   // Process each closed path
   std::list<obj> result = {};
   for (auto& input : closed) {
      qApp->processEvents();
      LiteEngine ge{
          frm1LHBWSpin.value(),
          frm1GirdOwSpin.value(),
          frm1GirdIwSpin.value(),
          frm1GirdBwSpin.value(),
          frm1GirdAsSpin.value(),
          frm1GirdMaSpin.value(),
          frm1SplitAtYSpin.value(),
          stDir,
          LiteEngine::mode_e::FORMER,
          &frm1Progress };

      obj out = {};
      bool geOk = ge.run(input,
         out,
         frm1LiteEnabled.isChecked(),
         frm1LiteNotchDet.isChecked(),
         frm1GirdEnabled.isChecked(),
         frm1GirdShowConst.isChecked(),
         anchorAtNotches,
         frm1Hsplit.isChecked(),
         frm1Vsplit.isChecked());

      result.push_back(out);
      if (!geOk)
         break;
   }

   // Create the result drawing
   frm1Result.del();
   for (auto& object : result) {
      frm1Result.copy_from(object);
   }
   frm1State = frm1States::PROCESSED;
   updateFormer1Preview();
   frm1Progress.reset();
}

zoomer::zoomer(QGraphicsView* view)
   : QObject(view),
   view(view) {
   view->viewport()->installEventFilter(this);
   view->setMouseTracking(true);
   modifiers = Qt::ControlModifier;
   zoomFactorBase = 1.0015;
}

void zoomer::gentleZoom(double factor) {
   view->scale(factor, factor);
   view->centerOn(target_scene_pos);
   QPointF delta_viewport_pos = target_viewport_pos - QPointF(view->viewport()->width() / 2.0,
      view->viewport()->height() / 2.0);
   QPointF viewport_center = view->mapFromScene(target_scene_pos) - delta_viewport_pos;
   view->centerOn(view->mapToScene(viewport_center.toPoint()));
   emit zoomed();
}

void zoomer::setModifiers(Qt::KeyboardModifiers value) {
   modifiers = value;
}

void zoomer::setZoomFactorBase(double value) {
   zoomFactorBase = value;
}

bool zoomer::eventFilter(QObject* object, QEvent* event) {
   if (event->type() == QEvent::MouseMove) {
      QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
      QPointF delta = target_viewport_pos - mouse_event->pos();
      if (qAbs(delta.x()) > 5 || qAbs(delta.y()) > 5) {
         target_viewport_pos = mouse_event->pos();
         target_scene_pos = view->mapToScene(mouse_event->pos());
      }
   }
   else if (event->type() == QEvent::Wheel) {
      QWheelEvent* wheel_event = static_cast<QWheelEvent*>(event);
      if (QApplication::keyboardModifiers() == modifiers) {
         double angle = wheel_event->angleDelta().y();
         double factor = qPow(zoomFactorBase, angle);
         gentleZoom(factor);
         return true;
      }
   }
   Q_UNUSED(object)
      return false;
}

bool ruler::eventFilter(QObject* obj, QEvent* event) {
   switch (state) {
   case WAITING: {
      // Wait for a mouse right button click to start a measurement
      if (event->type() == QEvent::GraphicsSceneMousePress) {
         QGraphicsSceneMouseEvent* mp = static_cast<QGraphicsSceneMouseEvent*>(event);
         if (mp->button() == Qt::RightButton) {
            // Start a measurement line
            start = mp->buttonDownScenePos(Qt::RightButton);
            state = MEASURING;

            emit userMessage(SS("Measuring...click right button again to finish"));

            QGraphicsScene* sc = static_cast<QGraphicsScene*>(obj);
            QPen prvpen(Qt::black, 0.25, Qt::DotLine, Qt::RoundCap, Qt::RoundJoin);
            measLine = sc->addLine(QLineF(start, start), prvpen);

            return true;
         }
      }
      break;
   }

   case MEASURING: {
      char str[120];
      if (event->type() == QEvent::GraphicsSceneMouseMove) {
         // If the mouse has moved, update the measurement line and display coords
         QGraphicsSceneMouseEvent* mp = static_cast<QGraphicsSceneMouseEvent*>(event);
         measLine->setLine(QLineF(start, mp->scenePos()));
         snprintf(str, 120, "(%.1lf, %.1lf)", mp->scenePos().x(), -mp->scenePos().y());
         emit userMessage(SS(str));
         return true;
      }
      else if (event->type() == QEvent::GraphicsSceneMousePress) {
         QGraphicsSceneMouseEvent* mp = static_cast<QGraphicsSceneMouseEvent*>(event);
         if (mp->button() == Qt::RightButton) {
            // Remove the measurement line
            QGraphicsScene* sc = static_cast<QGraphicsScene*>(obj);
            sc->removeItem(measLine);

            // Complete the measurement
            char str[120];
            final = mp->buttonDownScenePos(Qt::RightButton);
            line ln(coord_t{ start.x(), -start.y() }, coord_t{ final.x(), -final.y() });
            double dx = final.x() - start.x();
            double dy = -final.y() + start.y();
            snprintf(str, 120, "From(%.1lf,%.1lf) To(%.1lf,%.1lf)  dx %.1lf dy %.1lf  Length %.1lfmm  Angle %.1lfdeg",
               start.x(),
               -start.y(),
               final.x(),
               -final.y(),
               dx,
               dy,
               ln.len(),
               TO_DEGS(ln.angle()));
            emit userMessage(SS(" ") + str);
            state = WAITING;
            return true;
         }
      }
      break;
   }

   default:
      break;
   }

   return false;
}

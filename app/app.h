#pragma once
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

#include <memory>
#include <qmath.h>
#include <unordered_map>
#include <vector>

#include <QApplication>
#include <QDir>
#include <QFrame>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QList>
#include <QMainWindow>
#include <QMouseEvent>
#include <QObject>
#include <QRect>
#include <QRectF>
#include <QScrollBar>
#include <QString>
#include <QStringList>
#include <QTabWidget>
#include <QTableView>
#include <QVBoxLayout>
#include <QtWidgets>

#include "airfoil.h"
#include "former.h"
#include "json.hpp"
#include "json_fwd.hpp"
#include "object_oo.h"
#include "tabs.h"
#include "version.h"
#include "wing.h"
using json = nlohmann::json;

QT_BEGIN_NAMESPACE
class QTabWidget;
class QAction;
class QMenu;
class QPlainTextEdit;
class QSessionManager;
QT_END_NAMESPACE

#define QT_STREAM_VERSION QDataStream::Qt_5_12
#define FILE_SUFFIX "acad"
#define FILE_EXTENSION ".acad"
#define FILE_FILTER "ACAD files (*.acad)"

/**
 * @brief Zoom handler for preview tabs
 */
   class zoomer : public QObject {
   Q_OBJECT

   public:
      explicit zoomer(QGraphicsView* view);
      void gentleZoom(double factor);
      void setModifiers(Qt::KeyboardModifiers modifiers);
      void setZoomFactorBase(double value);

   private:
      QGraphicsView* view;
      Qt::KeyboardModifiers modifiers;
      double zoomFactorBase;
      QPointF target_scene_pos, target_viewport_pos;
      bool eventFilter(QObject* object, QEvent* event);

   signals:
      void zoomed();
};

/**
 * @brief Event filter for preview scene allowing measurements to
 * be made using a right mouse click
 */
class ruler : public QObject {
   Q_OBJECT

signals:
   void userMessage(std::string);

protected:
   bool eventFilter(QObject* obj, QEvent* event) override;
   enum states {
      WAITING,
      MEASURING
   };
   enum states state = WAITING;
   QPointF start, final;
   QGraphicsLineItem* measLine;
};

class App : public QMainWindow {
   Q_OBJECT

public:
   App();

protected:
private slots:
   /**
    * @brief Perform a full build of the model (plan + parts) in detail mode
    */
   void buildHpgl();
   void buildDxf();

   /**
    * @brief Exit the application
    */
   void closeEvent(QCloseEvent* event);

   /**
    * @brief Handle changes to preview draft mode
    */
   void draftEvent(bool state);

   /**
    * @brief Load the configuration json from the application directory
    */
   json loadConfigJson();

   /**
    * @brief Save changes and then clear the model
    */
   void newFile();

   /**
    * @brief Save changes and then open a saved model; see save for format
    */
   void open();

   /**
    * @brief Core functionality of opening a file
    */
   void openCore();

   /**
    * @brief save the contents of the model in Q format
    *
    * The format is:
    *    QString VERSION
    *
    *    QString TAB
    *       >> GenericTab::save()
    */
   void save();

   /**
    * @brief As for save(), but offer new filename selection
    */
   void saveAs();

   /**
    * @brief Show a temporary message on the status bar
    */
   void showStatusBarMsg(std::string msg);

   /**
    * @brief Tab-specific on-selection behaviour
    *
    * Plan and Part preview tabs: Rebuild the relevant part of the model in draft mode
    * and update the graphics scene.
    */
   void tabChanged(int tabIdx);

private:
   /**
    * @brief Check if the default user directory exists, create it if not
    */
   void checkCreateDefaultDirectory();

   /**
    * @brief Delete the model data in every generic tab, delete the graphics in preview tabs
    */
   void clearTabs();

   /**
    * @brief Create buttons and connect them
    */
   void createActions();

   /**
    * @brief Create the set of generic part entry tabs based on the JSON configuration
    */
   void createGenericTabs(const json& cfg);

   /**
    * @brief Create a tab for previewing drawings
    */
   void createPreviewTab(
      QGraphicsView& view,
      QGraphicsScene& scene,
      std::unique_ptr<zoomer>& zm,
      std::unique_ptr<ruler>& rl,
      int* idx,
      const char* title);

   /**
    * @brief Create a tab for handling formers
    */
   void createFormer1Tab();

   /**
    * @brief Create and initialise status bar
    */
   void createStatusBar();

   /**
    * @brief Return the full path, including filename, of the current model save file
    */
   QString currentFileName();

   /**
    * @brief Ask the user for a filename (either for saving to or opening from)
    */
   bool getFileName(bool saveNotOpen);

   /**
    * @brief Check if there are unsaved changes; save them if the user wants them saved
    */
   void needsSaving();

   /**
    * @brief Construct a wing object from the data stored in the model
    */
   void buildWingModel(Wing&, bool inDraftMode = false);

   /**
    * @brief Build the plan view
    */
   obj buildPlan();

   /**
    * @brief Build the set of parts
    */
   obj buildPart(bool isDraft);

   /**
    * @brief Update a preview tab with a drawing object
    */
   void updatePreview(QGraphicsView& view, QGraphicsScene& scene, obj& object);

   /**
    * @brief File import for former tab
    */
   void former1Import();

   /**
    * @brief File export for former tab
    */
   void former1Export();

   /**
    * @brief Update former preview tab
    */
   void updateFormer1Preview();

   /**
    * @brief Apply the former processing
    */
   void former1Execute();

   QTabWidget QTabW; //!< The main functionality of the application window

   QGraphicsView planv = {};        //!< Preview of plan tab view
   QGraphicsScene plans = {};       //!< Graphics scene for plan preview
   int planIdx = -1;                //!< Index of the tab containing the plan preview
   std::unique_ptr<zoomer> plangvz; //!< Zoom engine for the plan
   std::unique_ptr<ruler> planrl;   //!< Measure engine for the plan

   QGraphicsView partv = {};        //!< Preview of part tab view
   QGraphicsScene parts = {};       //!< Graphics scene for parts preview
   int partIdx = -1;                //!< Index of the tab containing the plan preview
   std::unique_ptr<zoomer> partgvz; //!< Zoom engine for the parts
   std::unique_ptr<ruler> partrl;   //!< Measure engine for the parts

   // "Former 1" tab
   enum frm1States {
      EMPTY,
      IMPORTED,
      PROCESSED
   } frm1State = frm1States::EMPTY;

   obj frm1Import = {};
   obj frm1Result = {};
   QGraphicsView frm1v = {};  //!< Preview of former tab view
   QGraphicsScene frm1s = {}; //!< Graphics scene for former preview
   int frm1idx = -1;          //!< Index of the tab containing the plan preview
   zoomer* frm1gvz;           //!< Zoom engine for the parts
   ruler frm1rl{};            //!< Measure engine for the parts
   QProgressBar frm1Progress{};
   QVBoxLayout frm1Layout{};
   QHBoxLayout frm1MenuBar{};

   QFrame frm1ActionFrame{};
   QVBoxLayout frm1ActionButtons{};
   QPushButton frm1ImpFile{ tr("Import File") };
   QPushButton frm1ExpFile{ tr("Export File") };
   QPushButton frm1Process{ tr("Process") };

   QFrame frm1LiteFrame{};
   QVBoxLayout frm1LiteVbox{};
   QHBoxLayout frm1LiteButtons{};
   QHBoxLayout frm1LiteValues{};
   QCheckBox frm1LiteNotchDet{ tr("Notch Detection") };
   QCheckBox frm1LiteEnabled{ tr("Lighten Former") };
   QLabel frm1LHBWLabel{ tr("Lightening Border Width:") };
   QDoubleSpinBox frm1LHBWSpin{};

   QFrame frm1GirdFrame{};
   QVBoxLayout frm1GirdVbox{};
   QHBoxLayout frm1GirdButtons{};
   QHBoxLayout frm1GirdValues{};
   QCheckBox frm1GirdEnabled{ tr("Girder Former") };
   QCheckBox frm1GirdShowConst{ tr("Show Construction") };
   QLabel frm1GirdCompassLabel{ tr("Start At:") };
   QComboBox frm1GirdCompass{};
   QLabel frm1GirdOwLabel{ tr("Outer rim width:") };
   QDoubleSpinBox frm1GirdOwSpin{};
   QLabel frm1GirdIwLabel{ tr("Inner rim width:") };
   QDoubleSpinBox frm1GirdIwSpin{};
   QLabel frm1GirdBwLabel{ tr("Girder bar width:") };
   QDoubleSpinBox frm1GirdBwSpin{};
   QLabel frm1GirdAsLabel{ tr("Anchor spacing:") };
   QDoubleSpinBox frm1GirdAsSpin{};
   QLabel frm1GirdMaLabel{ tr("Min Included Angle:") };
   QDoubleSpinBox frm1GirdMaSpin{};

   QFrame frm1SplitFrame{};
   QVBoxLayout frm1SplitVbox{};
   QHBoxLayout frm1SplitButtons{};
   QHBoxLayout frm1SplitValues{};
   QCheckBox frm1Hsplit{ tr("Horizontal Split") };
   QCheckBox frm1Vsplit{ tr("Vertical Split") };
   QLabel frm1SplitAtYLabel{ tr("Horiz. split at Y=") };
   QDoubleSpinBox frm1SplitAtYSpin{};

   std::unordered_map<std::string, GenericTab*> tabMap; //!< Map tab keys to their tabs
   QString currPath = {};                                //!< Path of the current model save file
   QString currFile = {};                                //!< Name (no path) of the current save file
   std::unique_ptr<QToolBar> fileToolBar;                //!< The main window tool bar
   QCheckBox draftCb{ "Draft Previews", this };            //!< Checkbox for determining if previews are in draft mode
   bool rescalePreviews = true;                          //!< If true, the next preview window update will rescale the views
};

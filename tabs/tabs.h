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

#include <list>

#include <QAction>
#include <QComboBox>
#include <QDataStream>
#include <QDebug>
#include <QFileDialog>
#include <QFrame>
#include <QHash>
#include <QHeaderView>
#include <QItemDelegate>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QPushButton>
#include <QScrollBar>
#include <QSignalMapper>
#include <QStandardItemModel>
#include <QString>
#include <QTabWidget>
#include <QTableView>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariant>

#include "debug.h"
#include "json.hpp"
#include "json_fwd.hpp"
using json = nlohmann::json;

constexpr int buttonSize = 32;
constexpr int resetButtonCol = 0;

/**
 * @brief Roles used to identify types of data stored in QStandardItem
 */
typedef enum {
   whatAmI = Qt::UserRole + 1, //!< Data describes what I am, see AttrQtTypes_e
   keyRole,                    //!< Data is the key string for this item
   defaultRole,                //!< Data is the default value or this item
   choicesRole,                //!< Data is the QStringList of text choices for a combobox item
   airfoilXRole,               //!< Data is a QStringList of double x values for the airfoil
   airfoilYRole,               //!< Data is a QStringList of double y values for the airfoil
   planformXRole,              //!< Data is a QStringList of double x values for the planform
   planformYRole               //!< Data is a QStringList of double y values for the planform
} AttrQtRoles_e;

/**
 * @brief Types of QStandardItem in a generic tab model (whatAmI)
 */
typedef enum {
   normal,       //!< Normal data (int, double or string)
   choices,      //!< A combobox of string choices
   airfoilfile,  //!< A filename of the file from which an airfoil was loaded
   planformfile, //!< A filename of the file from which LE or TE points were loaded
   resetButton,  //!< The button for resetting an entry part to its default values
   addButton,    //!< The button for adding the entry part as a new item in the model data
   deleteButton  //!< The button for deleting an item from the model data
} AttrQtTypes_e;

/**
 * @brief Delegate for handling model entries in a generic tab
 */
class GTabDelegate : public QItemDelegate {
   Q_OBJECT

public:
   explicit GTabDelegate(QObject* parent = 0);

   QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
      const QModelIndex& index) const;

   void setEditorData(QWidget* editor, const QModelIndex& index) const;

   void setModelData(QWidget* editor, QAbstractItemModel* model,
      const QModelIndex& index) const;

   void updateEditorGeometry(QWidget* editor,
      const QStyleOptionViewItem& option,
      const QModelIndex& index) const;
};

/**
 * @brief Generic data entry tab
 *
 * Each tab consists of a vertical box containing two table views, one for data entry
 * and one for displaying entered parts
 */
class GenericTab : public QObject {
   Q_OBJECT

public:
   static bool modelChangedSinceSave;
   static bool modelChangedSincePrvw;

   GenericTab(QTabWidget* qtbw, json& cfg);

   /**
    * @brief Create an entry part from its JSON configuration
    */
   void AddEntry(json& js);

   /**
    * @brief Clear the data model
    */
   void ClearData();

   /**
    * @brief Access a value at a given row and key
    */
   QVariant Get(int row, const char* key);
   QVariant Get(int row, const char* key, int role);
   double gdbl(int row, const char* key);
   int gint(int row, const char* key);
   QString gqst(int row, const char* key);

   /**
    * @brief Get my key
    */
   std::string GetKey();

   /**
    * @brief Get the number of model parts
    */
   int GetNumParts();

   /*
    * Access functions for dataChanged
    */
   static void setModelChangedSave(bool v);
   static void setModelChangedPrvw(bool v);
   static bool getModelChangedSave();
   static bool getModelChangedPrvw();

   /**
    * @brief Save the tab in Q format
    *
    * File format is as follows
    *    QString key
    *    int number of rows
    *    int number of columns
    *    QStandardItem ordered across then down
    */
   void save(QDataStream& ds) const;

   /**
    * @brief Load the tab in Q format
    */
   void load(QDataStream& ds);

private slots:
   /**
    * @brief Reset an entry part to its default values
    */
   void ResetEntry();

   /**
    * @brief Move an entry part to the data model
    */
   void MoveEntryToModel();

   /**
    * @brief Delete a data model part
    */
   void DeleteModelPart();

   /**
    * @brief Model data has been changed by the user
    */
   void ModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

private:
   std::unique_ptr<QFrame> frme;      //!< The tab's frame
   std::unique_ptr<QVBoxLayout> vbox; //!< The tab's main vertical layout box

   std::unique_ptr<GTabDelegate> entdel;     //!< Handler for this tab's models
   std::unique_ptr<QStandardItemModel> entm; //!< Model for the data entry
   std::unique_ptr<QTableView> entv;         //!< Table view for data entry

   std::unique_ptr<GTabDelegate> datdel;     //!< Handler for this tab's data
   std::unique_ptr<QStandardItemModel> datm; //!< Model for already entered data
   std::unique_ptr<QTableView> datv;         //!< Table view for already entered data

   QStringList sortList = {}; //!< List of keys to sort data by

   QList<QString> headings = {}; //<! Headings for entry and data model views

   QString key = {}; //<! Indexing key for the tab

   int idxCol = -1; //<! Column index of part indexes (-1 otherwise)
   int lnkCol = -1; //<! Column index of part links   (-1 otherwise)

   bool sortEnabled = true; //<! True if automatic sorting of data model is enabled

   //! @brief Used in the sorting of linked items
   struct linkmap {
      int idx = -1;
      int lnk = -1;
      int newidx = -1;
      int newlnk = -1;
   };

   /**
    * @brief Return true if this item has something linked to it
    */
   bool isParent(int row);

   /**
    * @brief Disable automatic sorting of data model entries
    */
   void DisableSort() {
      sortEnabled = false;
   };

   /**
    * @brief Enable automatic sorting of data model entries
    */
   void EnableSort() {
      sortEnabled = true;
   };

   /**
    * @brief Sort data model by the column keys in sortList
    *
    * Sorting is done in two stages:
    *    The data is sorted column by column using the list of sort columns defined in the config json;
    *    If the model has an index & link column, then unlinked rows are re-indexed in order and linked rows
    *    are inserted directly after their parent.
    */
   void SortData();

   /**
    * @brief Update indexes and rearrange based on row links
    *
    * If the model has an index & link column, then unlinked rows are re-indexed in order and linked rows
    *    are inserted directly after their parent.  Any orphaned linked row is deleted.
    */
   bool SortLinkedRows();

   /**
    * @brief Self-recursive link updating
    *
    * @param ll            vector of linkmaps, one per row of the model
    * @param item          we are looking for entries in ll that are linked to item
    * @param uniqueindex   the current uniqueindex i.e., the one assigned to item. It will be incremented for each linked item found
    */
   void UpdateLinks(std::vector<linkmap>& ll, std::vector<linkmap>::iterator& item, int* uniqueindex);
};

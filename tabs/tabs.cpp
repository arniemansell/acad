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

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

#include <QTabWidget>
#include <QTableView>

#include "tabs.h"

GenericTab::GenericTab(QTabWidget* qtbw, json& cfg) {
   frme = std::unique_ptr<QFrame>(new QFrame(qtbw));

   vbox = std::unique_ptr<QVBoxLayout>(new QVBoxLayout(frme.get()));

   entv = std::unique_ptr<QTableView>(new QTableView());
   entm = std::unique_ptr<QStandardItemModel>(new QStandardItemModel());
   entdel = std::unique_ptr<GTabDelegate>(new GTabDelegate());
   entv->setModel(entm.get());
   entv->setItemDelegate(entdel.get());
   entv->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
   vbox->addWidget(entv.get());

   datv = std::unique_ptr<QTableView>(new QTableView());
   datm = std::unique_ptr<QStandardItemModel>(new QStandardItemModel());
   datdel = std::unique_ptr<GTabDelegate>(new GTabDelegate());
   datv->setModel(datm.get());
   datv->setItemDelegate(datdel.get());
   datv->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
   connect(datm.get(), SIGNAL(dataChanged(QModelIndex, QModelIndex)), this,
      SLOT(ModelDataChanged(QModelIndex, QModelIndex)));
   vbox->addWidget(datv.get());

   // Allow the model part of the layout to occupy the majority of the vbox and frame
   vbox->setStretchFactor(entv.get(), 1);
   vbox->setStretchFactor(datv.get(), 200);

   qtbw->addTab(frme.get(), QString::fromStdString(cfg.at("title")));

   // List of keys to sort by
   sortEnabled = true;
   if (cfg.count("sort_by")) {
      json sb = cfg.at("sort_by");
      if (sb.is_array())
         for (json::iterator sbi = sb.begin(); sbi != sb.end(); ++sbi)
            sortList.append(QString::fromStdString(sbi->get<std::string>()));
   }

   // Indexing key
   key = QString::fromStdString(cfg.at("key"));
};

void GenericTab::AddEntry(json& js) {
   QList<QStandardItem*> fields;
   headings.clear();

   // Reset Button
   headings.append(QString(""));
   fields.append(new QStandardItem);
   fields.back()->setData(QVariant((int)resetButton), whatAmI);
   fields.back()->setData(QVariant(QString("LEFTBUTTON")), keyRole);

   QToolButton* reset = new QToolButton();
   reset->setIcon(QIcon(":/images/refresh.png"));
   reset->setIconSize(QSize(buttonSize, buttonSize));
   reset->setToolTip(QString("Click to reset to default values"));
   connect(reset, SIGNAL(clicked()), this, SLOT(ResetEntry()));

   // Title and Meta Data
   headings.append(QString("Part Type"));
   QStandardItem* meta = new QStandardItem;
   fields.append(meta);

   meta->setData(QVariant(QString::fromStdString(js.at("title"))), Qt::DisplayRole);
   meta->setData(QVariant(QString::fromStdString(js.at("title"))), (int)defaultRole);
   meta->setToolTip(QString::fromStdString(js.at("help")));
   meta->setData(QVariant((int)normal), whatAmI);
   meta->setData(QVariant(QString::fromStdString(js.at("key"))), keyRole);
   meta->setEditable(false);
   meta->setSelectable(false);

   // Normal attributes
   json ats = js.at("attributes");
   json dflt;
   std::string key;

   for (json::iterator at = ats.begin(); at != ats.end(); ++at) {
      QStandardItem* data = new QStandardItem;

      headings.append(QString::fromStdString(at->at("title").get<std::string>()));
      fields.append(data);

      dflt = at->at("default");
      key = at->at("key").get<std::string>();
      data->setData(QVariant(QString::fromStdString(key)), keyRole);
      data->setData(QVariant(Qt::AlignCenter), Qt::TextAlignmentRole);
      data->setToolTip(QString::fromStdString(at->at("help")));

      if (at->count("inactive")) {
         data->setBackground(QBrush(Qt::gray, Qt::DiagCrossPattern));
         data->setForeground(QBrush(Qt::transparent, Qt::DiagCrossPattern));
         data->setEditable(false);
         data->setSelectable(false);
         data->setToolTip(QString(""));
      }

      if (key == "IDX")
         idxCol = fields.count() - 1;

      if (key == "LINK")
         lnkCol = fields.count() - 1;

      if (key == "AIRFOIL") {
         // This is an airfoil section
         data->setData(QVariant((int)airfoilfile), whatAmI);
         data->setData(QVariant(QString::fromStdString(dflt.get<std::string>())), defaultRole);
         data->setData(QVariant(QString::fromStdString(dflt.get<std::string>())), Qt::DisplayRole);
         data->setData(QVariant(QStringList()), airfoilXRole);
         data->setData(QVariant(QStringList()), airfoilYRole);
      }

      else if (key == "XYFILE") {
         // This is a file containing x,y coordinates
         data->setData(QVariant((int)planformfile), whatAmI);
         data->setData(QVariant(QString::fromStdString(dflt.get<std::string>())), defaultRole);
         data->setData(QVariant(QString::fromStdString(dflt.get<std::string>())), Qt::DisplayRole);
         data->setData(QVariant(QStringList()), planformXRole);
         data->setData(QVariant(QStringList()), planformYRole);
      }

      else if (dflt.is_array()) {
         // This is a choices string parameter
         data->setData(QVariant((int)choices), whatAmI);

         // Read the choices into the combobox
         QStringList choiceList;
         for (json::iterator ch = dflt.begin(); ch != dflt.end(); ++ch)
            choiceList.append(QString::fromStdString(ch->get<std::string>()));

         // Store the choices and set the current and default values
         data->setData(QVariant(choiceList), (int)choicesRole);
         data->setData(choiceList.at(0), (int)Qt::DisplayRole);
         data->setData(choiceList.at(0), (int)defaultRole);
      }

      else if (dflt.is_string()) {
         data->setData(QVariant((int)normal), whatAmI);
         data->setData(QVariant(QString::fromStdString(dflt.get<std::string>())), (int)defaultRole);
         data->setData(QVariant(QString::fromStdString(dflt.get<std::string>())), Qt::DisplayRole);
      }

      else if (dflt.is_number_float()) {
         data->setData(QVariant((int)normal), whatAmI);
         data->setData(QVariant(dflt.get<double>()), (int)defaultRole);
         data->setData(QVariant(dflt.get<double>()), Qt::DisplayRole);
      }

      else if (dflt.is_number_integer()) {
         data->setData(QVariant((int)normal), whatAmI);
         data->setData(QVariant(dflt.get<int>()), (int)defaultRole);
         data->setData(QVariant(dflt.get<int>()), Qt::DisplayRole);
      }
      else
         dbg::fatal(
            std::string(
               "Unsupported type of dflt (not AIRFOIL, choices array, string, float, integer ")
            + __FILE__);
   }

   // Add Button
   headings.append(QString(""));
   fields.append(new QStandardItem);
   fields.back()->setData(QVariant((int)addButton), whatAmI);
   fields.back()->setData(QVariant(QString("RIGHTBUTTON")), keyRole);

   QToolButton* add = new QToolButton();
   add->setIcon(QIcon(":/images/add.png"));
   add->setIconSize(QSize(buttonSize, buttonSize));
   add->setToolTip(QString("Click to add to model as a new part"));
   connect(add, SIGNAL(clicked()), this, SLOT(MoveEntryToModel()));

   // Create model row
   entm->appendRow(fields);
   entm->setHorizontalHeaderLabels(headings);
   datm->setHorizontalHeaderLabels(headings);
   int myRow = entm->rowCount() - 1;

   // Install buttons
   entv->setIndexWidget(entm->index(myRow, resetButtonCol), reset);
   entv->setColumnWidth(resetButtonCol, buttonSize);

   entv->setIndexWidget(entm->index(myRow, (entm->columnCount() - 1)), add);
   entv->setColumnWidth((entm->columnCount() - 1), buttonSize);

   // Set the height
   int count = entv->verticalHeader()->count();
   int scrollBarHeight = entv->horizontalScrollBar()->height();
   int horizontalHeaderHeight = entv->horizontalHeader()->height();
   int rowTotalHeight = 0;
   for (int i = 0; i < count; ++i)
      rowTotalHeight += entv->verticalHeader()->sectionSize(i);
   entv->setMinimumHeight(horizontalHeaderHeight + rowTotalHeight + scrollBarHeight);

   DBGLVL1("Created Entry Part: %s", js.at("title").get<std::string>().c_str());
};

void GenericTab::ClearData() {
   int rows = datm->rowCount();
   for (int r = (rows - 1); r >= 0; r--) {
      datm->removeRow(r);
   }
   DBGLVL1("Tab %s data cleared, #rows: %d", key.toStdString().c_str(), rows);
}

QVariant GenericTab::Get(int row, const char* key, int role) {
   for (int c = 0; c < datm->columnCount(); c++)
      if (datm->data(datm->index(row, c), keyRole) == QString(key))
         return datm->data(datm->index(row, c), role);

   dbg::fatal(SS("Unable to find value with key ") + key + " " + __FILE__);
   return QVariant();
}

QVariant GenericTab::Get(int row, const char* key) {
   return Get(row, key, Qt::DisplayRole);
}

double GenericTab::gdbl(int row, const char* key) {
   return Get(row, key).toDouble();
}

int GenericTab::gint(int row, const char* key) {
   return Get(row, key).toInt();
}

QString GenericTab::gqst(int row, const char* key) {
   return Get(row, key).toString();
}

std::string GenericTab::GetKey() {
   return key.toStdString();
}

int GenericTab::GetNumParts() {
   return datm->rowCount();
}

bool GenericTab::modelChangedSinceSave = false;
bool GenericTab::modelChangedSincePrvw = false;
void GenericTab::setModelChangedSave(bool v) {
   modelChangedSinceSave = v;
};
void GenericTab::setModelChangedPrvw(bool v) {
   modelChangedSincePrvw = v;
};
bool GenericTab::getModelChangedSave() {
   return modelChangedSinceSave;
};
bool GenericTab::getModelChangedPrvw() {
   return modelChangedSincePrvw;
};

void GenericTab::save(QDataStream& ds) const {
   ds << key;

   int rows = datm->rowCount();
   ds << rows;

   int cols = datm->columnCount();
   ds << cols;

   for (int r = 0; r < rows; r++)
      for (int c = 0; c < cols; c++) {
         ds << *(datm->item(r, c));
      }

   DBGLVL1("Tab %s wrote #rows %d #columns %d", key.toStdString().c_str(), rows, cols);
}

void GenericTab::load(QDataStream& ds) {
   DisableSort();
   int exCols, exRows;
   ds >> exRows;
   ds >> exCols;

   // Read in the complete model table
   QStandardItem* items[200][50];
   for (int exR = 0; exR < exRows; exR++)
      for (int exC = 0; exC < exCols; exC++) {
         items[exR][exC] = new QStandardItem;
         ds >> *items[exR][exC];
      }
   DBGLVL1("Tab %s read in #rows %d #columns %d", key.toStdString().c_str(), exRows, exCols);

   // Work through the table, looking up each required internal key in the external data
   for (int exR = 0; exR < exRows; exR++)
      for (int inC = 0; inC < entm->columnCount(); inC++) {
         bool isFound = false;

         // Get WhatAmI from the entry part
         AttrQtTypes_e inWai = (AttrQtTypes_e)entm->item(0, inC)->data(whatAmI).toInt();

         // Get the right and whatAmI (the buttons cause a few issues)
         QString intKey;
         switch (inWai) {
         case normal:
         case choices:     // Fallthrough
         case airfoilfile: // Fallthrough
         case planformfile: // Fallthrough
            intKey = entm->data(entm->index(0, inC), keyRole).toString();
            break;

         case resetButton:
            intKey = QString("LEFTBUTTON");
            break;

         case addButton:
            intKey = QString("RIGHTBUTTON");
            inWai = deleteButton;
            break;

         default:
            dbg::fatal(SS("Unhandled WhatAmI value ") + TS(inWai) + " " + __FILE__);
            break;
         }

         // Find the column in the loaded data
         for (int exC = 0; exC < exCols; exC++) {
            int exWai = items[exR][exC]->data(whatAmI).toInt();
            if (exWai == normal || exWai == choices || exWai == airfoilfile || exWai == planformfile) {
               if (items[exR][exC]->data(keyRole).toString() == intKey) {
                  datm->setItem(exR, inC, items[exR][exC]);
                  isFound = true;
               }
            }
         }

         // If it wasn't found, set it to default
         if (!isFound) {
            datm->setItem(exR, inC, entm->item(0, inC)->clone());
            if ((inWai != deleteButton) && (inWai != resetButton))
               DBGLVL1("Saved data Row %d is missing key %s; using default values from entry part 0", exR,
                  intKey.toStdString().c_str());
         }

         if (inWai == deleteButton) {
            // Add button in entry part needs converting to delete button in data part
            QToolButton* del = new QToolButton();
            del->setIcon(QIcon(":/images/del.png"));
            del->setIconSize(QSize(buttonSize, buttonSize));
            del->setToolTip(QString("Click to delete from the model"));
            connect(del, SIGNAL(clicked()), this, SLOT(DeleteModelPart()));
            datv->setIndexWidget(datm->index(exR, inC), del);
            datv->setColumnWidth(inC, buttonSize);
            datv->setColumnWidth(resetButtonCol, buttonSize);
         }
      }
   EnableSort();
   SortData();
}

void GenericTab::ResetEntry() {
   int row = (entv->indexAt(dynamic_cast<QWidget*>(sender())->pos())).row();
   int columns = entm->columnCount();
   for (int col = 0; col < columns; col++) {
      QStandardItem* item = entm->item(row, col);
      if (item) {
         switch (item->data(whatAmI).toInt()) {
         case normal: // Fallthrough
         case choices:
         case airfoilfile:
         case planformfile:
            item->setData(item->data((int)defaultRole), Qt::DisplayRole);
            break;

         case resetButton: // Fallthrough
         case addButton:   // Fallthrough
         case deleteButton:
            break;

         default:
            dbg::fatal(std::string("Unhandled whatAmI when resetting entry ") + __FILE__);
            break;
         }
      }
      else
         dbg::fatal(SS("NULL item pointer in ") + __FILE__);
   }

   DBGLVL1("Tab %s reset entry row %d", key.toStdString().c_str(), row);
}

void GenericTab::MoveEntryToModel() {
   // Get the index of the entry part Add Button
   QModelIndex butIdx = (entv->indexAt(dynamic_cast<QWidget*>(sender())->pos()));

   // Take a copy of the row
   QList<QStandardItem*> ql;
   for (int i = 0; i < entm->columnCount(); i++) {
      QStandardItem* qsi = entm->item(butIdx.row(), i)->clone();
      ql.append(qsi);
   }

   if (idxCol >= 0) {
      // There is an index column.  If the value is 0, then it is to be assigned a unique value now.
      QStandardItem* idx = ql.at(idxCol);

      if (idx->data(Qt::DisplayRole).toInt() == 0) {
         int uniqueIdx = 1;
         int r = 0;
         while (r < datm->rowCount()) {
            if (datm->item(r++, idxCol)->data(Qt::DisplayRole).toInt() == uniqueIdx) {
               r = 0;
               if (++uniqueIdx == INT_MAX)
                  dbg::fatal(SS("Something has gone wrong with finding a unique index for the part"));
            }
         }
         idx->setData(QVariant(uniqueIdx), Qt::DisplayRole);

         // Once in the data model, we need to be able to see the index
         idx->setBackground(QBrush(Qt::white, Qt::NoBrush));
         idx->setForeground(QBrush(Qt::gray, Qt::NoBrush));
      }

      if (lnkCol >= 0) {
         // There is a link column.  If the value is > -1 then we need to link.
         QStandardItem* lnk = ql.at(lnkCol);
         int linkval = lnk->data(Qt::DisplayRole).toInt();
         if (linkval >= 0) {
            for (int r = 0; true;) {
               if (datm->item(r, idxCol)->data(Qt::DisplayRole).toInt() == linkval)
                  break;

               if (++r == datm->rowCount()) {
                  dbg::alert(SS("There is no item with index ") + TS(linkval) + " to link to");
                  return;
               }
            }
         }
      }
   }

   datm->appendRow(ql);

   // Set the delete button and the column width for column 0
   QToolButton* del = new QToolButton();
   del->setIcon(QIcon(":/images/del.png"));
   del->setIconSize(QSize(buttonSize, buttonSize));
   del->setToolTip(QString("Click to delete from the model"));
   connect(del, SIGNAL(clicked()), this, SLOT(DeleteModelPart()));
   datv->setIndexWidget(datm->index(datm->rowCount() - 1, datm->columnCount() - 1), del);
   datv->setColumnWidth(datm->columnCount() - 1, buttonSize);
   datv->setColumnWidth(resetButtonCol, buttonSize);
   datm->setData(datm->index(datm->rowCount() - 1, datm->columnCount() - 1), QVariant((int)deleteButton), whatAmI);

   // Sort the data
   SortData();
   setModelChangedSave(true);
   setModelChangedPrvw(true);

   DBGLVL1("Tab %s moved entry part %d  to model which now has #rows %d", key.toStdString().c_str(), butIdx.row(),
      datm->rowCount());
}

void GenericTab::DeleteModelPart() {
   QModelIndex butIdx = (datv->indexAt(dynamic_cast<QWidget*>(sender())->pos()));

   if (isParent(butIdx.row())) {
      QMessageBox msgBox;
      msgBox.setText(
         "The part is linked with other part(s). If you continue, all of the linked parts will be deleted too.");
      msgBox.setInformativeText("Delete the Parts?");
      msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
      msgBox.setDefaultButton(QMessageBox::No);
      msgBox.setIcon(QMessageBox::Warning);
      msgBox.setWindowTitle(QString("Delete linked parts?"));
      if (msgBox.exec() == QMessageBox::No)
         return;
   }

   datm->removeRow(butIdx.row());
   SortData();
   setModelChangedSave(true);
   setModelChangedPrvw(true);

   DBGLVL1("Tab %s deleted model part from row %d", key.toStdString().c_str(), butIdx.row());
}

void GenericTab::ModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight) {
   (void)topLeft;
   (void)bottomRight;

   SortData();
   setModelChangedSave(true);
   setModelChangedPrvw(true);
}

bool GenericTab::isParent(int row) {
   if ((idxCol == -1) || (lnkCol == -1))
      return false; // No linked column or index column

   QStandardItem* itIdx = datm->item(row, idxCol);
   if (!itIdx)
      return false; // Model incomplete

   int index = itIdx->data(Qt::DisplayRole).toInt();

   for (int r = 0; r < datm->rowCount(); r++) {
      QStandardItem* itLnk = datm->item(r, lnkCol);
      if (!itIdx || !itLnk)
         return false; // Model is not complete
      if (itLnk->data(Qt::DisplayRole) == index)
         return true; // Found a linked item
   }

   return false;
}

void GenericTab::SortData() {
   if (sortEnabled) {
      // Sort by each column in the SortBy list
      QStringList::const_iterator it;
      for (it = sortList.constBegin(); it != sortList.constEnd(); ++it) {
         // Find the relevant column for the key and sort by it
         for (int col = 0; col < datm->columnCount(); col++) {
            if (datm->item(0, col))
               if (datm->item(0, col)->data((int)keyRole) == *it)
                  datm->sort(col);
         }
      }

      // If there are indexes/links, sort by them
      SortLinkedRows();

      // Model has changed
      setModelChangedSave(true);
      setModelChangedPrvw(true);
   }
};

bool GenericTab::SortLinkedRows() {
   if ((idxCol == -1) || (lnkCol == -1))
      return false; // No linked column or index column

   std::vector<linkmap> ll; // Store on link map per model row

   // Read in the current set of indexes and links into the linkmap for each model row
   for (int r = 0; r < datm->rowCount(); r++) {
      struct linkmap l;
      QStandardItem* itIdx = datm->item(r, idxCol);
      QStandardItem* itLnk = datm->item(r, lnkCol);
      if (!itIdx || !itLnk)
         return false; // Model is not complete

      l.idx = itIdx->data(Qt::DisplayRole).toInt();
      l.lnk = itLnk->data(Qt::DisplayRole).toInt();
      ll.emplace_back(l);
   }

   // Update the indexes and links
   //   Unlinked rows are given a unique index in their existing order
   //   Linked rows are inserted directly after their parent
   int uidx = 0; // Unique index
   for (std::vector<linkmap>::iterator it1 = ll.begin(); it1 != ll.end(); ++it1) {
      if (it1->lnk == -1) {
         // This is not a linked item, give it a unique index
         it1->newidx = ++uidx;

         // Now find anything linked to it and reorder (recursively)
         UpdateLinks(ll, it1, &uidx);
      }
   }

   DBGLVL2("Link sort results for tab %s ", this->key.toStdString().c_str());
   for (std::vector<linkmap>::iterator it1 = ll.begin(); it1 != ll.end(); ++it1)
      DBGLVL2("Dump SLR:  Index: %d:  LinkTo: %d  NewIdx: %d  NewLnk: %d",
         it1->idx,
         it1->lnk,
         it1->newidx,
         it1->newlnk);

   // Update the data model with the new values
   DisableSort();
   for (int r = 0; r < datm->rowCount(); r++) {
      QStandardItem* itIdx = datm->item(r, idxCol);
      QStandardItem* itLnk = datm->item(r, lnkCol);
      itIdx->setData(QVariant(ll.at(r).newidx), Qt::DisplayRole);
      itLnk->setData(QVariant(ll.at(r).newlnk), Qt::DisplayRole);
   }

   // Delete the rows which have been orphaned
   for (int r = 0; r < datm->rowCount();) {
      if (datm->item(r, idxCol)->data(Qt::DisplayRole).toInt() == -1) {
         datm->removeRow(r);
      }
      else
         ++r;
   }

   // Resort based on index
   datm->sort(idxCol);
   EnableSort();

   return false;
}

void GenericTab::UpdateLinks(std::vector<linkmap>& ll, std::vector<linkmap>::iterator& item, int* uniqueindex) {
   // Search through the list and update anything that is linked to item (recursively)
   for (std::vector<linkmap>::iterator it = ll.begin(); it != ll.end(); ++it) {
      if (it->lnk == item->idx) {
         it->newlnk = item->newidx;
         it->newidx = ++(*uniqueindex);
         UpdateLinks(ll, it, uniqueindex); // Recursive update
      }
   }
}

GTabDelegate::GTabDelegate(QObject* parent)
   : QItemDelegate(parent) {
};

QWidget* GTabDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option,
   const QModelIndex& index) const {
   const QAbstractItemModel* mdl = index.model();

   QWidget* editor;
   switch (mdl->data(index, whatAmI).toInt()) {
   case choices:
      // Editor needs to be a combo box
      editor = new QComboBox(parent);
      editor->setAutoFillBackground(true);
      editor->setStyleSheet("background-color: white; min-width: 5em;");

      return (editor);

   case airfoilfile: {
      QFileDialog* airfOpen = new QFileDialog(parent,
         tr("Open File"),
         QDir::homePath().append("/Documents/acad"),
         tr("Selig or Lednicer files (*.dat)"));
      return airfOpen;
   } break;

   case planformfile: {
      QFileDialog* planOpen = new QFileDialog(parent,
         tr("Open File"),
         QDir::homePath().append("/Documents/acad"),
         tr("X/Y coordinate text file (*.*)"));
      return planOpen;
   } break;

   default:
      break;
   }

   editor = QItemDelegate::createEditor(parent, option, index);
   editor->setAutoFillBackground(true);
   editor->setStyleSheet("background-color: white; min-width: 5em;");

   return (editor);
}

void GTabDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
   const QAbstractItemModel* mdl = index.model();

   switch (mdl->data(index, whatAmI).toInt()) {
   case choices: {
      // Update the combo box contents
      QComboBox* cbb = static_cast<QComboBox*>(editor);

      // Retrieve the list of choices
      cbb->addItems(mdl->data(index, choicesRole).toStringList());

      // Update the current index to match the display data
      int displayInd = cbb->findText(mdl->data(index, Qt::DisplayRole).toString());
      if (displayInd >= 0)
         cbb->setCurrentIndex(displayInd);

      return;
   }

   case airfoilfile: {
      QFileDialog* fd = static_cast<QFileDialog*>(editor);
      fd->setFileMode(QFileDialog::ExistingFile);
      return;
   } break;

   case planformfile: {
      QFileDialog* fd = static_cast<QFileDialog*>(editor);
      fd->setFileMode(QFileDialog::ExistingFile);
      return;
   } break;

   default:
      break;
   }

   return QItemDelegate::setEditorData(editor, index);
}

void GTabDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
   const QModelIndex& index) const {
   switch (model->data(index, whatAmI).toInt()) {
   case choices: {
      QComboBox* cbb = static_cast<QComboBox*>(editor);
      QVariant txt = QVariant(cbb->currentText());
      model->setData(index, txt, Qt::EditRole);

      return;
   }

   case airfoilfile: {
      // Retrieve the filename and add it to the model
      QFileDialog* fd = static_cast<QFileDialog*>(editor);
      if (fd->selectedFiles().size()) {
         const QFileInfo fi(fd->selectedFiles().at(0));

         // Open the file for reading
         std::ifstream fs;
         fs.open(fi.absoluteFilePath().toStdString(), std::ifstream::in);
         if (!fs.is_open()) {
            dbg::alert("Unable to open file for parsing: " + fi.fileName().toStdString());
            return;
         }
         DBGLVL1("Opened airfoil file: %s", fi.fileName().toStdString().c_str());

         // Prepare the item for updating
         QModelIndex invertInd = model->index(index.row(), index.column() + 1);
         bool invert = model->data(invertInd, Qt::DisplayRole).toString() == QString("Inverted") ? true : false;
         model->data(index, airfoilXRole).clear();
         model->data(index, airfoilYRole).clear();
         QStringList xVals, yVals;

         // Do a line by line parse of the airfoil file
         std::string line;
         bool doneLednicer = false;
         bool doneAirfoilName = false;
         while (getline(fs, line)) {
            double x, y;
            if (sscanf(line.c_str(), "%lf %lf", &x, &y) == 2) {
               if ((x > 1.01) && (y > 1.01) && !doneLednicer) {
                  // Both values > 1 suggest a Lednicer format file
                  DBGLVL1("Lednicer point counts: top %lf bottom %lf", x, y);
                  doneLednicer = true;
               }

               if ((x > 1.01) || (y > 1.01) || (x < -1.01) || (y < -1.01)) {
                  dbg::alert(SS("Airfoil import: Unparsable values in line ") + line);
                  return;
               }

               // Point looks OK
               x = 1.0 - x;
               xVals.append(QString("%1").arg(x, 0, 'f', 5));
               if (invert)
                  yVals.append(QString("%1").arg(-y, 0, 'f', 5));
               else
                  yVals.append(QString("%1").arg(y, 0, 'f', 5));

               DBGLVL2("Point: %lf %lf", x, y);
            }
            else {
               if (!doneAirfoilName) {
                  DBGLVL1("Airfoil name from .dat file: %s", line.c_str());
                  doneAirfoilName = true;
               }
               else {
                  dbg::alert(SS("Unknown line in .dat file: ") + line);
               }
            }
         }

         fs.close();

         // Store data into the model
         model->setData(index, QVariant(xVals), airfoilXRole);
         model->setData(index, QVariant(yVals), airfoilYRole);
         model->setData(index, QVariant(fi.fileName()), Qt::EditRole);
      }
      return;
   }

   case planformfile: {
      // Retrieve the filename and add it to the model
      QFileDialog* fd = static_cast<QFileDialog*>(editor);
      if (fd->selectedFiles().size()) {
         const QFileInfo fi(fd->selectedFiles().at(0));

         // Open the file for reading
         std::ifstream fs;
         fs.open(fi.absoluteFilePath().toStdString(), std::ifstream::in);
         if (!fs.is_open()) {
            dbg::alert("Unable to open file for parsing: " + fi.fileName().toStdString());
            return;
         }
         DBGLVL1("Opened planform file: %s", fi.fileName().toStdString().c_str());

         model->data(index, planformXRole).clear();
         model->data(index, planformYRole).clear();
         QStringList xVals, yVals;

         // Do a line by line parse of the planform file
         std::string line;
         while (getline(fs, line)) {
            double x, y;
            if (sscanf(line.c_str(), "%lf %lf", &x, &y) != 2)
               if (sscanf(line.c_str(), "%lf , %lf", &x, &y) != 2)
                  continue;
            DBGLVL1("Read planform point: %lf %lf", x, y);
            xVals.append(QString("%1").arg(x, 0, 'f', 5));
            yVals.append(QString("%1").arg(y, 0, 'f', 5));
         }

         fs.close();

         // Store data into the model
         model->setData(index, QVariant(xVals), planformXRole);
         model->setData(index, QVariant(yVals), planformYRole);
         model->setData(index, QVariant(fi.fileName()), Qt::EditRole);
      }
      return;
   }

   default:
      break;
   }

   return QItemDelegate::setModelData(editor, model, index);
}

void GTabDelegate::updateEditorGeometry(QWidget* editor,
   const QStyleOptionViewItem& option,
   const QModelIndex& index) const {
   const QAbstractItemModel* mdl = index.model();
   if (mdl->data(index, whatAmI).toInt() == airfoilfile
      || mdl->data(index, whatAmI).toInt() == planformfile)
      // Don't update the file open dialog
      return;

   return QItemDelegate::updateEditorGeometry(editor, option, index);
}

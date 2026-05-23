#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "graph.h"
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QMainWindow>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow() = default;

private slots:
  void onRunClicked();

private:
  void setupUI();
  void visualizeGraph(int n, const std::vector<Edge> &edges,
                      const SimulationResult &result);
  void fillMatrix(int n, const std::vector<Edge> &edges,
                  const SimulationResult &result);
  QColor getStateColor(int state_idx, int total_states);

  QTextEdit *inputArea;
  QPushButton *runButton;
  QTextEdit *outputArea;
  QTabWidget *tabWidget;

  QGraphicsView *graphView;
  QGraphicsScene *graphScene;
  QTableWidget *matrixTable;
};

#endif // MAINWINDOW_H

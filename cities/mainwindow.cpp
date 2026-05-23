#include "mainwindow.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>
#include <cmath>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setupUI();
  resize(1100, 750);
  setWindowTitle("Распределение городов между государствами");
}

void MainWindow::setupUI() {
  // Применяем светлый стиль, похожий на современные UI из отчетов
  this->setStyleSheet(R"(
    QMainWindow {
        background-color: #f5f6f8;
    }
    QWidget {
        color: #333333;
        font-family: "Segoe UI", Arial, sans-serif;
        font-size: 13px;
    }
    QLabel {
        font-weight: bold;
        color: #444444;
        margin-bottom: 4px;
    }
    QTextEdit {
        background-color: #ffffff;
        border: 1px solid #cfd4db;
        border-radius: 6px;
        padding: 8px;
        color: #24292e;
        selection-background-color: #add6ff;
    }
    QPushButton {
        background-color: #0088ff; /* Синяя кнопка как на скриншотах */
        border: none;
        border-radius: 5px;
        padding: 10px 16px;
        color: #ffffff;
        font-weight: bold;
    }
    QPushButton:hover {
        background-color: #0077e6;
    }
    QPushButton:pressed {
        background-color: #0066cc;
    }
    QTabWidget::pane {
        border: 1px solid #cfd4db;
        background-color: #ffffff;
        border-radius: 6px;
    }
    QTabBar::tab {
        background-color: #e1e4e8;
        border: 1px solid #cfd4db;
        padding: 8px 16px;
        border-top-left-radius: 5px;
        border-top-right-radius: 5px;
        color: #586069;
        margin-right: 2px;
    }
    QTabBar::tab:selected {
        background-color: #ffffff;
        color: #24292e;
        border-bottom-color: #ffffff;
        font-weight: bold;
    }
    QTableWidget {
        background-color: #ffffff;
        gridline-color: #e1e4e8;
        color: #24292e;
        border: none;
    }
    QHeaderView::section {
        background-color: #f6f8fa;
        color: #24292e;
        padding: 6px;
        border: 1px solid #e1e4e8;
        font-weight: bold;
    }
  )");

  QWidget *centralWidget = new QWidget(this);
  setCentralWidget(centralWidget);

  QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
  mainLayout->setContentsMargins(12, 12, 12, 12);

  QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
  mainLayout->addWidget(splitter);

  // Левая панель управления
  QWidget *leftPanel = new QWidget(this);
  QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
  leftLayout->setContentsMargins(0, 0, 8, 0);

  leftLayout->addWidget(new QLabel("Входные данные:\n(N M, затем M строк 'i j "
                                   "len',\nзатем K и K номеров столиц)",
                                   this));

  inputArea = new QTextEdit(this);
  inputArea->setPlainText("6 7\n"
                          "1 2 5.0\n"
                          "1 3 3.0\n"
                          "2 4 1.0\n"
                          "3 4 2.0\n"
                          "3 5 6.0\n"
                          "4 6 4.0\n"
                          "5 6 1.0\n"
                          "2\n"
                          "1 6");
  leftLayout->addWidget(inputArea);

  runButton =
      new QPushButton("Построить маршрут", this); // Название как на скрине
  leftLayout->addWidget(runButton);

  outputArea = new QTextEdit(this);
  outputArea->setReadOnly(true);
  leftLayout->addWidget(new QLabel("Результаты распределения:", this));
  leftLayout->addWidget(outputArea);

  splitter->addWidget(leftPanel);

  // Правая панель визуализации
  tabWidget = new QTabWidget(this);

  // Вкладка Графа
  graphScene = new QGraphicsScene(this);
  graphView = new QGraphicsView(graphScene, this);
  graphView->setRenderHint(QPainter::Antialiasing);
  graphView->setStyleSheet(
      "background-color: #ffffff; border: none;"); // Белый фон для графа
  tabWidget->addTab(graphView, "Интерактивное полотно");

  // Вкладка Матрицы
  matrixTable = new QTableWidget(this);
  matrixTable->setAlternatingRowColors(false);
  tabWidget->addTab(matrixTable, "Матрица смежности");

  splitter->addWidget(tabWidget);
  splitter->setStretchFactor(0, 3);
  splitter->setStretchFactor(1, 7);

  connect(runButton, &QPushButton::clicked, this, &MainWindow::onRunClicked);
}

void MainWindow::onRunClicked() {
  QString inputText = inputArea->toPlainText().trimmed();
  if (inputText.isEmpty())
    return;

  QTextStream stream(&inputText);
  int n, m;
  stream >> n >> m;
  if (stream.status() != QTextStream::Ok)
    return;

  std::vector<Edge> edges;
  for (int i = 0; i < m; ++i) {
    int u, v;
    double weight;
    stream >> u >> v >> weight;
    edges.push_back({u - 1, v - 1, weight});
  }

  int k;
  stream >> k;
  std::vector<int> capitals(k);
  for (int i = 0; i < k; ++i) {
    stream >> capitals[i];
    capitals[i]--;
  }

  SimulationResult result = partition_cities(n, edges, capitals);

  QString log;
  for (int s = 0; s < k; ++s) {
    log += QString("Государство %1 (Столица: %2):\nСписок городов: ")
               .arg(s + 1)
               .arg(capitals[s] + 1);
    for (size_t i = 0; i < result.states[s].size(); ++i) {
      log += QString::number(result.states[s][i] + 1);
      if (i + 1 < result.states[s].size())
        log += ", ";
    }
    log += "\n\n";
  }
  outputArea->setPlainText(log);

  visualizeGraph(n, edges, result);
  fillMatrix(n, edges, result);
}

QColor MainWindow::getStateColor(int state_idx, int total_states) {
  if (state_idx < 0)
    return QColor("#e0e0e0"); // Светло-серый цвет для нераспределенных городов

  // Генерируем пастельные приятные цвета (как на скриншотах с почтальоном)
  double hue = (state_idx * 360.0) / total_states;
  return QColor::fromHsvF(hue / 360.0, 0.45, 0.95);
}

void MainWindow::visualizeGraph(int n, const std::vector<Edge> &edges,
                                const SimulationResult &result) {
  graphScene->clear();
  if (n <= 0)
    return;

  double radius = 180.0;
  double centerX = 250.0;
  double centerY = 250.0;
  std::vector<QPointF> positions(n);

  for (int i = 0; i < n; ++i) {
    double angle = 2.0 * M_PI * i / n;
    positions[i] =
        QPointF(centerX + radius * cos(angle), centerY + radius * sin(angle));
  }

  // Отрисовка ребер
  for (const auto &edge : edges) {
    if (edge.u < n && edge.v < n) {
      QPen pen(QColor("#b0bec5"), 2); // Светло-серые линии ребер
      graphScene->addLine(positions[edge.u].x(), positions[edge.u].y(),
                          positions[edge.v].x(), positions[edge.v].y(), pen);

      QPointF mid = (positions[edge.u] + positions[edge.v]) / 2.0;

      // Текст веса ребра
      QGraphicsSimpleTextItem *text =
          graphScene->addSimpleText(QString::number(edge.weight));
      text->setBrush(QBrush(
          QColor("#d32f2f"))); // Красный/темный цвет для веса, чтобы выделялся
      QFont font = text->font();
      font.setPointSize(9);
      font.setBold(true);
      text->setFont(font);

      double textWidth = text->boundingRect().width();
      double textHeight = text->boundingRect().height();

      // Белая подложка для текста, чтобы не перекрывался линиями
      QGraphicsRectItem *bgRect = graphScene->addRect(
          mid.x() - textWidth / 2.0 - 2, mid.y() - textHeight / 2.0 - 2,
          textWidth + 4, textHeight + 4, QPen(Qt::NoPen),
          QBrush(QColor("#ffffff")));

      bgRect->setZValue(1);
      text->setZValue(2);
      text->setPos(mid.x() - textWidth / 2.0, mid.y() - textHeight / 2.0);
    }
  }

  double nodeRadius = 22.0;
  int k = result.states.size();

  // Отрисовка узлов
  for (int i = 0; i < n; ++i) {
    int state = result.city_to_state[i];
    QColor color = getStateColor(state, k);

    bool isCapital = false;
    if (state >= 0 && !result.states[state].empty() &&
        result.states[state][0] == i) {
      isCapital = true;
    }

    QPen pen;
    if (isCapital) {
      // Столицы выделяются более жирной обводкой основного цвета (или темной)
      QColor borderColor = color.darker(150);
      pen = QPen(borderColor, 3.0);
    } else {
      // Обычные города - тонкая обводка
      QColor borderColor = color.darker(110);
      pen = QPen(borderColor, 1.0);
    }

    QGraphicsEllipseItem *node = graphScene->addEllipse(
        positions[i].x() - nodeRadius, positions[i].y() - nodeRadius,
        nodeRadius * 2, nodeRadius * 2, pen, QBrush(color));
    node->setZValue(3);

    // Номер города внутри узла
    QGraphicsSimpleTextItem *text =
        graphScene->addSimpleText(QString::number(i + 1));
    text->setBrush(
        QBrush(QColor("#24292e"))); // Темный текст внутри светлых кругов
    QFont font = text->font();
    font.setBold(true);
    font.setPointSize(10);
    text->setFont(font);

    double tx = positions[i].x() - text->boundingRect().width() / 2.0;
    double ty = positions[i].y() - text->boundingRect().height() / 2.0;
    text->setPos(tx, ty);
    text->setZValue(4);
  }
}

void MainWindow::fillMatrix(int n, const std::vector<Edge> &edges,
                            const SimulationResult &result) {
  matrixTable->clear();
  matrixTable->setRowCount(n);
  matrixTable->setColumnCount(n);

  QStringList headers;
  for (int i = 0; i < n; ++i) {
    headers << QString("Г %1").arg(i + 1);
  }
  matrixTable->setHorizontalHeaderLabels(headers);
  matrixTable->setVerticalHeaderLabels(headers);

  // Инициализация пустых ячеек
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < n; ++j) {
      auto *item = new QTableWidgetItem(i == j ? "0" : "-");
      item->setTextAlignment(Qt::AlignCenter);
      matrixTable->setItem(i, j, item);
    }
  }

  // Заполнение весов
  for (const auto &edge : edges) {
    if (edge.u < n && edge.v < n) {
      matrixTable->item(edge.u, edge.v)->setText(QString::number(edge.weight));
      matrixTable->item(edge.v, edge.u)->setText(QString::number(edge.weight));
    }
  }

  // Раскраска матрицы по принадлежности к государствам
  int k = result.states.size();
  for (int i = 0; i < n; ++i) {
    int state_i = result.city_to_state[i];
    QColor color_i = getStateColor(state_i, k);

    // Красим заголовки
    if (auto *vHeader = matrixTable->verticalHeaderItem(i)) {
      vHeader->setBackground(QBrush(color_i));
      vHeader->setForeground(
          QBrush(QColor("#24292e"))); // Черный текст на светлом фоне
    }
    if (auto *hHeader = matrixTable->horizontalHeaderItem(i)) {
      hHeader->setBackground(QBrush(color_i));
      hHeader->setForeground(QBrush(QColor("#24292e")));
    }

    // Красим ячейки пересечений внутри одного государства
    for (int j = 0; j < n; ++j) {
      int state_j = result.city_to_state[j];
      if (state_i == state_j && state_i != -1) {
        QColor translucentColor = color_i;
        translucentColor.setAlpha(100); // Легкая заливка
        matrixTable->item(i, j)->setBackground(QBrush(translucentColor));
      }
    }
  }

  matrixTable->resizeColumnsToContents();
}

#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  MainWindow w;
  w.show();
  return app.exec();
}

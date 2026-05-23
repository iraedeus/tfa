#include <QApplication>
#include <QComboBox>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMap>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QSplitter>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <cmath>
#include <queue>
#include <vector>

using namespace std;

const double PI = std::acos(-1.0);

// Структура ребра
struct Edge {
  int u, v, w;
};

// --- АЛГОРИТМ (Жадная эвристика) ---
class ChinesePostman {
  int V;
  vector<Edge> edges;

public:
  ChinesePostman(int v, const vector<Edge> &e) : V(v), edges(e) {}

  pair<int, vector<int>> solve(int start_node) {
    if (edges.empty())
      return {0, {}};

    int total_weight = 0;
    vector<vector<int>> dist(V, vector<int>(V, 1e8));
    vector<vector<int>> next_node(V, vector<int>(V, -1));
    vector<int> degree(V, 0);

    for (int i = 0; i < V; ++i)
      dist[i][i] = 0;

    for (const auto &e : edges) {
      total_weight += e.w;
      degree[e.u]++;
      degree[e.v]++;
      if (e.w < dist[e.u][e.v]) {
        dist[e.u][e.v] = e.w;
        dist[e.v][e.u] = e.w;
        next_node[e.u][e.v] = e.v;
        next_node[e.v][e.u] = e.u;
      }
    }

    vector<bool> visited(V, false);
    queue<int> q;
    q.push(start_node);
    visited[start_node] = true;

    while (!q.empty()) {
      int curr = q.front();
      q.pop();
      for (int i = 0; i < V; ++i) {
        if (dist[curr][i] < 1e8 && dist[curr][i] > 0 && !visited[i]) {
          visited[i] = true;
          q.push(i);
        }
      }
    }

    for (int i = 0; i < V; ++i) {
      if (degree[i] > 0 && !visited[i])
        return {-1, {}};
    }

    for (int k = 0; k < V; ++k)
      for (int i = 0; i < V; ++i)
        for (int j = 0; j < V; ++j)
          if (dist[i][k] < 1e8 && dist[k][j] < 1e8 &&
              dist[i][k] + dist[k][j] < dist[i][j]) {
            dist[i][j] = dist[i][k] + dist[k][j];
            next_node[i][j] = next_node[i][k];
          }

    vector<int> odds;
    for (int i = 0; i < V; ++i)
      if (degree[i] % 2 != 0)
        odds.push_back(i);

    if (!odds.empty()) {
      vector<bool> used_odd(odds.size(), false);
      for (size_t step = 0; step < odds.size() / 2; ++step) {
        int best_i = -1, best_j = -1;
        int min_w = 1e8;

        for (size_t i = 0; i < odds.size(); ++i) {
          if (used_odd[i])
            continue;
          for (size_t j = i + 1; j < odds.size(); ++j) {
            if (used_odd[j])
              continue;
            if (dist[odds[i]][odds[j]] < min_w) {
              min_w = dist[odds[i]][odds[j]];
              best_i = i;
              best_j = j;
            }
          }
        }

        if (best_i != -1 && best_j != -1) {
          used_odd[best_i] = true;
          used_odd[best_j] = true;
          total_weight += min_w;

          int curr = odds[best_i];
          int target = odds[best_j];
          while (curr != target) {
            int nxt = next_node[curr][target];
            edges.push_back({curr, nxt, dist[curr][nxt]});
            degree[curr]++;
            degree[nxt]++;
            curr = nxt;
          }
        }
      }
    }

    struct AdjEdge {
      int to, id;
    };
    vector<vector<AdjEdge>> adj(V);
    int edge_id = 0;
    for (const auto &e : edges) {
      adj[e.u].push_back({e.v, edge_id});
      adj[e.v].push_back({e.u, edge_id});
      edge_id++;
    }

    vector<bool> used_edge(edge_id, false);
    vector<int> circuit;

    auto dfs = [&](auto &self, int u) -> void {
      while (!adj[u].empty()) {
        auto edge = adj[u].back();
        adj[u].pop_back();
        if (!used_edge[edge.id]) {
          used_edge[edge.id] = true;
          self(self, edge.to);
        }
      }
      circuit.push_back(u);
    };

    dfs(dfs, start_node);
    reverse(circuit.begin(), circuit.end());

    return {total_weight, circuit};
  }
};

// --- ИНТЕРАКТИВНОЕ ПОЛОТНО (Отрисовка в светлой теме) ---
class GraphWidget : public QGraphicsView {
  QGraphicsScene *scene;

public:
  GraphWidget(QWidget *parent = nullptr) : QGraphicsView(parent) {
    scene = new QGraphicsScene(this);
    setScene(scene);
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::TextAntialiasing);
    setBackgroundBrush(QBrush(QColor("#ffffff"))); // Белый фон
    setStyleSheet("border: none;");
  }

  void drawGraph(const QMap<QString, int> &name2id,
                 const QMap<int, QString> &id2name, const vector<Edge> &edges,
                 int start_node_id) {
    scene->clear();
    int V = name2id.size();
    if (V <= 0)
      return;

    int width = 600, height = 500;
    scene->setSceneRect(0, 0, width, height);

    int cx = width / 2, cy = height / 2;
    int R = 150;
    int r = 22; // Радиус узла

    vector<QPointF> pos(V);
    for (int i = 0; i < V; ++i) {
      double angle = -PI / 2 + 2.0 * PI * i / V;
      pos[i] = QPointF(cx + R * cos(angle), cy + R * sin(angle));
    }

    // Отрисовка ребер
    QPen edgePen(QColor("#b2bec3"), 2); // Светло-серая линия
    for (const auto &e : edges) {
      scene->addLine(pos[e.u].x(), pos[e.u].y(), pos[e.v].x(), pos[e.v].y(),
                     edgePen);

      double midX = (pos[e.u].x() + pos[e.v].x()) / 2.0;
      double midY = (pos[e.u].y() + pos[e.v].y()) / 2.0;

      // Белая плашка под текст, чтобы линия его не зачеркивала
      QGraphicsRectItem *bgRect =
          scene->addRect(midX - 12, midY - 10, 24, 20, Qt::NoPen,
                         QBrush(QColor(255, 255, 255, 220)));
      bgRect->setZValue(1);

      QGraphicsTextItem *weightText = scene->addText(QString::number(e.w));
      weightText->setDefaultTextColor(
          QColor("#e17055")); // Мягкий красный для веса
      QFont f = weightText->font();
      f.setPointSize(10);
      f.setBold(true);
      weightText->setFont(f);
      weightText->setPos(midX - weightText->boundingRect().width() / 2,
                         midY - weightText->boundingRect().height() / 2);
      weightText->setZValue(2);
    }

    // Отрисовка вершин
    for (int i = 0; i < V; ++i) {
      // Стартовая: желто-оранжевая. Обычные: голубые
      QColor bgColor =
          (i == start_node_id) ? QColor("#ffeaa7") : QColor("#74b9ff");
      QColor borderColor =
          (i == start_node_id) ? QColor("#fdcb6e") : QColor("#0984e3");

      QPen nodePen(borderColor, 2);
      QGraphicsEllipseItem *node =
          scene->addEllipse(pos[i].x() - r, pos[i].y() - r, 2 * r, 2 * r,
                            nodePen, QBrush(bgColor));
      node->setZValue(3);

      QGraphicsTextItem *nodeText = scene->addText(id2name[i]);
      nodeText->setDefaultTextColor(QColor("#2d3436")); // Темно-серый текст
      QFont f = nodeText->font();
      f.setBold(true);
      f.setPointSize(11);
      nodeText->setFont(f);
      nodeText->setPos(pos[i].x() - nodeText->boundingRect().width() / 2,
                       pos[i].y() - nodeText->boundingRect().height() / 2);
      nodeText->setZValue(4);
    }
  }
};

// --- ГЛАВНОЕ ОКНО (UI) ---
class MainWindow : public QWidget {
  QTextEdit *inputArea;
  QComboBox *startNodeCombo;
  QPushButton *solveBtn;
  QLabel *costLabel;
  QLabel *pathLabel;
  GraphWidget *graphWidget;

  QMap<QString, int> name2id;
  QMap<int, QString> id2name;
  vector<Edge> parsedEdges;

public:
  MainWindow() {
    setWindowTitle("Задача о китайском почтальоне");
    resize(1100, 700);

    // --- Светлая, чистая тема (macOS / Material style) ---

    setStyleSheet(R"(
        QWidget {
            background-color: #f5f6fa;
            font-family: 'Segoe UI', Arial, sans-serif;
            color: #2f3640;
        }
        /* Убираем рамки у всех меток, чтобы не было кругов вокруг заголовков */
        QLabel { 
            font-size: 12px; 
            font-weight: bold; 
            color: #7f8fa6; 
            text-transform: uppercase; 
            border: none; 
            background: transparent;
        }
        QTextEdit {
            background-color: #ffffff;
            border: 1px solid #dcdde1;
            border-radius: 6px;
            padding: 10px;
            color: #2f3640;
        }
        QComboBox {
            background-color: #ffffff;
            border: 1px solid #dcdde1;
            border-radius: 6px;
            padding: 5px;
            color: #2f3640;
        }
        /* Стили для кнопки */
        QPushButton#mainBtn {
            background-color: #0097e6;
            color: #ffffff; /* Явно белый цвет текста */
            border: none;
            border-radius: 6px;
            padding: 12px;
            font-size: 14px;
            font-weight: bold;
            min-height: 20px;
        }
        QPushButton#mainBtn:hover { 
            background-color: #00a8ff; 
        }
    )");

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    mainLayout->addWidget(splitter);

    // === ЛЕВАЯ ПАНЕЛЬ ===
    QWidget *leftWidget = new QWidget();
    leftWidget->setStyleSheet("background-color: #ffffff; border: 1px solid "
                              "#dcdde1; border-radius: 8px;");
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(20, 20, 20, 20);
    leftLayout->setSpacing(15);

    // Блок 1: Данные графа
    leftLayout->addWidget(new QLabel("Данные графа", this));
    QLabel *hintLabel = new QLabel("Формат: ИмяУзла: Цель(Вес)", this);
    hintLabel->setStyleSheet(
        "font-size: 11px; color: #7f8fa6; text-transform: none; border: none;");
    leftLayout->addWidget(hintLabel);

    inputArea = new QTextEdit(this);
    inputArea->setText("A: B(10) C(12)\nB: A(10) C(15)\nC: A(12) B(15)");
    inputArea->setStyleSheet("border: 1px solid #dcdde1;");
    leftLayout->addWidget(inputArea, 2);

    connect(inputArea, &QTextEdit::textChanged, this,
            &MainWindow::parseGraphData);

    // Блок 2: Расчет
    QLabel *calcLabel = new QLabel("Расчёт", this);
    calcLabel->setStyleSheet("border: none;");
    leftLayout->addWidget(calcLabel);

    QLabel *startHint = new QLabel("Стартовая вершина", this);
    startHint->setStyleSheet("font-size: 11px; color: #7f8fa6; border: none;");
    leftLayout->addWidget(startHint);

    startNodeCombo = new QComboBox(this);
    leftLayout->addWidget(startNodeCombo);

    solveBtn = new QPushButton("Построить маршрут", this);
    solveBtn->setObjectName("mainBtn");
    solveBtn->setStyleSheet(
        "color: white; font-weight: bold; background-color: #0097e6;");
    leftLayout->addWidget(solveBtn);

    // Блок 3: Итог
    QLabel *resLabel = new QLabel("Итог", this);
    resLabel->setStyleSheet("border: none;");
    leftLayout->addWidget(resLabel);

    QHBoxLayout *costLayout = new QHBoxLayout();
    QLabel *costTitle = new QLabel("Стоимость", this);
    costTitle->setStyleSheet("font-size: 12px; border: none;");
    costLabel = new QLabel("0", this);
    costLabel->setStyleSheet(
        "font-size: 20px; color: #44bd32; border: none;"); // Зеленый для
                                                           // успешного
                                                           // результата
    costLayout->addWidget(costTitle);
    costLayout->addStretch();
    costLayout->addWidget(costLabel);
    leftLayout->addLayout(costLayout);

    QLabel *pathTitle = new QLabel("Цепочка вершин", this);
    pathTitle->setStyleSheet(
        "font-size: 11px; text-transform: none; border: none;");
    leftLayout->addWidget(pathTitle);

    pathLabel = new QLabel("", this);
    pathLabel->setStyleSheet("font-size: 14px; font-weight: normal; color: "
                             "#2f3640; text-transform: none; border: none;");
    pathLabel->setWordWrap(true);
    leftLayout->addWidget(pathLabel, 1);

    splitter->addWidget(leftWidget);

    // === ПРАВАЯ ПАНЕЛЬ (ГРАФ) ===
    QWidget *rightWidget = new QWidget();
    rightWidget->setStyleSheet("background-color: #ffffff; border: 1px solid "
                               "#dcdde1; border-radius: 8px;");
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);

    QLabel *canvasLabel = new QLabel("Интерактивное полотно", this);
    canvasLabel->setStyleSheet("border: none;");
    rightLayout->addWidget(canvasLabel);

    graphWidget = new GraphWidget(this);
    rightLayout->addWidget(graphWidget);

    splitter->addWidget(rightWidget);
    splitter->setSizes({350, 700}); // Пропорции панелей 1 к 2

    connect(solveBtn, &QPushButton::clicked, this, &MainWindow::onSolveClicked);

    // Первичный парсинг при запуске
    parseGraphData();
  }

private:
  void parseGraphData() {
    QString text = inputArea->toPlainText();
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);

    name2id.clear();
    id2name.clear();
    parsedEdges.clear();
    int current_id = 0;

    auto getId = [&](const QString &name) {
      if (!name2id.contains(name)) {
        name2id[name] = current_id;
        id2name[current_id] = name;
        current_id++;
      }
      return name2id[name];
    };

    QRegularExpression re("(\\w+)\\s*\\(\\s*(\\d+)\\s*\\)");
    QSet<QString> edgesSet;

    for (const QString &line : lines) {
      QStringList parts = line.split(":", Qt::SkipEmptyParts);
      if (parts.size() < 2)
        continue;

      QString u_name = parts[0].trimmed();
      int u = getId(u_name);

      QRegularExpressionMatchIterator i = re.globalMatch(parts[1]);
      while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString v_name = match.captured(1);
        int w = match.captured(2).toInt();
        int v = getId(v_name);

        QString edgeKey = u < v ? QString("%1-%2").arg(u).arg(v)
                                : QString("%1-%2").arg(v).arg(u);
        if (!edgesSet.contains(edgeKey)) {
          edgesSet.insert(edgeKey);
          parsedEdges.push_back({u, v, w});
        }
      }
    }

    QString currentSelection = startNodeCombo->currentText();
    startNodeCombo->clear();
    for (int i = 0; i < current_id; ++i) {
      startNodeCombo->addItem(id2name[i]);
    }

    int idx = startNodeCombo->findText(currentSelection);
    if (idx != -1)
      startNodeCombo->setCurrentIndex(idx);
  }

  void onSolveClicked() {
    parseGraphData();

    if (name2id.isEmpty() || parsedEdges.empty()) {
      QMessageBox::warning(this, "Ошибка",
                           "Граф пуст или введен в неверном формате!");
      return;
    }

    QString startName = startNodeCombo->currentText();
    if (!name2id.contains(startName))
      return;
    int start_node = name2id[startName];

    graphWidget->drawGraph(name2id, id2name, parsedEdges, start_node);

    ChinesePostman cp(name2id.size(), parsedEdges);
    auto result = cp.solve(start_node);

    if (result.first == -1) {
      costLabel->setText("ERR");
      costLabel->setStyleSheet(
          "font-size: 20px; color: #e84118; border: none;");
      pathLabel->setText(
          "<span style='color: #e84118;'>Ошибка: Граф несвязный!</span>");
      return;
    }

    costLabel->setText(QString::number(result.first));
    costLabel->setStyleSheet(
        "font-size: 20px; color: #44bd32; border: none;"); // Зеленый при успехе

    // Красивое форматирование маршрута для светлой темы
    QString pathHtml = "";
    for (size_t i = 0; i < result.second.size(); ++i) {
      pathHtml +=
          "<span style='background-color: #f1f2f6; border: 1px solid #dcdde1; "
          "border-radius: 4px; padding: 4px 8px; color: #2f3640;'>";
      pathHtml += id2name[result.second[i]];
      pathHtml += "</span>";
      if (i != result.second.size() - 1) {
        pathHtml +=
            " &nbsp;<span style='color: #7f8fa6;'>&#8594;</span>&nbsp; ";
      }
    }
    pathLabel->setText(pathHtml);
  }
};

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  MainWindow window;
  window.show();
  return app.exec();
}

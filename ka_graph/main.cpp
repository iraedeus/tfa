#include <QApplication>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QPainter>
#include <QScrollArea>
#include <QMessageBox>
#include <QSplitter>
#include <QGroupBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QStringList>
#include <QTabWidget>
#include <QSlider>

#include <vector>
#include <queue>
#include <algorithm>
#include <set>
#include <map>
#include <cmath>

using namespace std;

// Структура для хранения временного интервала связи
struct Link {
    int u, v;
    int t_start;
    int t_end;
};

// Функция для проверки пересечения двух интервалов [a, b] и [c, d]
bool isIntersecting(int a, int b, int c, int d) {
    return max(a, c) < min(b, d); 
}

// =========================================================================
// Вкладка 1: Диаграмма временных интервалов (Гантта)
// =========================================================================
class GanttWidget : public QWidget {
public:
    GanttWidget(QWidget* parent = nullptr) : QWidget(parent), maxTime(60), currentTact(0) { 
        setMinimumSize(600, 400); 
    }

    void setData(const vector<Link>& lns, int limitT, int tact) {
        links = lns;
        maxTime = limitT;
        currentTact = tact;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.fillRect(rect(), QColor("#FFFFFF"));
        painter.setRenderHint(QPainter::Antialiasing);

        if (links.empty()) return;

        int marginX = 120;
        int marginY = 60;
        int rowHeight = 40;
        
        double scaleX = (width() - marginX - 50) / (double)maxTime;

        painter.setPen(QColor("#2C3E50"));
        painter.setFont(QFont("Segoe UI", 11, QFont::Bold));
        painter.drawText(marginX, 30, "Временные интервалы активности каналов связи КА");

        int axisBottom = marginY + links.size() * rowHeight + 10;
        
        // Сетка и шкала времени
        painter.setFont(QFont("Segoe UI", 9));
        int step = max(1, maxTime / 10);
        for (int t = 0; t <= maxTime; t += step) {
            int x = marginX + t * scaleX;
            painter.setPen(QPen(QColor("#E0E0E0"), 1, Qt::DashLine));
            painter.drawLine(x, marginY, x, axisBottom);
            painter.setPen(Qt::black);
            painter.drawLine(x, axisBottom, x, axisBottom + 5);
            painter.drawText(x - 10, axisBottom + 20, QString::number(t));
        }
        
        // Оси
        painter.setPen(QPen(Qt::black, 2));
        painter.drawLine(marginX, marginY, marginX, axisBottom);
        painter.drawLine(marginX, axisBottom, width() - 20, axisBottom);
        painter.drawText(width() - 15, axisBottom + 20, "t, мин");

        // Отрисовка интервалов связей
        for (size_t i = 0; i < links.size(); ++i) {
            int y = marginY + i * rowHeight;
            painter.setPen(Qt::black);
            painter.setFont(QFont("Segoe UI", 10));
            painter.drawText(20, y + 20, QString("КА %1 ↔ %2").arg(links[i].u).arg(links[i].v));

            int tL = links[i].t_start;
            int tR = links[i].t_end;

            // Проверяем активность связи на текущем такте
            int tactStart = currentTact * 10;
            int tactEnd = (currentTact + 1) * 10;
            bool activeNow = isIntersecting(tL, tR, tactStart, tactEnd);

            QColor barColor = activeNow ? QColor("#2ECC71") : QColor("#BDC3C7");

            QRect barRect(marginX + tL * scaleX, y + 10, (tR - tL) * scaleX, 15);
            painter.setPen(Qt::NoPen);
            painter.setBrush(barColor);
            painter.drawRoundedRect(barRect, 3, 3);

            painter.setPen(QColor("#7F8C8D"));
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(barRect, 3, 3);
        }

        // Вертикальный маркер текущего такта (интервала)
        int tactXStart = marginX + (currentTact * 10) * scaleX;
        int tactXEnd = marginX + ((currentTact + 1) * 10) * scaleX;
        
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(231, 76, 60, 40)); // Полупрозрачный красный интервал такта
        painter.drawRect(QRect(tactXStart, marginY, tactXEnd - tactXStart, axisBottom - marginY));

        painter.setPen(QPen(QColor("#E74C3C"), 2, Qt::SolidLine));
        painter.drawLine(tactXStart, marginY, tactXStart, axisBottom);
        painter.drawLine(tactXEnd, marginY, tactXEnd, axisBottom);

        setMinimumSize(600, axisBottom + 50);
    }
private:
    vector<Link> links;
    int maxTime;
    int currentTact;
};

// =========================================================================
// Вкладка 2: Визуализация топологии Графа на текущем такте
// =========================================================================
class GraphWidget : public QWidget {
public:
    GraphWidget(QWidget* parent = nullptr) : QWidget(parent) {}

    void setData(int totalNodes, const vector<Link>& activeLns, const vector<int>& comp, const vector<QColor>& cols) {
        nodeCount = totalNodes;
        activeLinks = activeLns;
        components = comp;
        colors = cols;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.fillRect(rect(), QColor("#FFFFFF"));
        painter.setRenderHint(QPainter::Antialiasing);

        if (nodeCount <= 0) return;

        int cx = width() / 2;
        int cy = height() / 2;
        int radius = std::min(width(), height()) / 2 - 50;
        int nodeRadius = 18;

        // Координаты узлов на окружности
        std::map<int, QPoint> positions;
        for (int i = 1; i <= nodeCount; ++i) {
            double angle = 2.0 * M_PI * (i - 1) / nodeCount;
            int x = cx + radius * std::cos(angle);
            int y = cy + radius * std::sin(angle);
            positions[i] = QPoint(x, y);
        }

        // Рисуем активные связи
        painter.setPen(QPen(QColor("#2980B9"), 3));
        for (const auto& link : activeLinks) {
            if (positions.count(link.u) && positions.count(link.v)) {
                painter.drawLine(positions[link.u], positions[link.v]);
            }
        }

        // Рисуем узлы
        for (int i = 1; i <= nodeCount; ++i) {
            QPoint p = positions[i];
            int compId = components[i];
            
            QColor nodeColor = (compId != -1) ? colors[compId % colors.size()] : QColor("#BDC3C7");

            painter.setBrush(nodeColor);
            painter.setPen(QPen(Qt::black, 1.5));
            painter.drawEllipse(p, nodeRadius, nodeRadius);

            painter.setPen(Qt::white);
            painter.setFont(QFont("Segoe UI", 10, QFont::Bold));
            QRect textRect(p.x() - nodeRadius, p.y() - nodeRadius, nodeRadius * 2, nodeRadius * 2);
            painter.drawText(textRect, Qt::AlignCenter, QString::number(i));
        }
    }

private:
    int nodeCount = 0;
    vector<Link> activeLinks;
    vector<int> components;
    vector<QColor> colors;
};

// =========================================================================
// Главное окно
// =========================================================================
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Стилизация для нейтрального отображения элементов интерфейса без наложений
    app.setStyleSheet(R"(
        QWidget { font-family: 'Segoe UI', Arial, sans-serif; font-size: 13px; }
        QLabel { color: #2C3E50; }
        
        /* Исправление QGroupBox: добавляем margin и padding, чтобы заголовки не наползали на контент */
        QGroupBox { 
            font-weight: bold; 
            border: 1px solid #BDC3C7; 
            border-radius: 6px; 
            margin-top: 16px; 
            padding-top: 10px;
            background-color: #FAFAFA; 
        }
        QGroupBox::title { 
            subcontrol-origin: margin; 
            subcontrol-position: top left; 
            left: 10px; 
            padding: 0 5px; 
            color: #2980B9; 
        }
        
        QPushButton { background-color: #2980B9; color: white; border: none; border-radius: 5px; padding: 10px; font-weight: bold; }
        QPushButton:hover { background-color: #3498DB; }
        
        QTextEdit, QPlainTextEdit, QLineEdit, QSpinBox { 
            background-color: #FFFFFF; 
            color: #000000; 
            border: 1px solid #BDC3C7; 
            border-radius: 4px; 
            padding: 5px; 
        }
        QTabWidget::pane { border: 1px solid #BDC3C7; background: #FFFFFF; }
        QTabBar::tab { background: #ECF0F1; color: #2C3E50; padding: 8px 15px; border: 1px solid #BDC3C7; border-bottom: none; border-top-left-radius: 4px; border-top-right-radius: 4px; }
        QTabBar::tab:selected { background: #FFFFFF; font-weight: bold; }
    )");

    QWidget window;
    window.setWindowTitle("Анализ структурной динамики группировок КА");
    window.resize(1350, 850);

    QHBoxLayout* mainLayout = new QHBoxLayout(&window);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    mainLayout->addWidget(splitter);

    // ================= ЛЕВАЯ ПАНЕЛЬ С НАСТРОЙКАМИ =================
    QWidget* leftPanel = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(5, 5, 5, 5);
    leftLayout->setSpacing(15); // Увеличиваем зазоры между группами

    // --- ГРУППА 1 ---
    QGroupBox* groupParams = new QGroupBox("1. Параметры времени и пространства");
    QFormLayout* formLayout = new QFormLayout(groupParams);
    // Добавляем верхний отступ (24px), чтобы заголовок группы гарантированно не перекрывал поля ввода
    formLayout->setContentsMargins(12, 24, 12, 12);
    formLayout->setSpacing(10);
    
    QSpinBox* spinN = new QSpinBox(); spinN->setRange(1, 100); spinN->setValue(8);
    QSpinBox* spinT = new QSpinBox(); spinT->setRange(10, 1440); spinT->setValue(60);
    formLayout->addRow("Количество КА (n):", spinN);
    formLayout->addRow("Время работы T (мин):", spinT);
    leftLayout->addWidget(groupParams);

    // --- ГРУППА 2 ---
    QGroupBox* groupLinks = new QGroupBox("2. Каналы связи (КА_1 КА_2 t_нач t_кон)");
    QVBoxLayout* linksLayout = new QVBoxLayout(groupLinks);
    linksLayout->setContentsMargins(12, 24, 12, 12);
    
    QPlainTextEdit* textLinks = new QPlainTextEdit();
    textLinks->setPlainText("1 2 0 15\n1 3 10 30\n2 4 5 25\n3 4 20 50\n4 5 15 45\n5 6 35 60\n6 7 40 60\n5 7 10 35\n6 8 0 25\n7 8 20 55");
    linksLayout->addWidget(textLinks);
    leftLayout->addWidget(groupLinks, 3); // Даем больше веса для растяжения по высоте

    // --- ГРУППА 3 ---
    QGroupBox* groupControl = new QGroupBox("3. Управление временной шкалой");
    QVBoxLayout* ctrlLayout = new QVBoxLayout(groupControl);
    ctrlLayout->setContentsMargins(12, 24, 12, 12);
    ctrlLayout->setSpacing(10);
    
    QSlider* timeSlider = new QSlider(Qt::Horizontal);
    timeSlider->setRange(0, 5);
    QLabel* lblTact = new QLabel("Выбранный такт: [0 - 10] мин");
    lblTact->setAlignment(Qt::AlignCenter);
    lblTact->setStyleSheet("font-weight: bold; color: #2980B9;");
    
    ctrlLayout->addWidget(timeSlider);
    ctrlLayout->addWidget(lblTact);
    leftLayout->addWidget(groupControl);

    // Кнопка перерасчета
    QPushButton* btnRecalc = new QPushButton("Рассчитать структурную динамику");
    leftLayout->addWidget(btnRecalc);

    // --- ГРУППА 4 ---
    QGroupBox* groupLog = new QGroupBox("4. Состояние группировок на такте");
    QVBoxLayout* logLayout = new QVBoxLayout(groupLog);
    logLayout->setContentsMargins(12, 24, 12, 12);
    
    QTextEdit* textOutput = new QTextEdit();
    textOutput->setReadOnly(true);
    logLayout->addWidget(textOutput);
    leftLayout->addWidget(groupLog, 2);

    splitter->addWidget(leftPanel);

    // ================= ПРАВАЯ ПАНЕЛЬ С ГРАФИКАМИ =================
    QTabWidget* tabWidget = new QTabWidget();
    
    QScrollArea* scrollAreaGantt = new QScrollArea();
    GanttWidget* ganttWidget = new GanttWidget();
    scrollAreaGantt->setWidget(ganttWidget);
    scrollAreaGantt->setWidgetResizable(true);
    tabWidget->addTab(scrollAreaGantt, "Диаграмма интервалов (Гантт)");

    GraphWidget* graphWidget = new GraphWidget();
    tabWidget->addTab(graphWidget, "Динамическая топология (Граф)");

    splitter->addWidget(tabWidget);
    splitter->setSizes({420, 930}); // Явно задаем комфортное распределение по ширине

    // Палитра цветов для компонент связности
    vector<QColor> palette = {
        QColor("#E74C3C"), QColor("#3498DB"), QColor("#F1C40F"),
        QColor("#9B59B6"), QColor("#1ABC9C"), QColor("#E67E22"),
        QColor("#16A085"), QColor("#27AE60"), QColor("#2980B9")
    };

    // Общая функция обновления данных по такту
    auto updateTactData = [&]() {
        int n = spinN->value();
        int maxT = spinT->value();
        int currentTact = timeSlider->value();
        
        int tactStart = currentTact * 10;
        int tactEnd = (currentTact + 1) * 10;

        lblTact->setText(QString("Такт %1: [%2 - %3] мин").arg(currentTact + 1).arg(tactStart).arg(tactEnd));

        // Считываем список связей из поля ввода
        QStringList lines = textLinks->toPlainText().split('\n', Qt::SkipEmptyParts);
        vector<Link> allLinks;
        for (const QString& line : lines) {
            QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 4) {
                int u = parts[0].toInt();
                int v = parts[1].toInt();
                int ts = parts[2].toInt();
                int te = parts[3].toInt();
                allLinks.push_back({u, v, ts, te});
            }
        }

        // Выбираем связи, активные на текущем такте
        vector<Link> activeLinks;
        vector<vector<int>> adj(n + 1);
        for (const auto& l : allLinks) {
            if (isIntersecting(l.t_start, l.t_end, tactStart, tactEnd)) {
                activeLinks.push_back(l);
                if (l.u <= n && l.v <= n) {
                    adj[l.u].push_back(l.v);
                    adj[l.v].push_back(l.u);
                }
            }
        }

        // Поиск компонент связности (Группировок) с помощью BFS
        vector<int> component(n + 1, -1);
        int compCounter = 0;
        for (int i = 1; i <= n; ++i) {
            if (component[i] == -1) {
                queue<int> q;
                q.push(i);
                component[i] = compCounter;
                while (!q.empty()) {
                    int curr = q.front();
                    q.pop();
                    for (int neighbor : adj[curr]) {
                        if (component[neighbor] == -1) {
                            component[neighbor] = compCounter;
                            q.push(neighbor);
                        }
                    }
                }
                compCounter++;
            }
        }

        // Формируем вывод результатов
        vector<vector<int>> groups(compCounter);
        for (int i = 1; i <= n; ++i) {
            if (component[i] != -1) {
                groups[component[i]].push_back(i);
            }
        }

        QString resLog = QString("<b>Интервал: %1 - %2 мин</b><br>").arg(tactStart).arg(tactEnd);
        resLog += QString("Найдено группировок: %1<br><br>").arg(compCounter);

        for (int i = 0; i < compCounter; ++i) {
            resLog += QString("<span style='color:%1'>● Группировка %2:</span> ")
                      .arg(palette[i % palette.size()].name()).arg(i + 1);
            for (int node : groups[i]) {
                resLog += QString("КА_%1 ").arg(node);
            }
            resLog += "<br>";
        }
        textOutput->setHtml(resLog);

        // Обновляем виджеты отрисовки
        ganttWidget->setData(allLinks, maxT, currentTact);
        graphWidget->setData(n, activeLinks, component, palette);
    };

    // Связываем сигналы изменения параметров
    QObject::connect(timeSlider, &QSlider::valueChanged, updateTactData);
    QObject::connect(spinN, QOverload<int>::of(&QSpinBox::valueChanged), [&]() {
        updateTactData();
    });
    QObject::connect(spinT, QOverload<int>::of(&QSpinBox::valueChanged), [&](int val) {
        timeSlider->setRange(0, (val / 10) - 1);
        updateTactData();
    });
    
    QObject::connect(btnRecalc, &QPushButton::clicked, updateTactData);

    // Первичный расчет при запуске
    timeSlider->setRange(0, (spinT->value() / 10) - 1);
    updateTactData();

    window.show();
    return app.exec();
}

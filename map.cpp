#include <iostream>
#include <vector>
#include <random>
#include <string>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <omp.h>

using namespace std;
namespace fs = std::filesystem;

// Геометрия поля
const int MAIN_WIDTH = 200;                           // Ширина основного канала
const int TOOTH_WIDTH = 100;                          // Ширина выступа влево
const int TOTAL_WIDTH = MAIN_WIDTH + TOOTH_WIDTH;     // Полная ширина (300: 0..299)

const int BUF_HEIGHT = 200;                           // Высота буферной зоны
const int COMB_HEIGHT = 1000;                         // Высота гребенки
const int HEIGHT = BUF_HEIGHT + COMB_HEIGHT;          // Общая высота (1200: 0..1199)

const int COUNT_PARTICLES = 1000;                      // Количество частиц
const int COUNT_ITER = 20;                            // Количество итераций
const int TIME = 500'000;                           // Время симуляции
const int SAVE_INTERVAL = TIME / 10;                  // Шаг сохранения

// Настройки для тепловой карты (разбиение 300x1200 на бины)
const int BIN_X = 5;  
const int BIN_Y = 10; 
const int HEAT_COLS = TOTAL_WIDTH / BIN_X;            // 60 колонок
const int HEAT_ROWS = HEIGHT / BIN_Y;                 // 120 строк

struct Particle {
    int x, y;
};

// Отдельная функция проверки границ геометрии
bool is_inside_boundary(int x, int y) {
    // Проверка глобальных границ по Y
    if (y < 0 || y >= HEIGHT) return false;

    // 1. Буферная зона (0 .. 199): идет прямо вдоль основного канала
    if (y < BUF_HEIGHT) {
        return (x >= TOOTH_WIDTH && x < TOTAL_WIDTH);
    }

    // 2. Гребешковая структура (y от 200 до 1199)
    int y_comb = y - BUF_HEIGHT; // относительная координата гребня (0 .. 999)

    // Проверяем, попадает ли y_comb в один из интервалов с выступом
    bool has_tooth = (y_comb >= 100 && y_comb <= 299) ||
                     (y_comb >= 400 && y_comb <= 599) ||
                     (y_comb >= 700 && y_comb <= 899);

    if (has_tooth) {
        // Участок с выступом влево: доступно X от 0 до 299
        return (x >= 0 && x < TOTAL_WIDTH);
    } else {
        // Прямой участок: доступен только основной канал (X от 100 до 299)
        return (x >= TOOTH_WIDTH && x < TOTAL_WIDTH);
    }
}

// Сохранение матрицы для тепловой карты в CSV
void save_heatmap_csv(const vector<Particle>& particles, const fs::path& filepath) {
    vector<vector<int>> heatmap(HEAT_ROWS, vector<int>(HEAT_COLS, 0));

    for (const Particle& p : particles) {
        int bx = min(p.x / BIN_X, HEAT_COLS - 1);
        int by = min(p.y / BIN_Y, HEAT_ROWS - 1);
        heatmap[by][bx]++;
    }

    ofstream out(filepath);
    if (!out.is_open()) return;

    for (int r = HEAT_ROWS - 1; r >= 0; --r) {
        for (int c = 0; c < HEAT_COLS; ++c) {
            out << heatmap[r][c];
            if (c + 1 < HEAT_COLS) out << ",";
        }
        out << "\n";
    }
    out.close();
}

void run_simulation_iteration(int iter_num, const fs::path& base_dir) {
    fs::path iter_dir = base_dir / to_string(iter_num);
    
    if (!fs::exists(iter_dir)) {
        fs::create_directories(iter_dir);
    }

    random_device rd;
    mt19937 gen(rd() ^ (iter_num + 1));
    
    // Частицы стартуют в буфере (x: 100..299, y: 0..199)
    uniform_int_distribution<> distrib_x(TOOTH_WIDTH, TOTAL_WIDTH - 1);
    uniform_int_distribution<> distrib_y(0, BUF_HEIGHT - 1);
    uniform_int_distribution<> distrib_dir(0, 3);
    
    vector<Particle> particles;               
    particles.reserve(COUNT_PARTICLES);

    // Проверка занятости ячейки прямым проходом по частицам
    auto is_occupied = [&particles](int cx, int cy) {
        for (const auto& p : particles) {
            if (p.x == cx && p.y == cy) {
                return true;
            }
        }
        return false;
    };

    // Начальная расстановка
    while (particles.size() < COUNT_PARTICLES) {
        int rx = distrib_x(gen);
        int ry = distrib_y(gen);

        if (is_inside_boundary(rx, ry) && !is_occupied(rx, ry)) {
            particles.push_back({rx, ry});
        }
    }

    // Симуляция
    for (int step = 1; step <= TIME; ++step) {
        for (Particle& p : particles) {
            int dir = distrib_dir(gen); 
            
            int nx = p.x;
            int ny = p.y;

            if (dir == 0)      nx++; 
            else if (dir == 1) nx--; 
            else if (dir == 2) ny++; 
            else               ny--; 

            // Сначала проверяем геометрию границ, затем коллизию с другими частицами
            if (is_inside_boundary(nx, ny)) {
                if (!is_occupied(nx, ny)) {
                    p.x = nx;
                    p.y = ny;
                }
            }
        }

        if (step % SAVE_INTERVAL == 0) {
            fs::path filepath = iter_dir / (to_string(step) + ".csv");
            save_heatmap_csv(particles, filepath);
        }
    }
    
    #pragma omp critical
    cout << "Итерация " << iter_num << " завершена.\n";
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    fs::path base_dir = "./heatmaps";
    
    if (!fs::exists(base_dir)) {
        fs::create_directories(base_dir);
    }

    #pragma omp parallel for
    for (int u = 0; u < COUNT_ITER; u++) {
        run_simulation_iteration(u, base_dir);
    }

    return 0;
}
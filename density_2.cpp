#include <iostream>
#include <vector>
#include <random>
#include <string>
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

const int COUNT_PARTICLES = 1000;                     // Количество частиц
const int COUNT_ITER = 20;                            // Количество прогонов
const int TIME = 4'000'000;                           // Время симуляции
const int SAVE_INTERVAL = 10'000;                     // Сохраняем каждые 10 000 шагов

struct Particle {
    int x, y;
};

// Проверка границ геометрии
inline bool is_inside_boundary(int x, int y) {
    if (y < 0 || y >= HEIGHT) return false;

    // Буферная зона (0 .. 199)
    if (y < BUF_HEIGHT) {
        return (x >= TOOTH_WIDTH && x < TOTAL_WIDTH);
    }

    // Гребешковая зона (200 .. 1199)
    int y_comb = y - BUF_HEIGHT;
    bool has_tooth = (y_comb >= 100 && y_comb <= 299) ||
                     (y_comb >= 400 && y_comb <= 599) ||
                     (y_comb >= 700 && y_comb <= 899);

    if (has_tooth) {
        return (x >= 0 && x < TOTAL_WIDTH);
    } else {
        return (x >= TOOTH_WIDTH && x < TOTAL_WIDTH);
    }
}

void run_simulation_iteration(int iter_num, const fs::path& base_dir) {
    fs::path json_path = base_dir / ("iteration_" + to_string(iter_num) + ".json");
    ofstream json_out(json_path);
    if (!json_out.is_open()) return;

    // Начало формирования JSON
    json_out << "{\n  \"iteration\": " << iter_num << ",\n  \"steps\": {\n";

    random_device rd;
    mt19937 gen(rd() ^ (iter_num + 10007));

    uniform_int_distribution<> distrib_x(TOOTH_WIDTH, TOTAL_WIDTH - 1);
    uniform_int_distribution<> distrib_y(0, BUF_HEIGHT - 1);
    uniform_int_distribution<> distrib_dir(0, 3);

    vector<Particle> particles;
    particles.reserve(COUNT_PARTICLES);

    // Сетка O(1) для исключения перекрытия частиц
    vector<vector<bool>> occupied(TOTAL_WIDTH, vector<bool>(HEIGHT, false));

    // Начальное распределение
    while (particles.size() < COUNT_PARTICLES) {
        int rx = distrib_x(gen);
        int ry = distrib_y(gen);

        if (is_inside_boundary(rx, ry) && !occupied[rx][ry]) {
            particles.push_back({rx, ry});
            occupied[rx][ry] = true;
        }
    }

    bool is_first_save = true;

    // Цикл симуляции
    for (int step = 1; step <= TIME; ++step) {
        for (Particle& p : particles) {
            int dir = distrib_dir(gen);

            int nx = p.x;
            int ny = p.y;

            if (dir == 0)      nx++;
            else if (dir == 1) nx--;
            else if (dir == 2) ny++;
            else               ny--;

            if (is_inside_boundary(nx, ny) && !occupied[nx][ny]) {
                occupied[p.x][p.y] = false;
                p.x = nx;
                p.y = ny;
                occupied[p.x][p.y] = true;
            }
        }

        // Сохранение координат в JSON каждые 10 000 шагов
        if (step % SAVE_INTERVAL == 0) {
            if (!is_first_save) {
                json_out << ",\n";
            }
            is_first_save = false;

            json_out << "    \"" << step << "\": [";
            for (size_t i = 0; i < particles.size(); ++i) {
                json_out << "{\"x\":" << particles[i].x << ",\"y\":" << particles[i].y << "}";
                if (i + 1 < particles.size()) json_out << ",";
            }
            json_out << "]";
        }
    }

    // Закрываем JSON структуру
    json_out << "\n  }\n}\n";
    json_out.close();

    #pragma omp critical
    cout << "Итерация " << iter_num << " успешно завершена и сохранена.\n";
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    fs::path base_dir = "./plotnost";
    if (!fs::exists(base_dir)) {
        fs::create_directories(base_dir);
    }

    cout << "Запуск " << COUNT_ITER << " итераций с параллелизацией через OpenMP...\n";

    #pragma omp parallel for schedule(dynamic)
    for (int u = 0; u < COUNT_ITER; ++u) {
        run_simulation_iteration(u, base_dir);
    }

    cout << "Все 20 файлов сохранены в папку: " << base_dir << "\n";
    return 0;
}
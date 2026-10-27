#include <iostream>
#include <vector>
#include <random>
#include <string>
#include <filesystem>
#include <fstream>

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
const int TIME = 500'000;                             // Время симуляции
const int SAVE_INTERVAL = TIME / 10;                  // Шаг сохранения (50 000)

struct Particle {
    int x, y;
};

// Функция проверки границ геометрии (как у вас)
bool is_inside_boundary(int x, int y) {
    if (y < 0 || y >= HEIGHT) return false;

    // 1. Буферная зона (0 .. 199)
    if (y < BUF_HEIGHT) {
        return (x >= TOOTH_WIDTH && x < TOTAL_WIDTH);
    }

    // 2. Гребешковая структура (y от 200 до 1199)
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

// Сохранение координат точек в CSV
void save_particles_csv(const vector<Particle>& particles, const fs::path& filepath) {
    ofstream out(filepath);
    if (!out.is_open()) return;

    out << "x,y\n";
    for (const Particle& p : particles) {
        out << p.x << "," << p.y << "\n";
    }
    out.close();
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    fs::path base_dir = "./particle_coords";
    if (!fs::exists(base_dir)) {
        fs::create_directories(base_dir);
    }

    random_device rd;
    mt19937 gen(rd());

    // Частицы стартуют в буфере (x: 100..299, y: 0..199)
    uniform_int_distribution<> distrib_x(TOOTH_WIDTH, TOTAL_WIDTH - 1);
    uniform_int_distribution<> distrib_y(0, BUF_HEIGHT - 1);
    uniform_int_distribution<> distrib_dir(0, 3);

    vector<Particle> particles;
    particles.reserve(COUNT_PARTICLES);

    // Ваша проверка занятости ячейки прямым проходом по вектору частиц
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

    cout << "Симуляция запущена (1 прогон, " << TIME << " шагов)...\n";

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

            // Сначала геометрия границ, затем коллизия с другими частицами
            if (is_inside_boundary(nx, ny)) {
                if (!is_occupied(nx, ny)) {
                    p.x = nx;
                    p.y = ny;
                }
            }
        }

        if (step % SAVE_INTERVAL == 0) {
            fs::path filepath = base_dir / (to_string(step) + ".csv");
            save_particles_csv(particles, filepath);
            cout << "Сохранен шаг: " << step << " / " << TIME << "\n";
        }
    }

    cout << "Готово! Координаты сохранены в " << base_dir << "/\n";
    return 0;
}
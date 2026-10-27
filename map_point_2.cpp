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

// Смещения: 0: вправо, 1: влево, 2: вниз, 3: вверх
const int DX[4] = {1, -1, 0, 0};
const int DY[4] = {0, 0, 1, -1};

// Проверка геометрии стенок
inline bool is_inside_boundary(int x, int y) {
    if (y < 0 || y >= HEIGHT) return false;

    // Буферная зона (0 .. 199)
    if (y < BUF_HEIGHT) {
        return (x >= TOOTH_WIDTH && x < TOTAL_WIDTH);
    }

    // Гребенка (200 .. 1199)
    int y_comb = y - BUF_HEIGHT;
    bool has_tooth = (y_comb >= 100 && y_comb <= 299) ||
                     (y_comb >= 400 && y_comb <= 599) ||
                     (y_comb >= 700 && y_comb <= 899);

    if (has_tooth) {
        return (x >= 0 && x < TOTAL_WIDTH);
    }
    return (x >= TOOTH_WIDTH && x < TOTAL_WIDTH);
}

// Быстрое сохранение координат в CSV
void save_particles_csv(const vector<Particle>& particles, const fs::path& filepath) {
    ofstream out(filepath, ios::out | ios::trunc);
    if (!out.is_open()) return;

    string buffer;
    buffer.reserve(COUNT_PARTICLES * 16);
    buffer += "x,y\n";

    for (const auto& p : particles) {
        buffer += to_string(p.x) + "," + to_string(p.y) + "\n";
    }
    out.write(buffer.data(), buffer.size());
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    fs::path base_dir = "./particle_coords";
    fs::create_directories(base_dir);

    random_device rd;
    mt19937 gen(rd());

    uniform_int_distribution<> distrib_x(TOOTH_WIDTH, TOTAL_WIDTH - 1);
    uniform_int_distribution<> distrib_y(0, BUF_HEIGHT - 1);

    // Сетка занятости O(1): 1200 x 300
    vector<uint8_t> occupied(HEIGHT * TOTAL_WIDTH, 0);
    vector<Particle> particles;
    particles.reserve(COUNT_PARTICLES);

    // Начальное размещение частиц в буфере
    while (particles.size() < COUNT_PARTICLES) {
        int rx = distrib_x(gen);
        int ry = distrib_y(gen);
        int idx = ry * TOTAL_WIDTH + rx;

        if (is_inside_boundary(rx, ry) && !occupied[idx]) {
            occupied[idx] = 1;
            particles.push_back({rx, ry});
        }
    }

    cout << "Симуляция запущена (1 прогон, " << TIME << " шагов)...\n";

    // Главный цикл симуляции
    for (int step = 1; step <= TIME; ++step) {
        for (Particle& p : particles) {
            int dir = gen() & 3; // Случайное направление 0..3
            int nx = p.x + DX[dir];
            int ny = p.y + DY[dir];

            if (is_inside_boundary(nx, ny)) {
                int next_idx = ny * TOTAL_WIDTH + nx;
                if (!occupied[next_idx]) {
                    occupied[p.y * TOTAL_WIDTH + p.x] = 0;
                    occupied[next_idx] = 1;
                    p.x = nx;
                    p.y = ny;
                }
            }
        }

        if (step % SAVE_INTERVAL == 0) {
            fs::path file_path = base_dir / (to_string(step) + ".csv");
            save_particles_csv(particles, file_path);
            cout << "Сохранен шаг: " << step << " / " << TIME << "\n";
        }
    }

    cout << "Симуляция успешно завершена! Данные в: " << base_dir << "/\n";
    return 0;
}
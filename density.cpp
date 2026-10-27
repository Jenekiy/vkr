#include <iostream>
#include <vector>
#include <random>
#include <string>
#include <filesystem>
#include <fstream>
#include <iomanip>

using namespace std;
namespace fs = std::filesystem;

// Геометрия поля
const int MAIN_WIDTH = 200;                           // Ширина основного канала
const int TOOTH_WIDTH = 100;                          // Ширина выступа влево
const int TOTAL_WIDTH = MAIN_WIDTH + TOOTH_WIDTH;     // Полная ширина (300: 0..299)

const int BUF_HEIGHT = 200;                           // Высота буферной зоны
const int COMB_HEIGHT = 1000;                         // Высота гребенки
const int HEIGHT = BUF_HEIGHT + COMB_HEIGHT;          // Общая высота (1200: 0..1199)

// Площади зон (в ячейках/пикселях)
const double AREA_MAIN = static_cast<double>(MAIN_WIDTH * COMB_HEIGHT); // 200 * 1000 = 200 000
const double AREA_TOOTH = static_cast<double>(TOOTH_WIDTH * 200);       // 100 * 200 = 20 000 (одинакова для всех 3 гребней)

const int COUNT_PARTICLES = 1000;                     // Количество частиц
const int TIME = 500'000;                             // Время симуляции
const int SAVE_INTERVAL = TIME / 10;                  // Шаг сохранения частиц (50 000)
const int DENSITY_SAVE_INTERVAL = 100;                // Шаг сохранения плотности

struct Particle {
    int x, y;
};

// Функция проверки границ геометрии
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
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    fs::path base_dir = "./particle_coords";
    if (!fs::exists(base_dir)) {
        fs::create_directories(base_dir);
    }

    // Файл для записи плотностей
    ofstream density_out("densities.csv");
    density_out << "step,main_channel,tooth_1,tooth_2,tooth_3\n";
    // Фиксируем вывод чисел с плавающей точкой
    density_out << setprecision(8);

    random_device rd;
    mt19937 gen(rd());

    uniform_int_distribution<> distrib_x(TOOTH_WIDTH, TOTAL_WIDTH - 1);
    uniform_int_distribution<> distrib_y(0, BUF_HEIGHT - 1);
    uniform_int_distribution<> distrib_dir(0, 3);

    vector<Particle> particles;
    particles.reserve(COUNT_PARTICLES);

    // Сетка занятости (occupied grid) для мгновенной O(1) проверки коллизий
    vector<vector<bool>> occupied(TOTAL_WIDTH, vector<bool>(HEIGHT, false));

    // Начальная расстановка
    while (particles.size() < COUNT_PARTICLES) {
        int rx = distrib_x(gen);
        int ry = distrib_y(gen);

        if (is_inside_boundary(rx, ry) && !occupied[rx][ry]) {
            particles.push_back({rx, ry});
            occupied[rx][ry] = true;
        }
    }

    cout << "Симуляция запущена (1 прогон, " << TIME << " шагов)...\n";

    // Основной цикл симуляции
    for (int step = 1; step <= TIME; ++step) {
        for (Particle& p : particles) {
            int dir = distrib_dir(gen);

            int nx = p.x;
            int ny = p.y;

            if (dir == 0)      nx++;
            else if (dir == 1) nx--;
            else if (dir == 2) ny++;
            else               ny--;

            // Проверка геометрии и коллизий за O(1)
            if (is_inside_boundary(nx, ny) && !occupied[nx][ny]) {
                occupied[p.x][p.y] = false;
                p.x = nx;
                p.y = ny;
                occupied[p.x][p.y] = true;
            }
        }

        // Подсчет и сохранение плотности каждые 100 шагов
        if (step % DENSITY_SAVE_INTERVAL == 0) {
            int count_main = 0;
            int count_t1 = 0;
            int count_t2 = 0;
            int count_t3 = 0;

            for (const Particle& p : particles) {
                // Исключаем буферную зону (y < BUF_HEIGHT)
                if (p.y >= BUF_HEIGHT) {
                    if (p.x >= TOOTH_WIDTH) {
                        // Основной прямоугольник (x: 100..299, y: 200..1199)
                        count_main++;
                    } else {
                        // Зубцы/гребни (x: 0..99)
                        int y_comb = p.y - BUF_HEIGHT;
                        if (y_comb >= 100 && y_comb <= 299) {
                            count_t1++;
                        } else if (y_comb >= 400 && y_comb <= 599) {
                            count_t2++;
                        } else if (y_comb >= 700 && y_comb <= 899) {
                            count_t3++;
                        }
                    }
                }
            }

            double rho_main = count_main / AREA_MAIN;
            double rho_t1   = count_t1 / AREA_TOOTH;
            double rho_t2   = count_t2 / AREA_TOOTH;
            double rho_t3   = count_t3 / AREA_TOOTH;

            density_out << step << ","
                        << rho_main << ","
                        << rho_t1 << ","
                        << rho_t2 << ","
                        << rho_t3 << "\n";
        }

        // Сохранение координат частиц (раз в 50 000 шагов)
        if (step % SAVE_INTERVAL == 0) {
            fs::path filepath = base_dir / (to_string(step) + ".csv");
            save_particles_csv(particles, filepath);
            cout << "Шаг: " << step << " / " << TIME << "\n";
        }
    }

    density_out.close();
    cout << "Готово!\n";
    cout << "- Координаты сохранены в: " << base_dir << "/\n";
    cout << "- Плотности сохранены в: densities.csv\n";

    return 0;
}
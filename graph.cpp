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

const int WIDTH = 200; // Ширина поля
const int HEIGHT = 1000; // Высота поля
const int BUF_HEIGHT = 200; // Высота буферной зоны
const int VALS = 25; // Количество интервалов
const int COUNT_PARTICLES = 500; // Количество частиц
const int COUNT_ITER = 20; // Количество итераций
const int TIME = 1'000'000; // Время (1 единица = 1 ход частицы)
const int SAVE_INTERVAL = TIME / 10; // Шаг сохранения файлов
const int LEN_VALS = (HEIGHT - BUF_HEIGHT) % VALS == 0 ? (HEIGHT - BUF_HEIGHT) / VALS : (HEIGHT - BUF_HEIGHT) / VALS + 1; // Длина интервала

struct Particle {
    int x, y;
};

void save_graph(const vector<Particle>& particles, const fs::path& filepath) {
    vector<int> res(VALS, 0);

    for (const Particle& p : particles) {
        int a = p.y - BUF_HEIGHT; 
        if (a >= 0) {
            int interval_idx = a / LEN_VALS;
            if (interval_idx < VALS) {
                res[interval_idx]++;
            }
        }
    }

    int mx = 0;
    for (int count : res) {
        mx = max(mx, count);
    }

    int rows = mx + 1; 
    vector<string> gr(rows, string(VALS, ' '));
    
    for (int i = 0; i < VALS; i++) {
        for (int j = 0; j < res[i]; j++) {
            if (j < rows) {
                gr[j][i] = '*';
            }
        }
    }

    ofstream out(filepath);
    if (!out.is_open()) return;

    for (int i = rows - 1; i >= 0; i--) {
        out << gr[i] << '\n';
    }
    out.close();
}

void run_simulation_iteration(int iter_num, const fs::path& base_dir) {
    fs::path iter_dir = base_dir / to_string(iter_num);
    
    if (!fs::exists(iter_dir)) {
        fs::create_directories(iter_dir);
    }

    random_device rd;
    mt19937 gen(rd() ^ iter_num);
    
    uniform_int_distribution<> distrib_x(0, WIDTH - 1);
    uniform_int_distribution<> distrib_y(0, BUF_HEIGHT - 1);
    uniform_int_distribution<> distrib_dir(0, 3);
    
    vector<Particle> particles;               
    particles.reserve(COUNT_PARTICLES);

    auto is_occupied = [&particles](int cx, int cy) {
        for (const auto& p : particles) {
            if (p.x == cx && p.y == cy) {
                return true;
            }
        }
        return false;
    };

    while (particles.size() < COUNT_PARTICLES) {
        int rx = distrib_x(gen);
        int ry = distrib_y(gen);

        if (!is_occupied(rx, ry)) {
            particles.push_back({rx, ry});
        }
    }

    for (int step = 1; step <= TIME; ++step) {
        for (Particle& p : particles) {
            int dir = distrib_dir(gen); 
            
            int nx = p.x;
            int ny = p.y;

            if (dir == 0)      nx++; 
            else if (dir == 1) nx--; 
            else if (dir == 2) ny++; 
            else               ny--; 

            if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {
                if (!is_occupied(nx, ny)) {
                    p.x = nx;
                    p.y = ny;
                }
            }
        }

        if (step % SAVE_INTERVAL == 0) {
            fs::path filepath = iter_dir / (to_string(step) + ".txt");
            save_graph(particles, filepath);
        }
    }
    
    #pragma omp critical
    cout << "Итерация " << iter_num << " завершена.\n";
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    fs::path base_dir = "/home/jenya/projects/hello_cpp/graphs";
    
    if (!fs::exists(base_dir)) {
        fs::create_directories(base_dir);
    }

    #pragma omp parallel for
    for (int u = 0; u < COUNT_ITER; u++) {
        run_simulation_iteration(u, base_dir);
    }

    return 0;
}
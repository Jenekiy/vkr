import os
import glob
import json
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.ticker import MultipleLocator

# Константы геометрии
MAIN_WIDTH = 200
TOOTH_WIDTH = 100
COMB_HEIGHT = 1000
BUF_HEIGHT = 200

# Площади зон (в пикселях / ячейках)
AREA_MAIN = float(MAIN_WIDTH * COMB_HEIGHT)  # 200 * 1000 = 200 000
AREA_TOOTH = float(TOOTH_WIDTH * 200)        # 100 * 200 = 20 000

json_dir = "./plotnost"
json_files = glob.glob(os.path.join(json_dir, "iteration_*.json"))

if not json_files:
    raise FileNotFoundError(f"Файлы JSON не найдены в {json_dir}")

print(f"Найдено файлов для анализа: {len(json_files)}")

data_by_step = {}

for filepath in json_files:
    with open(filepath, 'r') as f:
        run_data = json.load(f)

    steps_dict = run_data.get("steps", {})
    for step_str, particles in steps_dict.items():
        step = int(step_str)
        if step not in data_by_step:
            data_by_step[step] = {'main': [], 't1': [], 't2': [], 't3': []}

        count_main = 0
        count_t1 = 0
        count_t2 = 0
        count_t3 = 0

        for p in particles:
            x, y = p['x'], p['y']
            # Зоны внутри гребенки (y >= 200)
            if y >= BUF_HEIGHT:
                if x >= TOOTH_WIDTH:
                    count_main += 1
                else:
                    y_comb = y - BUF_HEIGHT
                    if 100 <= y_comb <= 299:
                        count_t1 += 1
                    elif 400 <= y_comb <= 599:
                        count_t2 += 1
                    elif 700 <= y_comb <= 899:
                        count_t3 += 1

        data_by_step[step]['main'].append(count_main / AREA_MAIN)
        data_by_step[step]['t1'].append(count_t1 / AREA_TOOTH)
        data_by_step[step]['t2'].append(count_t2 / AREA_TOOTH)
        data_by_step[step]['t3'].append(count_t3 / AREA_TOOTH)

steps = sorted(data_by_step.keys())

mean_main = [np.mean(data_by_step[s]['main']) for s in steps]
mean_t1   = [np.mean(data_by_step[s]['t1'])   for s in steps]
mean_t2   = [np.mean(data_by_step[s]['t2'])   for s in steps]
mean_t3   = [np.mean(data_by_step[s]['t3'])   for s in steps]

plt.style.use('seaborn-v0_8-whitegrid' if 'seaborn-v0_8-whitegrid' in plt.style.available else 'default')
plt.rcParams['font.size'] = 11

fig, ax = plt.subplots(figsize=(13, 7))

ax.plot(steps, mean_main, label='Основной канал (усреднено)', color='blue', linewidth=2)
ax.plot(steps, mean_t1,   label='Гребень 1 (верхний)', color='crimson', linewidth=1.8, alpha=0.9)
ax.plot(steps, mean_t2,   label='Гребень 2 (средний)', color='forestgreen', linewidth=1.8, alpha=0.9)
ax.plot(steps, mean_t3,   label='Гребень 3 (нижний)',  color='darkorange', linewidth=1.8, alpha=0.9)

ax.set_title(f'Динамика плотности частиц во времени (N / S, усреднение по {len(json_files)} итерациям)', 
             fontsize=14, fontweight='bold', pad=12)
ax.set_xlabel('Количество шагов (Steps)', fontsize=12)
ax.set_ylabel('Усредненная плотность (частиц / ячейка)', fontsize=12)

# Каждое деление оси X = 10 000 шагов
ax.xaxis.set_major_locator(MultipleLocator(50000))  # Основные подписи с текстом (раз в 50 000)
ax.xaxis.set_minor_locator(MultipleLocator(10000))  # Насечки ровно через каждые 10 000 шагов
ax.tick_params(axis='x', which='minor', length=4, color='gray')
ax.tick_params(axis='x', which='major', length=7)

ax.legend(loc='best', frameon=True)
ax.grid(True, which='both', linestyle='--', alpha=0.6)
plt.tight_layout()

plt.savefig('densities_combined_avg.png', dpi=300)
plt.show()

fig, axes = plt.subplots(2, 2, figsize=(15, 10), sharex=True)

zones = [
    (mean_main, 'Основной прямоугольник', 'blue', axes[0, 0]),
    (mean_t1,   'Гребень 1 (верхний)', 'crimson', axes[0, 1]),
    (mean_t2,   'Гребень 2 (средний)', 'forestgreen', axes[1, 0]),
    (mean_t3,   'Гребень 3 (нижний)',  'darkorange', axes[1, 1])
]

for mean_vals, title, color, ax in zones:
    ax.plot(steps, mean_vals, color=color, linewidth=2)
    ax.set_title(title, fontsize=12, fontweight='bold')
    ax.set_ylabel('Усредненная плотность', fontsize=10)

    ax.xaxis.set_major_locator(MultipleLocator(50000))
    ax.xaxis.set_minor_locator(MultipleLocator(10000))
    ax.grid(True, which='both', linestyle='--', alpha=0.5)

axes[1, 0].set_xlabel('Количество шагов', fontsize=11)
axes[1, 1].set_xlabel('Количество шагов', fontsize=11)

plt.suptitle(f'Усредненная плотность частиц по зонам ({len(json_files)} прогонов)', fontsize=15, fontweight='bold')
plt.tight_layout()

plt.savefig('densities_subplots_avg.png', dpi=300)
plt.show()
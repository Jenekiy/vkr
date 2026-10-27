import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

# ==============================================================================
#                      ГЛОБАЛЬНЫЕ НАСТРОЙКИ (КОНФИГУРАЦИЯ)
# ==============================================================================
TOTAL_TIME    = 500_000       # Значение TIME из C++
SAVE_INTERVAL = 50_000       # Значение SAVE_INTERVAL из C++ (TIME / 10)
COUNT_ITER    = 20           # Значение COUNT_ITER из C++

BASE_DIR      = "./heatmaps"           # Откуда берем данные C++
OUT_DIR       = "./heatmaps_averaged"  # Куда сохраняем результат
COLORMAP      = "turbo"                # Цветовая палитра ("turbo", "inferno", "magma")
# ==============================================================================

# Создаем папку вывода (если нет)
os.makedirs(OUT_DIR, exist_ok=True)

# Формируем строгий список шагов по константам: [50000, 100000, ..., 500000]
steps = list(range(SAVE_INTERVAL, TOTAL_TIME + 1, SAVE_INTERVAL))

print(f"--- Запуск обработки ---")
print(f"Ожидаемые шаги времени: {steps}")
print(f"Количество итераций:    {COUNT_ITER}")

avg_maps = {}
global_max = 0.0

# 1. Читаем и усредняем ТОЛЬКО нужные файлы
for step in steps:
    matrices = []
    
    # Ищем файл строго в пределах заданного числа итераций (0 .. COUNT_ITER-1)
    for it in range(COUNT_ITER):
        file_path = os.path.join(BASE_DIR, str(it), f"{step}.csv")
        if os.path.exists(file_path):
            matrices.append(pd.read_csv(file_path, header=None).values)
    
    if not matrices:
        print(f"Предупреждение: для шага {step} файлов не найдено! Проверьте симуляцию.")
        continue
    
    # Поэлементное усреднение матрицы по найденным итерациям
    avg_matrix = np.mean(matrices, axis=0)
    avg_maps[step] = avg_matrix
    global_max = max(global_max, float(np.max(avg_matrix)))
    
    # Сохраняем усредненный CSV (перезаписывает старый при совпадении)
    out_csv = os.path.join(OUT_DIR, f"{step}_avg.csv")
    pd.DataFrame(avg_matrix).to_csv(out_csv, header=False, index=False)
    print(f"Шаг {step:>8}: успешно усреднен по {len(matrices)} прогонам.")

if not avg_maps:
    print("Ошибка: ни один файл симуляции не найден. Убедитесь, что ./sim отработал.")
    exit(1)

# 2. Отрисовка сводной сетки (автоматический расчет числа строк и колонок)
cols = 5
rows = (len(steps) + cols - 1) // cols
fig, axes = plt.subplots(rows, cols, figsize=(cols * 3.2, rows * 6), squeeze=False)

for idx, step in enumerate(steps):
    r = idx // cols
    c = idx % cols
    ax = axes[r][c]
    
    if step in avg_maps:
        sns.heatmap(
            avg_maps[step], 
            ax=ax, 
            cmap=COLORMAP, 
            vmin=0, 
            vmax=global_max, 
            cbar=False, 
            xticklabels=False, 
            yticklabels=False
        )
        ax.set_title(f"t = {step:,}", fontsize=11, fontweight="bold")
    
    if c == 0:
        ax.set_ylabel("Высота Y (сверху вниз)")
    if r == rows - 1:
        ax.set_xlabel("Ширина X")

    # 3. Сохранение каждого отдельного шага картинкой
    if step in avg_maps:
        fig_single, ax_single = plt.subplots(figsize=(4, 8))
        sns.heatmap(
            avg_maps[step], 
            ax=ax_single, 
            cmap=COLORMAP, 
            vmin=0, 
            vmax=global_max, 
            cbar=True,
            xticklabels=False, 
            yticklabels=False
        )
        ax_single.set_title(f"Усредненная плотность (t = {step:,})")
        fig_single.savefig(os.path.join(OUT_DIR, f"step_{step}.png"), dpi=150, bbox_inches="tight")
        plt.close(fig_single)

# Скрываем неиспользуемые ячейки сетки, если графиков меньше, чем строк * столбцов
for idx in range(len(steps), rows * cols):
    axes[idx // cols][idx % cols].axis('off')

# Общий Colorbar справа
fig.subplots_adjust(right=0.88, hspace=0.22, wspace=0.15)
cbar_ax = fig.add_axes([0.90, 0.15, 0.02, 0.7])
sm = plt.cm.ScalarMappable(cmap=COLORMAP, norm=plt.Normalize(vmin=0, vmax=global_max))
fig.colorbar(sm, cax=cbar_ax, label="Средняя плотность частиц")

grid_output = "avg_heatmaps_grid.png"
fig.savefig(grid_output, dpi=200, bbox_inches="tight")
plt.close(fig)

print(f"\n Готово!")
print(f"-> Сводная сетка обновлена: {grid_output}")
print(f"-> Картинки шагов обновлены в: {OUT_DIR}/")
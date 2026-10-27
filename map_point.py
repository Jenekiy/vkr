import os
import matplotlib.pyplot as plt
from matplotlib.patches import Polygon
import pandas as pd

TOTAL_TIME    = 500_000
SAVE_INTERVAL = 50_000
BASE_DIR      = "./particle_coords"
OUT_DIR       = "./particle_plots"

os.makedirs(OUT_DIR, exist_ok=True)
steps = list(range(SAVE_INTERVAL, TOTAL_TIME + 1, SAVE_INTERVAL))

def get_geometry_polygon_horizontal():
    """
    Горизонтальный контур расчетной области:
    - Длина канала: 1200 px (слева направо)
    - Ширина канала: 300 px (снизу вверх)
    - Буферная зона: X от 0 до 200 (слева)
    - Зубья гребенки: опущены вниз (Y от 0 до 100) на участках X in [300, 500], [600, 800], [900, 1100]
    """
    poly_pts = [
        # 1. Левая стенка буфера
        (0, 100),
        (0, 300),
        # 2. Верхняя сплошная стенка основного канала
        (1200, 300),
        # 3. Правый тупик канала
        (1200, 100),
        # 4. Нижняя стенка с зубьями (идем справа налево)
        (1100, 100),
        (1100, 0), (900, 0),   # Выступ 3
        (900, 100),
        (800, 100),
        (800, 0), (600, 0),     # Выступ 2
        (600, 100),
        (500, 100),
        (500, 0), (300, 0),     # Выступ 1
        (300, 100),
        # 5. Возврат к началу буфера
        (0, 100)
    ]
    return Polygon(
        poly_pts, 
        closed=True, 
        facecolor="#F8FAFC", 
        edgecolor="#1E293B", 
        linewidth=1.6, 
        zorder=1
    )

def draw_horizontal_boundaries(ax):
    """Отрисовывает границы горизонтального канала."""
    ax.add_patch(get_geometry_polygon_horizontal())
    ax.set_xlim(-20, 1220)
    ax.set_ylim(-20, 320)
    ax.set_aspect("equal")

print("--- Генерация горизонтальных графиков ---")

# 1. Одиночные панорамные графики для каждого шага
for step in steps:
    file_path = os.path.join(BASE_DIR, f"{step}.csv")
    if not os.path.exists(file_path):
        continue

    df = pd.read_csv(file_path)

    # Широкий формат под горизонтальный канал (1200x300)
    fig, ax = plt.subplots(figsize=(12, 3.8))
    draw_horizontal_boundaries(ax)

    # Внимание: разворот осей!
    # X_plot = Y_sim (направление движения), Y_plot = X_sim (поперечная координата)
    ax.scatter(df["y"], df["x"], s=8, color="#E11D48", alpha=0.85, zorder=2, edgecolors="none")

    ax.set_title(f"Распространение частиц: t = {step:,} шагов", fontsize=11, fontweight="bold")
    ax.set_xlabel("Ось распространения (X, px)")
    ax.set_ylabel("Ширина (Y, px)")
    ax.grid(True, linestyle=":", alpha=0.4)

    fig.savefig(os.path.join(OUT_DIR, f"horizontal_step_{step}.png"), dpi=200, bbox_inches="tight")
    plt.close(fig)
    print(f"Шаг {step:>7}: сохранен в {OUT_DIR}/horizontal_step_{step}.png")

# 2. Сводная сетка (2 колонки x 5 строк для горизонтальных каналов)
cols = 2
rows = (len(steps) + cols - 1) // cols
fig, axes = plt.subplots(rows, cols, figsize=(cols * 8.5, rows * 2.8), squeeze=False)

for idx, step in enumerate(steps):
    r, c = idx // cols, idx % cols
    ax = axes[r][c]
    draw_horizontal_boundaries(ax)

    file_path = os.path.join(BASE_DIR, f"{step}.csv")
    if os.path.exists(file_path):
        df = pd.read_csv(file_path)
        ax.scatter(df["y"], df["x"], s=3.5, color="#E11D48", alpha=0.8, zorder=2, edgecolors="none")

    ax.set_title(f"t = {step:,}", fontsize=10, fontweight="bold")
    if c == 0:
        ax.set_ylabel("Y")
    if r == rows - 1:
        ax.set_xlabel("X (распространение)")

plt.tight_layout()
grid_output = os.path.join(OUT_DIR, "horizontal_grid.png")
fig.savefig(grid_output, dpi=200, bbox_inches="tight")
plt.close(fig)

print(f"\nГотово!")
print(f"-> Итоговая сетка: {grid_output}")
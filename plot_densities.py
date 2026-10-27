import pandas as pd
import matplotlib.pyplot as plt

# 1. Загрузка данных
csv_file = "densities.csv"
data = pd.read_csv(csv_file)

# Настройка стилей графика
plt.style.use('seaborn-v0_8-whitegrid' if 'seaborn-v0_8-whitegrid' in plt.style.available else 'default')
plt.rcParams['font.size'] = 11

# ==========================================
# ВАРИАНТ 1: Все зоны на одном общем графике
# ==========================================
plt.figure(figsize=(10, 6))

plt.plot(data['step'], data['main_channel'], label='Основной канал', color='blue', linewidth=1.5)
plt.plot(data['step'], data['tooth_1'], label='Гребень 1 (верхний)', color='crimson', linewidth=1.2, alpha=0.85)
plt.plot(data['step'], data['tooth_2'], label='Гребень 2 (средний)', color='forestgreen', linewidth=1.2, alpha=0.85)
plt.plot(data['step'], data['tooth_3'], label='Гребень 3 (нижний)', color='darkorange', linewidth=1.2, alpha=0.85)

plt.title('Динамика плотности частиц во времени (N / S)', fontsize=14, fontweight='bold', pad=12)
plt.xlabel('Количество шагов (Steps)', fontsize=12)
plt.ylabel('Плотность (частиц / ячейка)', fontsize=12)
plt.legend(loc='best', frameon=True)
plt.grid(True, linestyle='--', alpha=0.6)
plt.tight_layout()

# Сохранение и отображение общего графика
plt.savefig('densities_combined.png', dpi=300)
plt.show()

# ==========================================
# ВАРИАНТ 2: 4 отдельных графика (сетка 2х2)
# ==========================================
fig, axes = plt.subplots(2, 2, figsize=(14, 9), sharex=True)

zones = [
    ('main_channel', 'Основной прямоугольник', 'blue', axes[0, 0]),
    ('tooth_1', 'Гребень 1 (верхний)', 'crimson', axes[0, 1]),
    ('tooth_2', 'Гребень 2 (средний)', 'forestgreen', axes[1, 0]),
    ('tooth_3', 'Гребень 3 (нижний)', 'darkorange', axes[1, 1])
]

for col_name, title, color, ax in zones:
    ax.plot(data['step'], data[col_name], color=color, linewidth=1.5)
    ax.set_title(title, fontsize=12, fontweight='bold')
    ax.set_ylabel('Плотность', fontsize=10)
    ax.grid(True, linestyle='--', alpha=0.6)
    
    # Сглаживание скользящим средним (опционально, если линия слишком шумит)
    # rolling_avg = data[col_name].rolling(window=20).mean()
    # ax.plot(data['step'], rolling_avg, color='black', linestyle='--', alpha=0.7, label='Тренд')

axes[1, 0].set_xlabel('Количество шагов', fontsize=11)
axes[1, 1].set_xlabel('Количество шагов', fontsize=11)

plt.suptitle('Плотность частиц по зонам', fontsize=15, fontweight='bold')
plt.tight_layout()

# Сохранение и отображение сетки графиков
plt.savefig('densities_subplots.png', dpi=300)
plt.show()
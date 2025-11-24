# import csv
# import os
# import matplotlib as mpl

# def plot_csv(data_filename, plot_filename, show=False):

#     # choose a backend before importing pyplot
#     if not show:
#         mpl.use('Agg')  # non-interactive backend for headless saving

#     import matplotlib.pyplot as plt

#     generations = []
#     best = []
#     average = []

#     with open(data_filename, newline='') as csvfile:
#         reader = csv.DictReader(csvfile, delimiter=';')
#         for row in reader:
#             generations.append(int(row['generation']))
#             best.append(float(row['best_fitness']))
#             average.append(float(row['average_fitness']))

#     last_gen = generations[-1]

#     plt.figure()
#     plt.plot(generations, best, label='Best')
#     plt.plot(generations, average, label='Average')
#     plt.xlabel('Generation')
#     plt.ylabel('Score')
#     plt.title(f'Gen: {last_gen + 1}')
#     plt.legend()
#     plt.grid(True)
#     plt.ylim(bottom=0)
    
#     plt.savefig(plot_filename, dpi=300, bbox_inches='tight')
#     print(f"Saved plot to {plot_filename}")

#     if show:
#         plt.show()
#     plt.close()  # ensure the figure is closed

# if __name__ == "__main__":
#     import sys
#     # pass --s on the command line to display the plot interactively
#     show_flag = '--s' in sys.argv
#     plot_csv('data/genome_fitness.csv', 'data/plot.png', show=show_flag)

import csv
import matplotlib as mpl

def plot_csv(data_filename, plot_filename, show=False):
    
    # https://coolors.co/ffaf25-fd5d22-ff1c73-9c49db-597ee6
    bg_color='#303030'
    best_color='#FD5D22'
    avg_color='#597EE6'
    # avg_color='#9C49DB'
    fg_color='#F9E9EC'

    # choose a backend before importing pyplot
    if not show:
        mpl.use('Agg')  # non-interactive backend for headless saving

    import matplotlib.pyplot as plt

    generations = []
    best = []
    average = []

    with open(data_filename, newline='') as csvfile:
        reader = csv.DictReader(csvfile, delimiter=';')
        for row in reader:
            generations.append(int(row['generation']))
            best.append(float(row['best_fitness']))
            average.append(float(row['average_fitness']))

    last_gen = generations[-1]

    plt.figure(facecolor=bg_color)
    ax = plt.gca()
    ax.set_facecolor(bg_color)
    ax.tick_params(colors=fg_color)
    ax.spines['bottom'].set_color(fg_color)
    ax.spines['top'].set_color(fg_color)
    ax.spines['left'].set_color(fg_color)
    ax.spines['right'].set_color(fg_color)

    plt.plot(generations, best, label='Best', color=best_color)
    plt.plot(generations, average, label='Average', color=avg_color)
    plt.xlabel('Generation', color=fg_color)
    plt.ylabel('Score', color=fg_color)
    plt.title(f'Gen: {last_gen + 1}', color=fg_color)
    plt.legend(facecolor=bg_color, edgecolor=fg_color, labelcolor=fg_color)
    plt.grid(True, color=fg_color, alpha=0.3)
    plt.ylim(bottom=0)
    
    plt.savefig(plot_filename, dpi=300, bbox_inches='tight', facecolor=bg_color)
    print(f"Saved plot to {plot_filename}")

    if show:
        plt.show()
    plt.close()  # ensure the figure is closed

if __name__ == "__main__":
    import sys
    # pass --s on the command line to display the plot interactively
    show_flag = '--s' in sys.argv
    plot_csv('data/genome_fitness.csv', 'data/plot.png', show=show_flag)
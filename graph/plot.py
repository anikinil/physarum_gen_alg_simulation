import csv
import os
import matplotlib as mpl

def plot_csv(data_filename, plot_filename, show=False):

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

    plt.figure()
    plt.plot(generations, best, label='Best')
    plt.plot(generations, average, label='Average')
    plt.xlabel('Generation')
    plt.ylabel('Score')
    plt.title(f'Gen: {last_gen + 1}')
    plt.legend()
    plt.grid(True)
    plt.ylim(bottom=0)
    
    plt.savefig(plot_filename, dpi=300, bbox_inches='tight')
    print(f"Saved plot to {plot_filename}")

    if show:
        plt.show()
    plt.close()  # ensure the figure is closed

if __name__ == "__main__":
    import sys
    # pass --s on the command line to display the plot interactively
    show_flag = '--s' in sys.argv
    plot_csv('data/genome_fitness.csv', 'data/plot.png', show=show_flag)

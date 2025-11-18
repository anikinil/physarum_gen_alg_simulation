import csv
import os

import matplotlib.pyplot as plt
def plot_csv(filename):

    generations = []
    best = []
    average = []

    with open(filename, newline='') as csvfile:

        reader = csv.DictReader(csvfile, delimiter=';')

        for row in reader:
            generations.append(int(row['generation']))
            best.append(float(row['best_fitness']))
            average.append(float(row['average_fitness']))

    plt.plot(generations, best, label='Best')
    plt.plot(generations, average, label='Average')
    plt.xlabel('Generation')
    plt.ylabel('Score')
    plt.title('Genetic Algorithm Progress')
    plt.legend()
    plt.grid(True)
    plt.ylim(bottom=0)
    
    png_path = os.path.splitext(filename)[0] + '.png'
    plt.savefig(png_path, dpi=300, bbox_inches='tight')
    print(f"Saved plot to {png_path}")
    
    plt.show()

if __name__ == "__main__":
    plot_csv('data/genome_fitness.csv')
    
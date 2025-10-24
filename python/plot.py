import csv

import matplotlib.pyplot as plt

def plot_csv(filename):
    generations = []
    best = []
    average = []

    with open(filename, newline='') as csvfile:
        reader = csv.DictReader(csvfile, delimiter=';')
        for row in reader:
            generations.append(int(row['generation']))
            best.append(float(row['best']))
            average.append(float(row['average']))

    fig, ax1 = plt.subplots()
    color1 = 'tab:blue'
    ax1.set_xlabel('Generation')
    ax1.set_ylabel('Best', color=color1)
    l1, = ax1.plot(generations, best, color=color1, label='Best')
    ax1.tick_params(axis='y', labelcolor=color1)

    ax2 = ax1.twinx()
    color2 = 'tab:orange'
    ax2.set_ylabel('Average', color=color2)
    l2, = ax2.plot(generations, average, color=color2, label='Average')
    ax2.tick_params(axis='y', labelcolor=color2)

    # lines = [l1, l2]
    # labels = [l.get_label() for l in lines]
    # ax1.legend(lines, labels, loc='best')
    plt.title('Genetic Algorithm Progress')
    ax1.grid(True)
    fig.tight_layout()
    plt.show()

if __name__ == "__main__":
    plot_csv('../cpp/fitness_history.csv')
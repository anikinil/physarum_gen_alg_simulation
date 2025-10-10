from physarum import World, Food, Cell, NUM_STATES, NUM_ACTIONS
import math
import random
import pickle
from copy import deepcopy

from animation import animate


NUM_GENS = 200
NUM_INDIVIDUALS = 8
FITTEST_RATE = 0.2
NUM_FITTEST = round(NUM_INDIVIDUALS * FITTEST_RATE)

MUTATED_RATE = 0.7 * FITTEST_RATE
NUM_MUTATED = round(NUM_INDIVIDUALS * MUTATED_RATE)

MUTATION_RATE = 0.5
NUM_MUTATIONS = round(NUM_STATES * MUTATION_RATE)

NUM_STEPS = 90

DIRECTIONS = ['0', 'l', 'r', 'u', 'd']

# START_CELLS = [Cell(18, 25, energy=40)]
# FOOD = [Food(17, 15, 50)]
    
START_CELLS = [Cell(20, 15, energy=60)]
FOOD = [Food(30, 20, 10), Food(35, 25, 10), Food(10, 30, 10), Food(40, 10, 10), Food(5, 5, 10)]


def run():

    worlds = []
    
    for _ in range(NUM_INDIVIDUALS):
        worlds.append(World(cells=deepcopy(START_CELLS), food=deepcopy(FOOD), steps=NUM_STEPS))
        
    fitness_history = []
    sorted = []
    for gen in range(NUM_GENS):
        print(f"Gen {gen+1} - average: {fitness_history[-1] if fitness_history else 'N/A'}, best: {str(sorted[0].fitness) if len(sorted) > 0 else 'N/A'}")

        for i, world in enumerate(worlds):
            last_world_state = world.run()
            fitness = total_cells_fitness(last_world_state)
            print(f"    Ind {i+1}: Fitness: {round(fitness, 3)}")
            world.fitness = fitness

        sorted = sort_by_fitness(worlds)
        average_fitness = round(sum(sorted[i].fitness for i in range(len(sorted))) / len(sorted), 3)
        fitness_history.append(average_fitness)
        save_rules(sorted[0].rules)
        if gen < NUM_GENS - 1:
            worlds = crossover(sorted)
        else:
            best_world = World(cells=deepcopy(START_CELLS), food=deepcopy(FOOD), rules=sorted[0].rules, steps=NUM_STEPS)
            animate(best_world)
            plot_fitness_history(fitness_history)
            

def sort_by_fitness(worlds):
    return sorted(worlds, key=lambda w: w.fitness, reverse=True)

def total_cells_fitness(last_world_state):
    return len(last_world_state.cells)

def total_energy_fitness(last_world_state):
    return sum([cell.energy for cell in last_world_state.cells])

def aspect_ratio_fitness(last_world_state):
    min_x = min(cell.x for cell in last_world_state.cells)
    max_x = max(cell.x for cell in last_world_state.cells)
    min_y = min(cell.y for cell in last_world_state.cells)
    max_y = max(cell.y for cell in last_world_state.cells)
    return abs(abs(max_x - min_x) - abs(max_y - min_y))

def center_of_mass_fitness(last_world_state):
    cells = last_world_state.cells
    n = len(cells)
    sum_x = sum(cell.x for cell in cells)
    sum_y = sum(cell.y for cell in cells)
    avg_x = sum_x / n
    avg_y = sum_y / n
    max_dist_sq = max((cell.x - avg_x) ** 2 + (cell.y - avg_y) ** 2 for cell in cells)
    return -math.sqrt(max_dist_sq)

def food_proximity_fitness(last_world_state):
    min_dists = []
    if len(last_world_state.food) == 0:
        return 0
    for food in last_world_state.food:
        dists = []
        for cell in last_world_state.cells:
            dists.append(math.sqrt((cell.x - food.x)**2+(cell.y - food.y)**2))
        min_dists.append(min(dists))
    return -min(min_dists)

def custom_fitness(last_world_state):

    cells = last_world_state.cells
    if not cells:
        return 0

    # Average x position of cells
    avg_x_cells = sum(cell.x for cell in cells) / len(cells)

    # Average x position weighted by energy
    total_energy = sum(cell.energy for cell in cells)
    if total_energy == 0:
        avg_y_energy = 0
    else:
        avg_y_energy = sum(cell.y * cell.energy for cell in cells) / total_energy

    return abs(avg_x_cells - avg_y_energy)

def crossover(sorted_worlds):

    fittest = sorted_worlds[:NUM_FITTEST]
    least_fit = sorted_worlds[NUM_FITTEST:]
    
    crossed = []
    for i in range(0, len(fittest), 2):
        parent1 = fittest[i]
        parent2 = fittest[i+1] if i+1 < len(fittest) else fittest[0]

        rand_indexes = list(range(NUM_STATES))
        random.shuffle(rand_indexes)
        indexes1 = rand_indexes[:NUM_STATES//2]
        indexes2 = rand_indexes[NUM_STATES//2:]
        
        rules1 = [0] * NUM_STATES
        for i in indexes1:
            rules1[i] = parent1.rules[i]
        for i in indexes2:
            rules1[i] = parent2.rules[i]

        rules2 = [0] * NUM_STATES
        for i in indexes1:
            rules2[i] = parent2.rules[i]
        for i in indexes2:
            rules2[i] = parent1.rules[i]

        child1 = World(cells=deepcopy(START_CELLS), food=deepcopy(FOOD), rules=rules1, steps=NUM_STEPS)
        child2 = World(cells=deepcopy(START_CELLS), food=deepcopy(FOOD), rules=rules2, steps=NUM_STEPS)

        crossed.append(child1)
        crossed.append(child2)

    return mutate(crossed + fittest[:min(NUM_FITTEST, NUM_INDIVIDUALS - NUM_FITTEST)] + least_fit[:min(NUM_INDIVIDUALS - NUM_FITTEST - NUM_FITTEST, len(least_fit))])

def mutate(worlds):

    mutated_idx = random.sample(range(len(worlds)), NUM_MUTATED) # select individuals to mutate

    for i in mutated_idx:
        world = worlds[i]
        mutation_indexes = random.sample(range(NUM_STATES), NUM_MUTATIONS) # select states to mutate
        for idx in mutation_indexes:
            actions = list(range(NUM_ACTIONS))
            actions.remove(world.rules[idx])
            world.rules[idx] = random.choice(actions)

    return worlds
    
def save_rules(rules):
        
    with open('fittest_rules.pkl', 'wb') as f:
        pickle.dump(rules, f)
        
def plot_fitness_history(fitness_history):
    import matplotlib.pyplot as plt

    plt.plot(range(1, len(fitness_history)+1), fitness_history)
    plt.xlabel('Generation')
    plt.ylabel('Best Fitness')
    plt.title('Fitness over Generations')
    plt.grid()
    plt.savefig('fitness_history.png')
    plt.show()
        

if __name__ == "__main__":
    run()

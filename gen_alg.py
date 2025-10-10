from physarum import World, NUM_STATES, NUM_ACTIONS, NUM_STEPS, FOOD, START_CELLS
import math
import random
import pickle
from copy import deepcopy

from animation import animate


NUM_GENS = 100
NUM_INDIVIDUALS = 60
FITTEST_RATE = 0.05
NUM_FITTEST = round(NUM_INDIVIDUALS * FITTEST_RATE)

MUTATED_RATE = 0.9 * FITTEST_RATE
NUM_MUTATED = round(NUM_INDIVIDUALS * MUTATED_RATE)

MUTATION_RATE = 1
NUM_MUTATIONS = round(NUM_STATES * MUTATION_RATE)

adjusted_number_of_mutations_history = []

def run():

    worlds = []
    
    for _ in range(NUM_INDIVIDUALS):
        worlds.append(World(cells=deepcopy(START_CELLS), food=deepcopy(FOOD), steps=NUM_STEPS))

    average_fitness_history = []
    best_fitness_history = []
    for gen in range(NUM_GENS):
        print()
        print(f"Gen {gen+1} - average: {average_fitness_history[-1] if len(average_fitness_history) > 0 else 'N/A'}, best: {str(best_fitness_history[-1]) if len(best_fitness_history) > 0 else 'N/A'}")

        for i, world in enumerate(worlds):
            last_world_state = world.run()
            fitness = total_cells_fitness(last_world_state)
            print(f"    Ind {i+1} - fitness: {round(fitness, 3)}")
            world.fitness = fitness

        sorted = sort_by_fitness(worlds)
        save_rules(sorted[0].rules)
        if gen < NUM_GENS - 1:
            worlds = crossover(sorted)
        else:
            best_world = World(cells=deepcopy(START_CELLS), food=deepcopy(FOOD), rules=sorted[0].rules, steps=NUM_STEPS)
            animate(best_world)
        
        average_fitness = round(sum(sorted[i].fitness for i in range(len(sorted))) / len(sorted), 3)
        average_fitness_history.append(average_fitness)
        best_fitness_history.append(sorted[0].fitness)
        save_fitness_history_plot(average_fitness_history, best_fitness_history)
        

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

    similarity_punishment = (sum(sorted_worlds[i].fitness for i in range(len(sorted_worlds))) / len(sorted_worlds)) / max(world.fitness for world in sorted_worlds)
    # print("Similarity punishment:", similarity_punishment)
    adjusted_num_mutations = round(NUM_MUTATIONS*similarity_punishment)
    adjusted_number_of_mutations_history.append(adjusted_num_mutations)
    # print("Adjusted number of mutations:", adjusted_num_mutations)
    return mutate(crossed, adjusted_num_mutations) + fittest[:min(NUM_FITTEST, NUM_INDIVIDUALS - NUM_FITTEST)] + mutate(least_fit[:min(NUM_INDIVIDUALS - NUM_FITTEST - NUM_FITTEST, len(least_fit))])

def mutate(worlds, adjusted_num_mutations=NUM_MUTATIONS):

    mutated_idx = random.sample(range(len(worlds)), min(NUM_MUTATED, len(worlds))) # select individuals to mutate

    for i in mutated_idx:
        world = worlds[i]
        mutation_indexes = random.sample(range(NUM_STATES), min(adjusted_num_mutations, NUM_STATES)) # select states to mutate
        for idx in mutation_indexes:
            actions = list(range(NUM_ACTIONS))
            actions.remove(world.rules[idx])
            world.rules[idx] = random.choice(actions)

    return worlds
    
def save_rules(rules):
    with open('fittest_rules.pkl', 'wb') as f:
        pickle.dump(rules, f)
    

def save_fitness_history_plot(average_fitness_history, best_fitness_history):
    import matplotlib.pyplot as plt

    plt.figure()
    plt.plot(range(1, len(average_fitness_history)+1), average_fitness_history, label='Average Fitness')
    plt.plot(range(1, len(best_fitness_history)+1), best_fitness_history, label='Best Fitness')
    plt.plot(
        range(1, len(adjusted_number_of_mutations_history)+1),
        adjusted_number_of_mutations_history,
        label='Adjusted Number of Mutations',
        linestyle=':'
    )
    plt.xlabel('Generation')
    plt.ylabel('Fitness')
    plt.title('Fitness over Generations')
    plt.grid()
    plt.legend(['Average Fitness', 'Best Fitness', 'Adjusted Number of Mutations'], loc='upper left')
    plt.savefig('fitness_history.png')
    plt.close()

if __name__ == "__main__":
    run()

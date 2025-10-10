from physarum import World, Food, Cell, NUM_STATES, NUM_ACTIONS
import math
import random
import pickle
from copy import deepcopy

from animation import animate


NUM_GENS = 100
NUM_INDIVIDUALS = 100
# FITTEST_RATE = 0.2
# NUM_FITTEST = round(NUM_INDIVIDUALS * FITTEST_RATE)

MUTATED_RATE = 0.8
NUM_MUTATED = round(NUM_INDIVIDUALS//2 * MUTATED_RATE)

MUTATION_RATE = 0.9
NUM_MUTATIONS = round(NUM_STATES * MUTATION_RATE)

NUM_STEPS = 200

DIRECTIONS = ['0', 'l', 'r', 'u', 'd']

# START_CELLS = [Cell(18, 25, energy=40)]
# FOOD = [Food(17, 15, 50)]
    
START_CELLS = [Cell(20, 15, energy=40)]
FOOD = [Food(40, 25, 50)]


def run():

    worlds = []
    
    for _ in range(NUM_INDIVIDUALS):
        worlds.append(World(cells=deepcopy(START_CELLS), food=deepcopy(FOOD), steps=NUM_STEPS))
        
    fitness_history = []
    for gen in range(NUM_GENS):
        print(f"Generation {gen+1}")
        
        for i, world in enumerate(worlds):
            # print(f"  Individual {i+1}")
            last_world_state = world.run()
            # fitness = food_proximity_fitness(last_world_state)
            fitness = total_energy_fitness(last_world_state) - total_cells_fitness(last_world_state) + area_fitness(last_world_state)
            # print(f"    Fitness: {round(fitness, 3)}")
            world.fitness = fitness

        fittest = get_k_fittest(worlds, NUM_INDIVIDUALS//2+2)
        average_fitness = sum(fittest[i].fitness for i in range(len(fittest))) / len(fittest)
        fitness_history.append(average_fitness)
        print(f"      Average fitness: {average_fitness}")
        save_rules(fittest[0].rules)
        if gen < NUM_GENS - 1:
            worlds = crossover(fittest)
        else:
            best_world = World(cells=deepcopy(START_CELLS), food=deepcopy(FOOD), rules=fittest[0].rules, steps=NUM_STEPS)
            animate(best_world)
            plot_fitness_history(fitness_history)

def get_k_fittest(worlds, k):
    worlds.sort(key=lambda w: w.fitness, reverse=True)
    return worlds[:k]

def total_cells_fitness(last_world_state):
    return len(last_world_state.cells)

def total_energy_fitness(last_world_state):
    return sum([cell.energy for cell in last_world_state.cells])
    
def area_fitness(last_world_state):

    min_x = min(cell.x for cell in last_world_state.cells)
    max_x = max(cell.x for cell in last_world_state.cells)
    min_y = min(cell.y for cell in last_world_state.cells)
    max_y = max(cell.y for cell in last_world_state.cells)
    return (max_x - min_x) * (max_y - min_y)

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

def crossover(fittest_worlds):

    crossed = []

    for i in range(0, len(fittest_worlds), 2):
        parent1 = fittest_worlds[i]
        parent2 = fittest_worlds[i+1] if i+1 < len(fittest_worlds) else fittest_worlds[0]

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

    return mutate(crossed) + fittest_worlds[:NUM_INDIVIDUALS - len(crossed)]

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
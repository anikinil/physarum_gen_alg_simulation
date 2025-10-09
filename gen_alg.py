from physarum import World, Food, Cell, Rules
import random
import pickle

from animation import animate


NUM_GENS = 5
NUM_INDIVIDUALS = 10
FITTEST_RATE = 0.2
NUM_FITTEST = round(NUM_INDIVIDUALS * FITTEST_RATE)

MUTATED_RATE = 0.1
NUM_MUTATED = round(NUM_INDIVIDUALS * MUTATED_RATE)

# MUTATION_RATE = 0.01
# NUM_MUTATIONS = round(16*(5**4)*10 * MUTATION_RATE)
NUM_MUTATIONS = 5

NUM_STEPS = 200

DIRECTIONS = ['l', 'r', 'u', 'd', '0']

def run():

    start_cells = [Cell(10, 10, energy=20)]
    start_cells_copy = start_cells.copy()
    food = [Food(15, 15, 50)]
    
    worlds = []
    
    for _ in range(NUM_INDIVIDUALS):
        start_rules = Rules(
            # 16 neighbor states, 5**4 memory states, 10 energy levels
            growth         = [[random.choices(['l', 'r', 'u', 'd', '0'], weights=[1, 1, 1, 1, 1], k=10)]*(5**4)]*16,
            energy_passing = [[random.choices(['l', 'r', 'u', 'd', '0'], weights=[1, 1, 1, 1, 1], k=10)]*(5**4)]*16,
            signal_passing = [[random.choices(['l', 'r', 'u', 'd', '0'], weights=[1, 1, 1, 1, 1], k=10)]*(5**4)]*16
        )
        worlds.append(World(cells=start_cells, food=food, rules=start_rules, steps=NUM_STEPS))
        
    for gen in range(NUM_GENS):
        print(f"Generation {gen+1}")
        
        for i, world in enumerate(worlds):
            print(f"  Individual {i+1}")
            last_world_state = world.run()
            fitness = total_cells_fitness(last_world_state)
            print(f"    Fitness: {fitness}")
            world.fitness = fitness
        
        fittest = get_fittest(worlds, NUM_FITTEST)
        print(f"  Best fitness: {fittest[0].fitness}")
        if gen < NUM_GENS - 1:
            worlds = crossover(fittest)
        else:
            print("saving:")
            print(fittest[0].fitness)
            best_world = World(cells=start_cells_copy, food=food, rules=fittest[0].rules, steps=NUM_STEPS)
            animate(best_world)
            save_rules(fittest[0].rules)

def get_fittest(worlds, k):
    worlds.sort(key=lambda w: w.fitness, reverse=True)
    return worlds[:k]

def total_cells_fitness(last_world_state):
    return len(last_world_state.cells)

def total_energy_fitness(last_world_state):
    print(len(last_world_state.cells))
    print(len([cell.energy for cell in last_world_state.cells]))
    return sum([cell.energy for cell in last_world_state.cells])
    

def area_fitness(last_world_state):

    min_x = min(cell.x for cell in last_world_state.cells)
    max_x = max(cell.x for cell in last_world_state.cells)
    min_y = min(cell.y for cell in last_world_state.cells)
    max_y = max(cell.y for cell in last_world_state.cells)
    return (max_x - min_x) * (max_y - min_y)

# 3 * 100 000

def crossover(worlds):
    
    new_worlds = []
    
    j = 0
    for _ in range(NUM_INDIVIDUALS):
        if j % NUM_FITTEST == 0: j = 0
        new_worlds.append(worlds[j])
        j += 1
        
    return mutate(new_worlds)
    
def mutate(worlds):
    
    mutated = random.sample(worlds, NUM_MUTATED)

    for m in mutated:
        worlds.remove(m)

    for m in mutated:
        rand_growth_rule_idxs = random.sample(range(16), NUM_MUTATIONS)
        rand_energy_passing_rule_idxs = random.sample(range(5**4), NUM_MUTATIONS)
        rand_signal_passing_rule_idxs = random.sample(range(10), NUM_MUTATIONS)
        
        for i in range(NUM_MUTATIONS):
            
            # print(set(DIRECTIONS))
            # print(m.rules.growth[rand_growth_rule_idxs[i]])
            # print(set(m.rules.growth[rand_growth_rule_idxs[i]]))
            # print(set(DIRECTIONS) - set(m.rules.growth[rand_growth_rule_idxs[i]]))
            
            dir = m.rules.growth[rand_growth_rule_idxs[i]][rand_energy_passing_rule_idxs[i]][rand_signal_passing_rule_idxs[i]]
            m.rules.growth[rand_growth_rule_idxs[i]][rand_energy_passing_rule_idxs[i]][rand_signal_passing_rule_idxs[i]] = random.choice(list(set(DIRECTIONS) - set(dir)))
            
            dir = m.rules.energy_passing[rand_growth_rule_idxs[i]][rand_energy_passing_rule_idxs[i]][rand_signal_passing_rule_idxs[i]]
            m.rules.energy_passing[rand_growth_rule_idxs[i]][rand_energy_passing_rule_idxs[i]][rand_signal_passing_rule_idxs[i]] = random.choice(list(set(DIRECTIONS) - set(dir)))
            
            dir = m.rules.signal_passing[rand_growth_rule_idxs[i]][rand_energy_passing_rule_idxs[i]][rand_signal_passing_rule_idxs[i]]
            m.rules.signal_passing[rand_growth_rule_idxs[i]][rand_energy_passing_rule_idxs[i]][rand_signal_passing_rule_idxs[i]] = random.choice(list(set(DIRECTIONS) - set(dir)))
            
    return worlds + mutated
    
def save_rules(rules):
        
    with open('fittest_rules.pkl', 'wb') as f:
        pickle.dump(rules, f)
        

if __name__ == "__main__":
    run()
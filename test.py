import pickle
from animation import animate
from copy import deepcopy
from physarum import World, NUM_STEPS, START_CELLS, FOOD, NUM_STATES

# with open('fittest_rules.pkl', 'rb') as f:
#     rules = pickle.load(f)
    
rules = [4] * (NUM_STATES)
    
world = World(deepcopy(START_CELLS), deepcopy(FOOD), rules, NUM_STEPS)

last_state = world.run()

print(len(last_state.cells))

animate(world)

print(len(last_state.cells))
from copy import deepcopy
import random

DIRS = ['0', 'l', 'r', 'u', 'd']
NUM_DIRS = len(DIRS)
NUM_ACTIONS = 5*5
NUM_STATES = 16*16
LEN_STATE_CODE = 2*4

MIN_ENERGY = 1
MAX_ENERGY_PORTION = 3

class Food:
    def __init__(self, x, y, energy=5):
        self.x = x
        self.y = y
        self.energy = energy
        

class Cell:
    def __init__(self, x, y, energy, energy_dir='0'):
        self.x = x
        self.y = y
        self.energy = energy
        self.energy_dir = energy_dir
    def copy(self):
        return Cell(self.x, self.y, self.energy, self.energy_dir)


NUM_STEPS = 200
START_CELLS = [Cell(20, 15, energy=60)]
FOOD = [Food(23, 18, 80), Food(35, 25, 80), Food(10, 30, 80), Food(40, 10, 80), Food(5, 5, 80)]


TRANSFERABLE_FOOD_ENERGY = 2

def encode_state(neighbors, energy):
    idx = 0
    for bit in neighbors + energy:
        idx = (idx << 1) | bit
    return idx

def decode_state(state_code):
    states = tuple((state_code >> i) & 1 for i in reversed(range(LEN_STATE_CODE)))
    return states[slice(4)], states[slice(4, LEN_STATE_CODE)]

def encode_action(growth_dir, energy_dir):
    growth_dir_idx = DIRS.index(growth_dir)
    energy_dir_idx = DIRS.index(energy_dir)
    return growth_dir_idx * NUM_DIRS + energy_dir_idx

def decode_action(action_code):
    growth_dir_idx = action_code // NUM_DIRS
    energy_dir_idx = action_code % NUM_DIRS
    return (DIRS[growth_dir_idx], DIRS[energy_dir_idx])

def get_next_action(rules, neigh_state, energy_state):
    return decode_action(rules[encode_state(neigh_state, energy_state)])

def get_randomized_rules():
    weights_growth = [1, 1, 1 ,1, 1] # 0, l, r, u, d
    weights_energy = [1, 1, 1 ,1, 1] # 0, l, r, u, d
    weights_total = [w1 * w2 for w1 in weights_growth for w2 in weights_energy]
    return random.choices(range(NUM_ACTIONS), weights=weights_total, k=NUM_STATES)


class World:

    def __init__(self, cells, food, rules=None, steps=100):
        self.cells = cells
        self.food = food
        self.rules = rules if rules is not None else get_randomized_rules()
        self.steps = steps
        self.fitness = 0

    # optimize: reuse neighbor information on other neighboring cells (make a whole pass through the grid)
    def get_neighborhood_state(self, cell):
        # check where the neighbors are
        neighbor_state = (
            1 if any(c.x == cell.x-1 and c.y == cell.y for c in self.cells) else 0, # left
            1 if any(c.x == cell.x+1 and c.y == cell.y for c in self.cells) else 0, # right
            1 if any(c.x == cell.x and c.y == cell.y-1 for c in self.cells) else 0, # up
            1 if any(c.x == cell.x and c.y == cell.y+1 for c in self.cells) else 0, # down
        )
        # check where energy is coming from
        energy_state = (
            1 if any(c.x == cell.x-1 and c.y == cell.y and c.energy_dir == 'r' for c in self.cells) else 0, # left
            1 if any(c.x == cell.x+1 and c.y == cell.y and c.energy_dir == 'l' for c in self.cells) else 0, # right
            1 if any(c.x == cell.x and c.y == cell.y-1 and c.energy_dir == 'd' for c in self.cells) else 0, # up
            1 if any(c.x == cell.x and c.y == cell.y+1 and c.energy_dir == 'u' for c in self.cells) else 0, # down
        )
        return neighbor_state, energy_state
    
    def update_growth(self):
        updated_cells = deepcopy(self.cells)
        for cell in self.cells:
            neighbor_state, energy_state = self.get_neighborhood_state(cell)

            if cell.energy < MIN_ENERGY * 2: continue

            growth_dir, _ = get_next_action(self.rules, neighbor_state, energy_state)
            
            target_x, target_y = cell.x, cell.y
            if growth_dir == '0': continue # no growth
            elif growth_dir == 'l': target_x -= 1
            elif growth_dir == 'r': target_x += 1
            elif growth_dir == 'u': target_y -= 1
            elif growth_dir == 'd': target_y += 1
            
            if not any(c.x == target_x and c.y == target_y for c in updated_cells):
                if not any(f.x == target_x and f.y == target_y for f in self.food):
                    new_cell = Cell(target_x, target_y, cell.energy/2)
                    updated_cells.append(new_cell)
                    for i, c in enumerate(updated_cells):
                        if c.x == cell.x and c.y == cell.y:
                            updated_cells[i].energy /= 2
                            break

        self.cells = updated_cells

    def update_energy(self):
        updated_cells = deepcopy(self.cells)
        for cell in self.cells:
            neighbor_state, energy_state = self.get_neighborhood_state(cell)

            transferable_energy = cell.energy / MAX_ENERGY_PORTION          
            # if cell.energy < 0:
                # print("starting with", cell.x, cell.y, "with energy", cell.energy, "can transfer", transferable_energy)

            if cell.energy - transferable_energy < MIN_ENERGY:
                continue

            _, energy_dir = get_next_action(self.rules, neighbor_state, energy_state)

            target_x, target_y = cell.x, cell.y
            if energy_dir == '0': continue # no energy transfer
            elif energy_dir == 'l': target_x -= 1
            elif energy_dir == 'r': target_x += 1
            elif energy_dir == 'u': target_y -= 1
            elif energy_dir == 'd': target_y += 1

            if any(c.x == target_x and c.y == target_y for c in self.cells):
                # print("transferring energy")
                for i, c in enumerate(updated_cells):
                    if c.x == cell.x and c.y == cell.y:
                        # print("from cell", c.x, c.y, "with energy", c.energy)
                        updated_cells[i].energy -= transferable_energy
                        updated_cells[i].energy_dir = energy_dir
                        break
                for i, c in enumerate(updated_cells):
                    if c.x == target_x and c.y == target_y:
                        # print("to cell", c.x, c.y, "with energy", c.energy)
                        updated_cells[i].energy += transferable_energy
                        break
                # for i, c in enumerate(self.cells):
                #     if c.x == cell.x and c.y == cell.y:
                #         self.cells[i].energy -= transferable_energy

        self.cells = updated_cells

    def update_food(self):
        updated_cells = deepcopy(self.cells)
        updated_food = deepcopy(self.food)
        for food in self.food:
            for cell in self.cells:
                # ! proximity defined differently here !
                if abs(cell.x - food.x) <= 1 and abs(cell.y - food.y) <= 1:
                    for f in updated_food:
                        if f.x == food.x and f.y == food.y:
                            f.energy -= min(TRANSFERABLE_FOOD_ENERGY, f.energy)
                            if f.energy <= 0:
                                updated_food.remove(f)
                            break
                    for i, c in enumerate(self.cells):
                        if c.x == cell.x and c.y == cell.y:
                            updated_cells[i] = deepcopy(c)
                            updated_cells[i].energy += min(MAX_ENERGY_PORTION, updated_cells[i].energy)
                            break
                    break

        self.cells = updated_cells
        self.food = updated_food

    def run(self):
        for _ in range(self.steps):
            self.update_growth()
            self.update_food()
            self.update_energy()
        return self
        
    def save_state(self):
        pass

    def draw(self):
        for cell in self.cells:
            cell.draw()
        for food in self.food:
            food.draw()


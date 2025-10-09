import itertools


NEIGHBOR_STATE_LOOKUP = {
    0: {'l': 0, 'r': 0, 'u': 0, 'd': 0},  # no neighbors
    1: {'l': 1, 'r': 0, 'u': 0, 'd': 0},  # left
    2: {'l': 0, 'r': 1, 'u': 0, 'd': 0},  # right
    3: {'l': 0, 'r': 0, 'u': 1, 'd': 0},  # above
    4: {'l': 0, 'r': 0, 'u': 0, 'd': 1},  # below
    5: {'l': 1, 'r': 1, 'u': 0, 'd': 0},  # left, right
    6: {'l': 1, 'r': 0, 'u': 1, 'd': 0},  # left, above
    7: {'l': 1, 'r': 0, 'u': 0, 'd': 1},  # left, below
    8: {'l': 0, 'r': 1, 'u': 1, 'd': 0},  # right, above
    9: {'l': 0, 'r': 1, 'u': 0, 'd': 1},  # right, below
    10: {'l': 0, 'r': 0, 'u': 1, 'd': 1}, # above, below
    11: {'l': 1, 'r': 1, 'u': 1, 'd': 0}, # left, right, above
    12: {'l': 1, 'r': 1, 'u': 0, 'd': 1}, # left, right, below
    13: {'l': 1, 'r': 0, 'u': 1, 'd': 1}, # left, above, below
    14: {'l': 0, 'r': 1, 'u': 1, 'd': 1}, # right, above, below
    15: {'l': 1, 'r': 1, 'u': 1, 'd': 1}, # left, right, above, below
}

MEMORY_LENGTH = 4

MEMORY_STATE_LOOKUP = {}
symbols = ['0', 'l', 'r', 'u', 'd']
for combo in itertools.product(symbols, repeat=MEMORY_LENGTH):
    MEMORY_STATE_LOOKUP[len(MEMORY_STATE_LOOKUP)] = list(combo)
    

class Rules:
    def __init__(self, growth, energy_passing, signal_passing):
        self.growth = growth
        self.energy_passing = energy_passing
        self.signal_passing = signal_passing
    
    def get_growth_dir(self, neighbors, memory, energy):
        return self.growth[neighbors][memory][energy]

    def get_energy_dir(self, neighbors, memory, energy):
        return self.energy_passing[neighbors][memory][energy]

    def get_signal(self, neighbors, memory, energy):
        return self.signal_passing[neighbors][memory][energy]


class Food:
    
    def __init__(self, x, y, energy=5):
        self.x = x
        self.y = y
        self.energy = energy
        

class Cell:

    def __init__(self, x, y, energy=10, memory=['0', '0', '0', '0']):
        self.x = x
        self.y = y
        self.energy = energy
        self.memory = memory

    def receive_signal(self, signal):
        self.memory.append(signal)
        if len(self.memory) > 4:
            self.memory.pop()

    def copy(self):
        return Cell(self.x, self.y, self.energy, self.memory.copy())


class World:

    def __init__(self, cells, food, rules, steps=1000):
        self.cells = cells
        self.food = food
        self.rules = rules
        self.steps = steps
        self.fitness = 0
    
    def set_fitness(self, fitness):
        self.fitness = fitness
        
    def get_fitness(self):
        return self.fitness
        
    # optimize: reuse neighbor information on other neighbouring cells (make a whole pass through the grid)
    def get_cell_state(self, cell):
        val = {'l': 0, 'r': 0, 'u': 0, 'd': 0}
        for other in self.cells:
            if other.x == cell.x - 1 and other.y == cell.y:
                val['l'] = 1
            elif other.x == cell.x + 1 and other.y == cell.y:
                val['r'] = 1
            elif other.x == cell.x and other.y == cell.y - 1:
                val['u'] = 1
            elif other.x == cell.x and other.y == cell.y + 1:
                val['d'] = 1
        neighbors = None
        for key, value in NEIGHBOR_STATE_LOOKUP.items():
            if value == val:
                neighbors = key
                break
        memory = None
        for key, value in MEMORY_STATE_LOOKUP.items():
            if value == cell.memory:
                memory = key
                break
        return {
            'neighbors': neighbors,
            'memory': memory,
            'energy': round(min(cell.energy, 9)) # cap energy state to 9
        }

    def update_growth(self):
        updated_cells = self.cells.copy()
        for cell in self.cells:
            cell_state = self.get_cell_state(cell)

            neigh_state = cell_state['neighbors']
            memory_state = cell_state['memory']
            energy_state = cell_state['energy']

            if cell.energy < 2:
                continue

            dir = self.rules.get_growth_dir(neigh_state, memory_state, energy_state)

            target_x, target_y = cell.x, cell.y
            if dir == 'l': target_x -= 1
            elif dir == 'r': target_x += 1
            elif dir == 'u': target_y -= 1
            elif dir == 'd': target_y += 1
            else: continue # '0': no growth
            
            if not any(c.x == target_x and c.y == target_y for c in self.cells):
                new_cell = Cell(target_x, target_y, cell.energy/2)
                updated_cells.append(new_cell)
                # cell.energy /= 2
                for i, c in enumerate(updated_cells):
                    if c.x == cell.x and c.y == cell.y:
                        # updated_cells[i] = c.copy()
                        updated_cells[i].energy /= 2
                        break

        self.cells = updated_cells
    
    def update_energy(self):
        updated_cells = self.cells.copy()
        for cell in self.cells:
            cell_state = self.get_cell_state(cell)
            
            energy_state = cell_state['energy']
            neigh_state = cell_state['neighbors']
            memory_state = cell_state['memory']
            
            if cell.energy < 1.5:
                continue

            dir = self.rules.get_energy_dir(neigh_state, memory_state, energy_state)

            target_x, target_y = cell.x, cell.y
            if dir == 'l': target_x -= 1
            elif dir == 'r': target_x += 1
            elif dir == 'u': target_y -= 1
            elif dir == 'd': target_y += 1
            else: continue # '0': no energy passed
            
            if any(c.x == target_x and c.y == target_y for c in self.cells):
                for i, c in enumerate(self.cells):
                    if c.x == target_x and c.y == target_y:
                        updated_cells[i] = c.copy()
                        updated_cells[i].energy += 0.5
                        c.energy += 0.5
                        break
                for i, c in enumerate(self.cells):
                    if c.x == cell.x and c.y == cell.y:
                        updated_cells[i] = c.copy()
                        updated_cells[i].energy -= 0.5
                        break
                
        self.cells = updated_cells

    def update_signals(self):
        for cell in self.cells:
            cell_state = self.get_cell_state(cell)

            neigh_state = cell_state['neighbors']
            memory_state = cell_state['memory']
            energy_state = cell_state['energy']

            dir = self.rules.get_signal(neigh_state, memory_state, energy_state)
            
            target_x, target_y = cell.x, cell.y
            if dir == 'l':
                target_x -= 1
            elif dir == 'r':
                target_x += 1
            elif dir == 'u':
                target_y -= 1
            elif dir == 'd':
                target_y += 1
            else: continue # '0': no signal passed
            
            for cell in self.cells:
                if cell.x == target_x and cell.y == target_y:
                    cell.receive_signal(dir)
                    # if dir == 'l':
                    #     pygame.draw.line(SCREEN, 'blue', 
                    #                        (cell.x*GRID_SIZE, cell.y*GRID_SIZE + GRID_SIZE*0.4),
                    #                        (cell.x*GRID_SIZE, cell.y*GRID_SIZE+GRID_SIZE*0.6), 6)
                    # elif dir == 'r':
                    #     pygame.draw.line(SCREEN, 'blue', 
                    #                        (cell.x*GRID_SIZE+GRID_SIZE, cell.y*GRID_SIZE + GRID_SIZE*0.4),
                    #                        (cell.x*GRID_SIZE+GRID_SIZE, cell.y*GRID_SIZE+GRID_SIZE*0.6), 6)
                    # elif dir == 'u':
                    #     pygame.draw.line(SCREEN, 'blue', 
                    #                        (cell.x*GRID_SIZE+GRID_SIZE*0.4, cell.y*GRID_SIZE),
                    #                        (cell.x*GRID_SIZE+GRID_SIZE*0.6, cell.y*GRID_SIZE), 6)
                    # elif dir == 'd':
                    #     pygame.draw.line(SCREEN, 'blue', 
                    #                        (cell.x*GRID_SIZE+GRID_SIZE*0.4, cell.y*GRID_SIZE+GRID_SIZE),
                    #                        (cell.x*GRID_SIZE+GRID_SIZE*0.6, cell.y*GRID_SIZE+GRID_SIZE), 6)

    def update_food(self):
        updated_cells = self.cells.copy()
        updated_food = self.food.copy()
        for cell in self.cells:
            for food in self.food:
                if abs(cell.x - food.x) <= 1 and abs(cell.y - food.y) <= 1:
                    cell.energy += 1
                    for f in updated_food:
                        if f.x == food.x and f.y == food.y:
                            f.energy -= 1
                            if f.energy <= 0:
                                updated_food.remove(f)
                            break
                    for i, c in enumerate(self.cells):
                        if c.x == cell.x and c.y == cell.y:
                            updated_cells[i] = c.copy()
                            updated_cells[i].energy += 1
                            break
                    break
        self.cells = updated_cells
        self.food = updated_food

    def run(self):
        for step in range(self.steps):
            if step % 10 == 0:
                self.update_growth()
            if step % 5 == 0:
                self.update_energy()
                self.update_food()
            self.update_signals()
        return self
        
    def save_state(self):
        pass

    def draw(self):
        for cell in self.cells:
            cell.draw()
        for food in self.food:
            food.draw()


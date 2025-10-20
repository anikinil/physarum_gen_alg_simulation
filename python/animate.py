import csv
import pygame
from physarum import World, Cell, Food

# genetic algorithm
NUM_GENERATIONS = 15
NUM_INDS = 200
NUM_STEPS = 200

# animation
CELL_SIZE = 40
SCREEN_WIDTH, SCREEN_HEIGHT = 2500, 1500
FPS = 10
FONT_SIZE = 30
COL_GRID = (50, 50, 50)

INITIAL_ENERGY = 10
 
def draw_grid(screen):
    for x in range(SCREEN_WIDTH//2, screen.get_width(), CELL_SIZE):
        pygame.draw.line(screen, COL_GRID, (x, 0), (x, screen.get_height()))
    for y in range(SCREEN_HEIGHT//2, screen.get_height(), CELL_SIZE):
        pygame.draw.line(screen, COL_GRID, (0, y), (screen.get_width(), y))

def draw_cell(screen, x, y, energy):
    color = (int(255*(0.1 + energy/10)), int(255*(0.1 + energy/10)), 0)
    # color = (255, (255//cell.energy), 255-(255//cell.energy))
    # color = 'yellow'
    rect = pygame.Rect(SCREEN_WIDTH//2 + x*CELL_SIZE, SCREEN_HEIGHT//2 + y*CELL_SIZE, CELL_SIZE, CELL_SIZE)
    # energy = pygame.Rect(self.x*CELL_SIZE+1, self.y*CELL_SIZE+1, CELL_SIZE-2, CELL_SIZE-2)
    try:
        pygame.draw.rect(screen, color, rect)
    except:
        pygame.draw.rect(screen, 'red', rect)
    
    # if energy >= 20:
    font = pygame.font.SysFont(None, FONT_SIZE)
    text = font.render(str(round(energy, 2)), True, "blue")
    screen.blit(text, (SCREEN_WIDTH//2 + x * CELL_SIZE + 5, SCREEN_HEIGHT//2 + y * CELL_SIZE + 5))

def draw_food(screen, x, y, energy):
        cell = pygame.Rect(SCREEN_WIDTH//2 + x*CELL_SIZE, SCREEN_HEIGHT//2 + y*CELL_SIZE, CELL_SIZE, CELL_SIZE)
        pygame.draw.rect(screen, 'purple', cell)
        font = pygame.font.SysFont(None, FONT_SIZE)
        text = font.render(str(energy), True, "black")
        screen.blit(text, (SCREEN_WIDTH//2 + x * CELL_SIZE + 5, SCREEN_HEIGHT//2 + y * CELL_SIZE + 5))

def draw_world_frame(screen, world_state):
    # TODO
    pass

def plot_fitness_history():
    # TODO access fitness history from cpp and plot it to a png file here using matplotlib
    pass

def get_world(gen):
    filename = "../cpp/best_rules.csv"
    rules = []
    foods = []
    with open(filename, newline='') as csvfile:
        reader = csv.DictReader(csvfile)
        for row in reader:
            if int(row['gen']) == gen:
                rules_str = row['rules'].strip()
                rules = [int(x) for x in rules_str.split()]
                if len(rules) != 256:
                    raise ValueError(f"Expected 256 rules for gen {gen}, got {len(rules)}")
                foods_str = row['foods'].strip()
                foods = [[Food(int(x), int(y), int(energy)) for x, y, energy in (f.split(" ") for f in foods_str.split(","))]]
                return World(cells=[Cell(0, 0, INITIAL_ENERGY)], rules=rules, foods=foods)
    raise KeyError(f"No rules found for generation {gen} in {filename}")

def animate():

    pygame.init()
    screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
    pygame.display.set_caption("Physarum")
    clock = pygame.time.Clock()

    world = get_world(gen=NUM_GENERATIONS-1)
    # use the physarum.h implementation to define the world here

    running = True
    step = 0
    while running:
        
        for event in pygame.event.get():
            if event.type == pygame.QUIT or (event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE):
                running = False
                
        screen.fill('black')

        # draw_grid(screen)

        draw_world_frame(screen, step) # also physarum.h logic here
        pygame.display.flip()  
        clock.tick(FPS)

        if step < NUM_STEPS:
            step += 1
            # use the physarum.h implementation to step the world here
            pygame.display.set_caption(f"Physarum - Step {step}/{NUM_STEPS}")
        elif step == NUM_STEPS:
            pygame.display.set_caption(f"Physarum - Finished {NUM_STEPS} steps")
            print("Animation finished")

    # Quit Pygame
    pygame.quit()

if __name__ == "__main__":


    # start gen alg with params here

    plot_fitness_history()
    animate()
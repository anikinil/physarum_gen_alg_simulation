import sys
import pickle
import pygame

from physarum import World, Food, Cell

CELL_SIZE = 40

SCREEN_WIDTH = 2500
SCREEN_HEIGHT = 1500

COL_GRID = (50, 50, 50)

FPS = 10

FONT_SIZE = 30


START_CELLS = [Cell(20, 15, energy=60)]
FOOD = [Food(30, 20, 10), Food(35, 25, 10), Food(10, 30, 10), Food(40, 10, 10), Food(12, 12, 10)]
NUM_STEPS = 90

def animate(world):

    pygame.init()
    screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
    pygame.display.set_caption("Physarum")
    clock = pygame.time.Clock()

    running = True
    count = 0
    while running:
        
        for event in pygame.event.get():
            if event.type == pygame.QUIT or (event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE):
                running = False
                
        screen.fill('black')

        draw_grid(screen)

        if count < world.steps:
            
            world.update_growth()
            draw_world(screen, world)
            pygame.display.flip()  
            clock.tick(FPS*3)
            
            world.update_energy()
            draw_world(screen, world)
            pygame.display.flip()
            clock.tick(FPS*3)
            
            world.update_food()
            draw_world(screen, world)
            pygame.display.flip()
            clock.tick(FPS*3)

        count += 1
        if count < world.steps:
            pygame.display.set_caption(f"Physarum - Step {count}/{world.steps}")
        elif count == world.steps:
            pygame.display.set_caption(f"Physarum - Finished {world.steps} steps")
            print("Animation finished")

    # Quit Pygame
    pygame.quit()
    
def draw_grid(screen):
    for x in range(0, screen.get_width(), CELL_SIZE):
        pygame.draw.line(screen, COL_GRID, (x, 0), (x, screen.get_height()))
    for y in range(0, screen.get_height(), CELL_SIZE):
        pygame.draw.line(screen, COL_GRID, (0, y), (screen.get_width(), y))

def draw_world(screen, world):

    for cell in world.cells:
        draw_cell(screen, cell)
    for food in world.food:
        draw_food(screen, food)

def draw_cell(screen, cell):
    color = (255-(255//cell.energy), 255-(255//cell.energy), 0)
    # color = (255, (255//cell.energy), 255-(255//cell.energy))
    # color = 'yellow'
    rect = pygame.Rect(cell.x*CELL_SIZE, cell.y*CELL_SIZE, CELL_SIZE, CELL_SIZE)
    # energy = pygame.Rect(self.x*CELL_SIZE+1, self.y*CELL_SIZE+1, CELL_SIZE-2, CELL_SIZE-2)
    try:
        pygame.draw.rect(screen, color, rect)
    except:
        pygame.draw.rect(screen, 'red', rect)
    # pygame.draw.rect(SCREEN, YELLOW, energy, 3)
    # font = pygame.font.SysFont(None, FONT_SIZE)
    # text = font.render(str(round(cell.energy, 2)), True, "blue")
    # screen.blit(text, (cell.x * CELL_SIZE + 5, cell.y * CELL_SIZE + 5))
    
    # font = pygame.font.SysFont(None, FONT_SIZE)
    # text = font.render(cell.energy_dir, True, "black")
    # screen.blit(text, (cell.x * CELL_SIZE + CELL_SIZE - 20, cell.y * CELL_SIZE + CELL_SIZE - 30))

def draw_food(screen, food):
        cell = pygame.Rect(food.x*CELL_SIZE, food.y*CELL_SIZE, CELL_SIZE, CELL_SIZE)
        pygame.draw.rect(screen, 'orange', cell)
        font = pygame.font.SysFont(None, FONT_SIZE)
        text = font.render(str(food.energy), True, "black")
        screen.blit(text, (food.x * CELL_SIZE + 5, food.y * CELL_SIZE + 5))

if __name__ == "__main__":
    

    if len(sys.argv) < 2:
        rules_file_path = 'fittest_rules.pkl'
    else:
        rules_file_path = sys.argv[1]

    with open(rules_file_path, 'rb') as f:
        rules = pickle.load(f)

    world = World(START_CELLS, FOOD, rules, NUM_STEPS)
    # world = World(cells=start_cells, food=food, steps=steps)

    animate(world)
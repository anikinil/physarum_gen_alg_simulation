import sys
import pickle
import pygame

from physarum import World, Food, Cell

CELL_SIZE = 60
GRID_SIZE = 25

COL_GRID = (50, 50, 50)

def animate(world):

    pygame.init()
    screen = pygame.display.set_mode((CELL_SIZE * GRID_SIZE, CELL_SIZE * GRID_SIZE))
    pygame.display.set_caption("Physarum")
    clock = pygame.time.Clock()

    running = True
    count = 0
    while running:
        
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
                
        screen.fill('black')

        draw_grid(screen)

        if count % 10 == 0:
            if count < world.steps:
                world.update_growth()
            draw_world(screen, world)
            pygame.display.flip()  
            clock.tick(300)

        if count % 5 == 0:
            if count < world.steps:
                world.update_energy()
                world.update_food()
            draw_world(screen, world)
            pygame.display.flip()
            clock.tick(300)

        if count < world.steps:
            world.update_signals()
        draw_world(screen, world)
        pygame.display.flip()
        clock.tick(300)
    
        count += 1
        if count < world.steps:
            print(f"Step {count}/{world.steps}")
        elif count == world.steps:
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
    # color = (255-(255//self.energy), 255-(255//self.energy), 0)
    color = (255, (255//cell.energy), 255-(255//cell.energy))
    # color = 'yellow'
    rect = pygame.Rect(cell.x*CELL_SIZE, cell.y*CELL_SIZE, CELL_SIZE, CELL_SIZE)
    # energy = pygame.Rect(self.x*CELL_SIZE+1, self.y*CELL_SIZE+1, CELL_SIZE-2, CELL_SIZE-2)
    pygame.draw.rect(screen, color, rect)
    # pygame.draw.rect(SCREEN, YELLOW, energy, 3)
    font = pygame.font.SysFont(None, 40)
    text = font.render(str(round(cell.energy, 2)), True, "black")
    screen.blit(text, (cell.x * CELL_SIZE + 5, cell.y * CELL_SIZE + 5))

def draw_food(screen, food):
        cell = pygame.Rect(food.x*CELL_SIZE, food.y*CELL_SIZE, CELL_SIZE, CELL_SIZE)
        pygame.draw.rect(screen, 'orange', cell)
        font = pygame.font.SysFont(None, 30)
        text = font.render(str(food.energy), True, "black")
        screen.blit(text, (food.x * CELL_SIZE + 5, food.y * CELL_SIZE + 5))

if __name__ == "__main__":
    
    start_cells = [Cell(10, 10, energy=20)]
    food = [Food(15, 15, 50)]
    steps = 200
    
    rules_file_path = sys.argv[1]
    
    with open(rules_file_path, 'rb') as f:
        rules = pickle.load(f)

    world = World(start_cells, food, rules, steps)

    animate(world)
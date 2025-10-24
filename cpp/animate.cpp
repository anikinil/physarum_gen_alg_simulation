#include <SFML/Graphics.hpp>
#include <fstream>
#include <sstream>
#include "gen_alg.hpp"
#include <numeric>

using namespace std;

int WIN_WIDTH = 2000;
int WIN_HEIGHT = 1500;

int CELL_SIZE = 30;

float FPS = 15.0f;

World readWorld(int gen) {

    ifstream file("best_individual.csv");
    if (!file) throw runtime_error("Could not open file");

    string line;
    getline(file, line); // skip header

    string selectedLine, lastLine;
    while (getline(file, line)) {
        lastLine = line;
        if (gen != -1) {
            string genStr = line.substr(0, line.find(';'));
            if (stoi(genStr) == gen) {
                selectedLine = line;
                break;
            }
        }
    }

    if (selectedLine.empty()) {
        cout << "Generation not specified or not found, using last line." << endl;
        selectedLine = lastLine;
    }

    stringstream ss(selectedLine);
    string genStr, rulesStr, foodsStr;
    getline(ss, genStr, ';');
    getline(ss, rulesStr, ';');
    getline(ss, foodsStr);

    vector<uint8_t> rules;
    rules.reserve(256);
    stringstream rulesStream(rulesStr);
    for (string token; getline(rulesStream, token, ' ');)
        if (!token.empty()) rules.push_back(static_cast<uint8_t>(stoi(token)));
    
    return World{{}, {}, rules};
}

void drawCells(sf::RenderWindow& window, vector<Cell>& cells) {
    int middleX = WIN_WIDTH / 2;
    int middleY = WIN_HEIGHT / 2;

    for (const Cell& cell : cells) {
        sf::RectangleShape shape(sf::Vector2f(CELL_SIZE - 2, CELL_SIZE - 2));
        shape.setPosition(middleX + cell.x * CELL_SIZE, middleY + cell.y * CELL_SIZE);
        int brightness = static_cast<int>(std::min(255.0f, std::max(50.0f, cell.energy * INITIAL_ENERGY / 10)));
        shape.setFillColor(sf::Color(brightness, brightness, 0));
        shape.setOutlineColor(sf::Color::Red);
        if (cell.energy >= MIN_GROWTH_ENERGY) shape.setOutlineThickness(3);
        window.draw(shape);
    }
}

void drawFoods(sf::RenderWindow& window, vector<Food>& foods) {
    int middleX = WIN_WIDTH / 2;
    int middleY = WIN_HEIGHT / 2;

    for (const Food& food : foods) {
        sf::RectangleShape shape(sf::Vector2f(CELL_SIZE - 2, CELL_SIZE - 2));
        shape.setPosition(middleX + food.x * CELL_SIZE, middleY + food.y * CELL_SIZE);
        int brightness = static_cast<int>(std::min(255.0f, std::max(100.0f, food.energy * 1.0f)));
        shape.setFillColor(sf::Color(brightness, 0, brightness));
        shape.setOutlineColor(sf::Color(brightness, 0, brightness));
        // shape.setOutlineThickness(1);

        window.draw(shape);
    }
}


void animate(World& world) {
    
    sf::RenderWindow window(sf::VideoMode(WIN_WIDTH, WIN_HEIGHT), "Physarum Polycephalum");

    sf::Clock clock;
    const float targetFrameTime = 1.f / FPS;

    int step = 0;
    while (window.isOpen()) {

        sf::Event event;
        while (window.pollEvent(event))
            if (event.type == sf::Event::Closed)
                window.close();

        window.clear();

        drawCells(window, world.cells);
        drawFoods(window, world.foods);

        window.display();

        
        if (step < NUM_STEPS) {
            window.setTitle("Physarum Polycephalum - Step: "
                + std::to_string(step)
                + "/" + std::to_string(static_cast<int>(NUM_STEPS))
                + " | Energy: " + std::to_string(accumulate(
                    world.cells.begin(), world.cells.end(), 0.0f,
                    [](float sum, const Cell& c){ return sum + c.energy; })));
            world = world.step();
            step++;
            sf::sleep(sf::seconds(targetFrameTime) - clock.getElapsedTime());
            clock.restart();
        } else {
            window.setTitle("Physarum Polycephalum - Finished at step: " 
                + std::to_string(step)
                + "/" + std::to_string(static_cast<int>(NUM_STEPS))
                + " | Energy: " + std::to_string(accumulate(
                    world.cells.begin(), world.cells.end(), 0.0f,
                    [](float sum, const Cell& c){ return sum + c.energy; })));
        }
    }
}

int main(int argc, char*argv[]) {

    int gen = atoi(argv[0]);

    World world = readWorld(gen);
    world.cells.push_back(Cell{0, 0, INITIAL_ENERGY, 'n'});
    vector<Food> newFoods = getRandomizedFood();
    world.foods.insert(world.foods.end(), newFoods.begin(), newFoods.end());

    // cout << "Rules:" << endl;
    // int count = 0;
    // for (int rule : world.rules) {
    //     cout << count << ": " << static_cast<int>(rule) << ", ";
    //     count++;
    // }
    // cout << endl;

    // cout << "Foods:" << endl;
    // for (const Food& food : world.foods) {
    //     cout << "Food at (" << food.x << ", " << food.y << ") with energy " << food.energy << endl;
    // }

    // cout << "Cells:" << endl;
    // for (const Cell& cell : world.cells) {
    //     cout << "Cell at (" << cell.x << ", " << cell.y << ") with energy " << cell.energy << endl;
    // }

    animate(world);

    return 0;
}


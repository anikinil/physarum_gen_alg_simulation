#include <SFML/Graphics.hpp>
#include <fstream>
#include <sstream>
#include "physarum.hpp"

using namespace std;

int WIN_WIDTH = 1500;
int WIN_HEIGHT = 1000;

int CELL_SIZE = 40;

int GEN = -1;

World readWorld() {
    ifstream file("best_individual.csv");
    if (!file) throw runtime_error("Could not open file");

    string line;
    getline(file, line); // skip header

    string selectedLine, lastLine;
    while (getline(file, line)) {
        lastLine = line;
        if (GEN != -1) {
            string genStr = line.substr(0, line.find(';'));
            if (stoi(genStr) == GEN) {
                selectedLine = line;
                break;
            }
        }
    }

    if (selectedLine.empty()) {
        cout << "GEN not specified or not found, using last line." << endl;
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

    vector<Food> foods;
    stringstream foodsStream(foodsStr);
    for (string token; getline(foodsStream, token, ',');) {
        if (token.empty()) continue;
        stringstream entry(token);
        int x, y; float e;
        entry >> x >> y >> e;
        foods.push_back({x, y, e});
    }

    return World{{Cell{0,}}, foods, rules};
}

void drawCells(sf::RenderWindow& window, vector<Cell>& cells) {
    int middleX = WIN_WIDTH / 2;
    int middleY = WIN_HEIGHT / 2;

    for (const Cell& cell : cells) {
        sf::RectangleShape shape(sf::Vector2f(CELL_SIZE - 2, CELL_SIZE - 2));
        shape.setPosition(middleX + cell.x * CELL_SIZE, middleY + cell.y * CELL_SIZE);
        shape.setFillColor(sf::Color::Yellow);
        shape.setOutlineColor(sf::Color::Yellow);
        // shape.setOutlineThickness(1);
        window.draw(shape);
    }
}

void drawFoods(sf::RenderWindow& window, vector<Food>& foods) {
    int middleX = WIN_WIDTH / 2;
    int middleY = WIN_HEIGHT / 2;

    for (const Food& food : foods) {
        sf::RectangleShape shape(sf::Vector2f(CELL_SIZE - 2, CELL_SIZE - 2));
        shape.setPosition(middleX + food.x * CELL_SIZE, middleY + food.y * CELL_SIZE);
        shape.setFillColor(sf::Color::Magenta);
        shape.setOutlineColor(sf::Color::Magenta);
        // shape.setOutlineThickness(1);
        window.draw(shape);
    }
}


void animate(World& world) {
    sf::RenderWindow window(sf::VideoMode(WIN_WIDTH, WIN_HEIGHT), "Physarum Polycephalum");

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event))
            if (event.type == sf::Event::Closed)
                window.close();

        window.clear();

        drawCells(window, world.cells);
        drawFoods(window, world.foods);

        window.display();
        
        world = world.step();
        cout << world.cells[0].x << " - " << world.cells[0].y << endl;
    }
}

int main() {

    World world = readWorld();

    cout << "Rules:" << endl;
    int count = 0;
    for (int rule : world.rules) {
        cout << count << ": " << static_cast<int>(rule) << ", ";
        count++;
    }
    cout << endl;

    cout << "Foods:" << endl;
    for (const Food& food : world.foods) {
        cout << "Food at (" << food.x << ", " << food.y << ") with energy " << food.energy << endl;
    }

    animate(world);

    return 0;
}


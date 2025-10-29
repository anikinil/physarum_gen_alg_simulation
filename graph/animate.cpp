#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>

#include "animate.hpp"
#include "physarum.hpp"


using namespace std;


vector<Frame> loadFrames() {
    vector<Frame> frames;
    ifstream file("data/animation_frames.csv");
    if (!file) throw runtime_error("Could not open file data/animation_frames.csv");

    string line;
    getline(file, line); // skip header

    int currentStep = -1;
    Frame currentFrame;

    while (getline(file, line)) {
        stringstream ss(line);
        string stepStr;
        getline(ss, stepStr, ',');
        int step = stoi(stepStr);
        if (step != currentStep) {
            if (currentStep != -1) frames.push_back(std::move(currentFrame));
            currentFrame = Frame();
            currentStep = step;
        }

        currentFrame.addObject(line);
    }

    if (currentStep != -1) frames.push_back(std::move(currentFrame));
    return frames;
}

void drawJunctions(sf::RenderWindow& window, const vector<JunctionVisual>& junctions) {
    for (const auto& junc : junctions) {
        sf::CircleShape shape(JUNCTION_RADIUS);
        shape.setFillColor(sf::Color::Yellow);
        shape.setPosition(WIN_WIDTH/2 + junc.x - JUNCTION_RADIUS, WIN_HEIGHT/2 + junc.y - JUNCTION_RADIUS);
        window.draw(shape);
    }
}

void drawTubes(sf::RenderWindow& window, const vector<TubeVisual>& tubes) {
    for (const auto& tube : tubes) {
        sf::Vector2f p1(WIN_WIDTH/2 + tube.x1, WIN_HEIGHT/2 + tube.y1);
        sf::Vector2f p2(WIN_WIDTH/2 + tube.x2, WIN_HEIGHT/2 + tube.y2);
        sf::Vector2f dir = p2 - p1;
        float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        float angle = std::atan2(dir.y, dir.x) * 180 / M_PI;

        sf::RectangleShape thickLine(sf::Vector2f(length, TUBE_THICKNESS)); // 3px thickness
        thickLine.setPosition(p1);
        thickLine.setRotation(angle);
        thickLine.setFillColor(sf::Color::Yellow);
        window.draw(thickLine);
    }
}


void drawFoodSources(sf::RenderWindow& window, const vector<FoodSourceVisual>& foodSources) {
    for (const auto& food : foodSources) {
        float radius = food.radius;
        sf::CircleShape shape(radius);
        shape.setFillColor(sf::Color::Magenta);
        shape.setPosition(WIN_WIDTH/2 + food.x - radius, WIN_HEIGHT/2 + food.y - radius);
        window.draw(shape);
    }
}

World readWorld(int gen) {

    ifstream file("data/genome_fitness.csv");
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
    string genStr, fitnessStr, genomeStr;
    getline(ss, genStr, ';');
    getline(ss, fitnessStr, ';');
    getline(ss, genomeStr, ';');

    vector<double> weights;
    weights.reserve((128 + 19) + (104 + 18));
    stringstream rulesStream(genomeStr);
    for (string token; getline(rulesStream, token, ' ');) {
        weights.push_back(static_cast<double>(stoi(token)));
    }

    vector<double> weights_a(weights.begin(), weights.begin() + 128 + 19);
    vector<double> weights_b;
    weights_b.assign(weights.begin() + 128 + 19, weights.end());

    Genome genome;
    genome.setGenomeValues(weights_a, weights_b);

    vector<unique_ptr<Junction>> junctions;
    junctions.push_back(make_unique<Junction>(Junction{0.0, 0.0, 100.0}));
    vector<unique_ptr<FoodSource>> foodSources;
    foodSources.push_back(make_unique<FoodSource>(FoodSource{50.0, 0.0, 100.0}));
    return World{genome, std::move(junctions), std::move(foodSources)};
}


int main() {

    int gen = -1;

    World world = readWorld(gen);

    // run and save simulation
    world.run(30, true);

    // Load saved generation
    vector<Frame> frames = loadFrames();

    size_t currentFrame = 0;
    bool paused = false;
    
    sf::Clock clock;
    const float targetFrameTime = 1.f / FPS;

    sf::RenderWindow window(sf::VideoMode(WIN_WIDTH, WIN_HEIGHT), "Physarum Simulation");
    window.setFramerateLimit(60);
    
    while (window.isOpen()) {
        sf::sleep(sf::seconds(targetFrameTime) - clock.getElapsedTime());
        clock.restart();
        
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
            window.close();
            else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Space)
                paused = !paused;
                if (paused && event.key.code == sf::Keyboard::Right)
                currentFrame = min(currentFrame + 1, frames.size()-1);
                if (paused && event.key.code == sf::Keyboard::Left)
                currentFrame = (currentFrame == 0 ? 0 : currentFrame - 1);
            }
        }
        
        window.clear();
        
        // Draw objects
        drawJunctions(window, frames[currentFrame].junctions);
        drawTubes(window, frames[currentFrame].tubes);
        drawFoodSources(window, frames[currentFrame].foodSources);
        
        // Display info text
        sf::Font font;
        // font.loadFromFile("Arial.ttf"); // or your font
        sf::Text info("Frame: " + std::to_string(currentFrame), font, 20);
        info.setFillColor(sf::Color::White);
        info.setPosition(10, 10);
        window.draw(info);

        window.display();

        if (!paused && currentFrame < frames.size()-1)
        currentFrame++;
    }
}


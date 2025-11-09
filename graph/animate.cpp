#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>

#include "animate.hpp"
#include "physarum.hpp"
#include "gen_alg.hpp"


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


void drawFoodSources(sf::RenderWindow& window, const vector<FoodSourceVisual>& foodSources) {
    for (const auto& food : foodSources) {
        float radius = food.radius;
        sf::CircleShape shape(radius);
        shape.setFillColor(sf::Color::White);
        shape.setPosition(WIN_WIDTH/2 + food.x - radius, WIN_HEIGHT/2 + food.y - radius);
        window.draw(shape);
    }
}

void drawJunctions(sf::RenderWindow& window, const vector<JunctionVisual>& junctions) {
    for (const auto& junc : junctions) {
        float radius = 1 + JUNCTION_RADIUS * junc.energy;
        sf::CircleShape shape(radius);
        shape.setFillColor(sf::Color::Yellow);
        shape.setPosition(WIN_WIDTH/2 + junc.x - radius, WIN_HEIGHT/2 + junc.y - radius);
        window.draw(shape);
    }
}

void drawTubes(sf::RenderWindow& window, const vector<TubeVisual>& tubes) {
    for (const auto& tube : tubes) {

        double thickness = 1 + TUBE_THICKNESS * tube.flowRate;

        sf::Vector2f p1(WIN_WIDTH/2 + tube.x1, WIN_HEIGHT/2 + tube.y1 - thickness / 2);
        sf::Vector2f p2(WIN_WIDTH/2 + tube.x2, WIN_HEIGHT/2 + tube.y2 - thickness / 2);
        sf::Vector2f dir = p2 - p1;
        float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        float angle = std::atan2(dir.y, dir.x) * 180 / M_PI;

        sf::RectangleShape thickLine(sf::Vector2f(length, thickness));
        thickLine.setPosition(p1);
        thickLine.setRotation(angle);
        thickLine.setFillColor(sf::Color::Yellow);
        window.draw(thickLine);
    }
}


vector<unique_ptr<FoodSource>> createRandomizedFoodSources() {
    vector<unique_ptr<FoodSource>> foodSources;
    for (int i = 0; i < NUM_FOOD_SOURCES; ++i) {
        double x = Random::uniform(-500.0, 500.0);
        double y = Random::uniform(-500.0, 500.0);
        double radius = Random::uniform(10.0, 50.0);
        double energy = Random::uniform(5.0, 15.0);
        foodSources.push_back(make_unique<FoodSource>(FoodSource{x, y, radius, energy}));
    }
    return foodSources;
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
    string genStr, bestFitnessStr, averageFitnessStr, genomeStr;
    getline(ss, genStr, ';');
    getline(ss, bestFitnessStr, ';');
    getline(ss, averageFitnessStr, ';');
    getline(ss, genomeStr, ';');

    // cout << "genomeStr: " << genomeStr << endl;

    vector<double> weights;
    int weights_size = 0;
    for (const auto& layer_dim : GROW_NET_DIMS) {
        int in = layer_dim.first;
        int out = layer_dim.second;
        weights_size += in * out + out;
    }
    for (const auto& layer_dim : FLOW_NET_DIMS) {
        int in = layer_dim.first;
        int out = layer_dim.second;
        weights_size += in * out + out;
    }
    weights.reserve(weights_size);
    stringstream rulesStream(genomeStr);
    for (string token; getline(rulesStream, token, ' ');) {
        // cout << "token: " << token << endl;
        weights.push_back(static_cast<double>(stod(token)));
    }

    int grow_net_size = 0;
    for (const auto& layer_dim : GROW_NET_DIMS) {
        int in = layer_dim.first;
        int out = layer_dim.second;
        grow_net_size += in * out + out;
    }

    vector<double> weights_a(weights.begin(), weights.begin() + grow_net_size);
    vector<double> weights_b;
    weights_b.assign(weights.begin() + grow_net_size, weights.end());

    Genome genome;
    // cout << "weights_a[14]: " << weights_a[14] << endl;
    // cout << "weights_b[14]: " << weights_b[14] << endl;
    genome.setGenomeValues(weights_a, weights_b);

    vector<unique_ptr<Junction>> junctions;
    junctions.push_back(make_unique<Junction>(Junction{0.0, 0.0, INITIAL_ENERGY}));
    vector<unique_ptr<FoodSource>> foodSources = createRandomizedFoodSources();
    // cout << "readWorld: " << genome.getGrowNetValues()[0][14] << endl;
    return World{genome, std::move(junctions), std::move(foodSources)};
}

int main() {

    int gen = -1;

    World world = readWorld(gen);

    // run and save simulation
    world.run(NUM_STEPS, true);

    // Load saved generation
    vector<Frame> frames = loadFrames();

    size_t currentFrame = 0;
    bool paused = false;
    
    sf::Clock clock;
    const float targetFrameTime = 1.f / FPS;

    sf::RenderWindow window(sf::VideoMode(WIN_WIDTH, WIN_HEIGHT), "Physarum Simulation");

    sf::View view(sf::FloatRect(0, 0, WIN_WIDTH, WIN_HEIGHT));
    window.setView(view);

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
            } else if (event.type == sf::Event::MouseWheelScrolled) {
                sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
                sf::Vector2f beforeZoom = window.mapPixelToCoords(pixelPos, view);
                float zoomFactor = (event.mouseWheelScroll.delta > 0) ? 0.9f : 1.1f;
                view.zoom(zoomFactor);
                sf::Vector2f afterZoom = window.mapPixelToCoords(pixelPos, view);
                sf::Vector2f offset = beforeZoom - afterZoom;
                view.move(offset);
                window.setView(view);
            }
        }

        window.setView(view);

        double totalEnergy = accumulate(
            frames[currentFrame].junctions.begin(),
            frames[currentFrame].junctions.end(), 0,
            [](int sum, const JunctionVisual& j) { return sum + j.energy; }
        );

        window.setTitle("Energy: " + std::to_string(totalEnergy) + " | Frame: " + std::to_string(currentFrame) + "/" + std::to_string(frames.size()-1));
        window.clear();
        
        // Draw objects
        drawFoodSources(window, frames[currentFrame].foodSources);
        drawJunctions(window, frames[currentFrame].junctions);
        drawTubes(window, frames[currentFrame].tubes);
        
        // // Display info text
        // sf::Font font;
        // // font.loadFromFile("Arial.ttf"); // or your font
        // sf::Text info("Frame: " + std::to_string(currentFrame), font, 20);
        // info.setFillColor(sf::Color::White);
        // info.setPosition(10, 10);
        // window.draw(info);

        window.display();

        if (!paused) {
            if (currentFrame < frames.size()-1)
                currentFrame++;
            else
                paused = true;
        }
    }
}


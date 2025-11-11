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

        // if (step < 30) {
        //     cout << "-------"  << endl;
        //     cout << "Step: " << currentStep << ", Junctions: " << currentFrame.junctions.size() << endl;
        //     for (size_t i = 0; i < currentFrame.junctions.size(); i++) {
        //         cout << " Junc " << i << ": ";
        //         cout << currentFrame.junctions[i].x << ", " << currentFrame.junctions[i].y << ", " << currentFrame.junctions[i].energy << endl;
        //     }
        //     double totalEnergy = 0.0;
        //     for (const auto& j : currentFrame.junctions) {
        //         totalEnergy += j.energy;
        //     }
        //     cout << "Total energy: " << totalEnergy << endl;
        // }
    }

    if (currentStep != -1) frames.push_back(std::move(currentFrame));
    return frames;
}


void drawFoodSources(sf::RenderWindow& window, const vector<FoodSourceVisual>& foodSources) {
    for (const auto& food : foodSources) {
        float radius = food.radius;
        sf::CircleShape area(radius);
        area.setFillColor(sf::Color(255, 130, 40));
        area.setPosition(WIN_WIDTH/2 + food.x - radius, WIN_HEIGHT/2 + food.y - radius);

        sf::CircleShape capacity(sqrt(food.energy/3.14));
        capacity.setFillColor(sf::Color::Transparent);
        capacity.setOutlineColor(sf::Color::Blue);
        capacity.setOutlineThickness(0.5);
        capacity.setPosition(WIN_WIDTH/2 + food.x - capacity.getRadius(), WIN_HEIGHT/2 + food.y - capacity.getRadius());
        window.draw(area);
        window.draw(capacity);
    }
}

void drawJunctions(sf::RenderWindow& window, const vector<JunctionVisual>& junctions) {
    for (const auto& junc : junctions) {

        int radius = 1 + JUNCTION_RADIUS_FACTOR * sqrt(junc.energy / 3.14); // scale radius with energy
        sf::Color color = sf::Color(255, 200, 40);
        if (radius == 1) color = sf::Color(180, 140, 20);
        sf::CircleShape shape(radius);
        shape.setFillColor(color);
        shape.setPosition(WIN_WIDTH/2 + junc.x - radius, WIN_HEIGHT/2 + junc.y - radius);
        window.draw(shape);
    }
}

void drawTubes(sf::RenderWindow& window, const vector<TubeVisual>& tubes) {
    for (const auto& tube : tubes) {
        float thickness = 1.f + TUBE_THICKNESS * static_cast<float>(tube.flowRate);
        sf::Vector2f p1(WIN_WIDTH/2 + tube.x1, WIN_HEIGHT/2 + tube.y1);
        sf::Vector2f p2(WIN_WIDTH/2 + tube.x2, WIN_HEIGHT/2 + tube.y2);
        sf::Vector2f dir = p2 - p1;
        float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        float angle = std::atan2(dir.y, dir.x) * 180.f / static_cast<float>(M_PI);

        sf::RectangleShape thickLine(sf::Vector2f(length, thickness));
        // center the rectangle on the tube centerline so thickness is symmetric about the junction centers
        thickLine.setOrigin(0.f, thickness * 0.5f);
        thickLine.setPosition(p1);
        thickLine.setRotation(angle);
        thickLine.setFillColor(sf::Color(255, 200, 40));
        window.draw(thickLine);
    }
}

vector<unique_ptr<FoodSource>> createRandomizedFoodSources() {
    vector<unique_ptr<FoodSource>> foodSources;
    for (int i = 0; i < NUM_FOOD_SOURCES-1; ++i) {
        double x = Random::uniform(-300.0, 300.0);
        double y = Random::uniform(-300.0, 300.0);
        double energy = Random::uniform(500.0, 1000.0);
        double radius = sqrt(energy/3.14); // area proportional to energy
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

    if (selectedLine.empty()) selectedLine = lastLine;

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
    view.zoom(DEFAULT_ZOOM);
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
            frames[currentFrame].junctions.end(), 0.0,
            [](double sum, const JunctionVisual& j) { return sum + j.energy; }
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


#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>

#include "animate.hpp"

using namespace std;

// --- hover utility ---
bool hoverCircle(float cx, float cy, float r, const sf::Vector2f& m) {
    float dx = m.x - cx;
    float dy = m.y - cy;
    return dx*dx + dy*dy <= r*r;
}

float distToSegment(sf::Vector2f p, sf::Vector2f a, sf::Vector2f b) {
    sf::Vector2f ab = b - a;
    float t = ((p - a).x * ab.x + (p - a).y * ab.y) / (ab.x*ab.x + ab.y*ab.y);
    t = std::max(0.f, std::min(1.f, t));
    sf::Vector2f proj = a + t * ab;
    float dx = p.x - proj.x;
    float dy = p.y - proj.y;
    return std::sqrt(dx*dx + dy*dy);
}

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
        sf::CircleShape area(radius);
        area.setFillColor(FOOD_SOURCE_COLOR);
        area.setPosition(WIN_WIDTH/2 + food.x - radius, WIN_HEIGHT/2 + food.y - radius);

        sf::CircleShape capacity(10.0 * sqrt(food.energy/3.14));
        capacity.setFillColor(sf::Color::Transparent);
        capacity.setOutlineColor(FOOD_SOURCE_CAPACITY_COLOR);
        capacity.setOutlineThickness(0.5);
        capacity.setPosition(WIN_WIDTH/2 + food.x - capacity.getRadius(), WIN_HEIGHT/2 + food.y - capacity.getRadius());
        window.draw(area);
        window.draw(capacity);
    }
}

void drawJunctions(sf::RenderWindow& window, const vector<JunctionVisual>& junctions) {
    for (const auto& junc : junctions) {

        int radius = 1 + JUNCTION_RADIUS_FACTOR * sqrt(junc.energy / 3.14); // scale radius with energy
        sf::Color color = JUNCTION_COLOR;
        if (radius == 1) color = DEPLETED_JUNCTION_COLOR;
        sf::CircleShape shape(radius);
        shape.setFillColor(color);
        shape.setPosition(WIN_WIDTH/2 + junc.x - radius, WIN_HEIGHT/2 + junc.y - radius);
        window.draw(shape);
    }
}

void drawTubes(sf::RenderWindow& window, const vector<TubeVisual>& tubes) {
    for (const auto& tube : tubes) {
        float thickness = MIN_TUBE_THICKNESS + TUBE_THICKNESS_FACTOR * static_cast<float>(tube.flowRate);
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
        thickLine.setFillColor(TUBE_COLOR);
        window.draw(thickLine);
    }
}

// vector<unique_ptr<FoodSource>> createRandomizedFoodSources() {
//     vector<unique_ptr<FoodSource>> foodSources;
//     for (int i = 0; i < NUM_FOOD_SOURCES-1; ++i) {
//         double x = Random::uniform(-300.0, 300.0);
//         double y = Random::uniform(-300.0, 300.0);
//         double energy = Random::uniform(500.0, 1000.0);
//         double radius = sqrt(energy/3.14); // area proportional to energy
//         foodSources.push_back(make_unique<FoodSource>(FoodSource{x, y, radius, energy}));
//     }
//     return foodSources;
// }

void drawLoadingScreen(sf::RenderWindow& window, const sf::Font& font) {
    sf::RectangleShape overlay(sf::Vector2f(WIN_WIDTH, WIN_HEIGHT));
    overlay.setFillColor(sf::Color(0, 0, 0, 100));
    window.draw(overlay);
    sf::Text loadingText("Loading...", font, 50);
    loadingText.setFillColor(sf::Color::White);
    loadingText.setPosition(WIN_WIDTH / 2 - loadingText.getLocalBounds().width / 2, WIN_HEIGHT / 2 - loadingText.getLocalBounds().height / 2);
    window.draw(loadingText);
    window.display();
}

void drawGrid(sf::RenderWindow& window) {
    const int gridSpacing = 50;
    sf::Color gridColor(200, 200, 200, 50);

    // float normalThickness = .4f;
    float normalThickness = 0.0f;
    float centerThickness = .8f;

    // Vertical lines
    for (int x = WIN_WIDTH / 2 % gridSpacing; x < WIN_WIDTH; x += gridSpacing) {
        float thickness = (x == WIN_WIDTH / 2) ? centerThickness : normalThickness;

        sf::RectangleShape line(sf::Vector2f(WIN_HEIGHT, thickness));
        line.setFillColor(gridColor);
        line.setRotation(90.f);
        line.setPosition(static_cast<float>(x), 0.f);

        window.draw(line);
    }

    // Horizontal lines
    for (int y = WIN_HEIGHT / 2 % gridSpacing; y < WIN_HEIGHT; y += gridSpacing) {
        float thickness = (y == WIN_HEIGHT / 2) ? centerThickness : normalThickness;

        sf::RectangleShape line(sf::Vector2f(WIN_WIDTH, thickness));
        line.setFillColor(gridColor);
        line.setPosition(0.f, static_cast<float>(y));

        window.draw(line);
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
            gen -= 1; // adjust for zero indexing
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
    genome.setGrowNetWights(weights_a);
    genome.setFlowNetWights(weights_b);

    vector<unique_ptr<Junction>> junctions;
    junctions.push_back(make_unique<Junction>(Junction{0.0, 0.0, INITIAL_ENERGY}));
    vector<unique_ptr<FoodSource>> foodSources = createRandomizedFoodSources();
    World world(genome);
    world.placeNewFoodSources(std::move(foodSources));
    world.placeNewJunctions(std::move(junctions));
    return world;
}



int main(int argc, char* argv[]) {

    int gen = -1;
    // read gen from command line argument
    if (argc > 1) {
        gen = std::stoi(argv[1]);
    }

    World world = readWorld(gen);
    world.run(NUM_STEPS, true);
    vector<Frame> frames = loadFrames();

    size_t currentFrame = 0;
    bool paused = false;
    sf::Clock clock;
    const float targetFrameTime = 1.f / FPS;

    sf::RenderWindow window(sf::VideoMode(WIN_WIDTH, WIN_HEIGHT), "Physarum Simulation");
    sf::View view(sf::FloatRect(0, 0, WIN_WIDTH, WIN_HEIGHT));
    view.zoom(DEFAULT_ZOOM);
    window.setView(view);

    sf::View uiView(sf::FloatRect(0, 0, WIN_WIDTH, WIN_HEIGHT));

    sf::Vector2f dragStart;
    bool dragging = false;

    sf::Cursor grabbing, point;
    grabbing.loadFromSystem(sf::Cursor::SizeAll);
    point.loadFromSystem(sf::Cursor::Arrow);

    sf::Font font;
    font.loadFromFile("Arial.ttf");

    while (window.isOpen()) {
        sf::sleep(sf::seconds(targetFrameTime) - clock.getElapsedTime());
        clock.restart();

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Space) paused = !paused;
                else if (event.key.code == sf::Keyboard::Right) {
                    if (!paused) paused = true;
                    currentFrame = min(currentFrame + 1, frames.size()-1); 
                }
                else if (event.key.code == sf::Keyboard::Left) {
                    if (!paused) paused = true;
                    currentFrame = (currentFrame == 0 ? 0 : currentFrame - 1);
                }
                // go to step zero
                else if (event.key.code == sf::Keyboard::Num0) {
                    currentFrame = 0;
                }
                // restart animation with next genome
                else if (event.key.code == sf::Keyboard::Enter) {
                    drawLoadingScreen(window, font);
                    world = readWorld(gen);
                    world.run(NUM_STEPS, true);
                    frames = loadFrames();
                    currentFrame = 0;
                    paused = false;
                }
                // set zoom and view to default
                else if (event.key.code == sf::Keyboard::Escape) {
                    view = sf::View(sf::FloatRect(0, 0, WIN_WIDTH, WIN_HEIGHT));
                    view.zoom(DEFAULT_ZOOM);
                    window.setView(view);
                }
            } else if (event.type == sf::Event::MouseWheelScrolled) {
                sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
                sf::Vector2f beforeZoom = window.mapPixelToCoords(pixelPos, view);
                float zoomFactor = (event.mouseWheelScroll.delta > 0) ? 0.9f : 1.1f;
                view.zoom(zoomFactor);
                sf::Vector2f afterZoom = window.mapPixelToCoords(pixelPos, view);
                view.move(beforeZoom - afterZoom);
                window.setView(view);
            }
        }

        window.setView(view);

        // double totalEnergy = accumulate(frames[currentFrame].junctions.begin(), frames[currentFrame].junctions.end(), 0.0,
        // [](double sum, const JunctionVisual& j) { return sum + j.energy; });

        window.setTitle("Fitness: " + std::to_string(frames[currentFrame].fitness) + 
            " | Junctions: " + std::to_string(frames[currentFrame].junctions.size()) +
            " | Frame: " + std::to_string(currentFrame) + "/" + std::to_string(frames.size()-1));
        window.clear(BACKGROUND_COLOR);

        drawGrid(window);
        drawFoodSources(window, frames[currentFrame].foodSources);
        drawJunctions(window, frames[currentFrame].junctions);
        drawTubes(window, frames[currentFrame].tubes);

        // --- Hover detection ---
        sf::Vector2i pixel = sf::Mouse::getPosition(window);
        sf::Vector2f mouse = window.mapPixelToCoords(pixel, view);

        string hoverText;

        // Food source info
        for (auto& f : frames[currentFrame].foodSources) {
            float cx = WIN_WIDTH/2 + f.x;
            float cy = WIN_HEIGHT/2 + f.y;
            if (hoverCircle(cx, cy, f.radius, mouse)) {
                hoverText = "FoodSource\n" +
                string("x = ") + to_string(f.x) +
                " y = " + to_string(f.y) +
                "\nenergy = " + to_string(f.energy);
            }
        }

        // Junction info
        for (auto& j : frames[currentFrame].junctions) {
            float r = 1 + JUNCTION_RADIUS_FACTOR * sqrt(j.energy / 3.14);
            float cx = WIN_WIDTH/2 + j.x;
            float cy = WIN_HEIGHT/2 + j.y;
            if (hoverCircle(cx, cy, r, mouse)) {
                hoverText = "Junction\n" +
                string("x = ") + to_string(j.x) +
                " y = " + to_string(j.y) +
                "\nenergy = " + to_string(j.energy) +
                "\ntouchingFoodSource = " + j.touchingFoodSource +
                "\nsignal = " + to_string(j.signal) + 
                "\nsignalHistory = [";
                for (size_t i = 0; i < j.signalHistory.size(); ++i) {
                    hoverText += to_string(j.signalHistory[i]);
                    if (i < j.signalHistory.size() - 1) hoverText += ", ";
                }
                hoverText += "]";
            }
        }

        // Tube info
        for (auto& t : frames[currentFrame].tubes) {
            sf::Vector2f p1(WIN_WIDTH/2 + t.x1, WIN_HEIGHT/2 + t.y1);
            sf::Vector2f p2(WIN_WIDTH/2 + t.x2, WIN_HEIGHT/2 + t.y2);
            float thickness = 1.f + TUBE_THICKNESS * t.flowRate;
            if (distToSegment(mouse, p1, p2) <= thickness*0.5f) {
                hoverText = "Tube\nflow=" + to_string(t.flowRate) +
                "\n(x1,y1)=" + to_string(t.x1) + "," + to_string(t.y1) +
                "\n(x2,y2)=" + to_string(t.x2) + "," + to_string(t.y2);
            }
        }

        window.setView(uiView);
        // Tooltip drawing
        if (!hoverText.empty()) {

            window.setMouseCursor(point);

            sf::Vector2i mp = sf::Mouse::getPosition(window);

            sf::Text text(hoverText, font, 40);
            text.setFillColor(sf::Color::White);
            sf::FloatRect bounds = text.getLocalBounds();

            float boxOffsetX = 20.f;
            float boxOffsetY = 20.f;

            int textOffsetX = 10;
            int textOffsetY = 6;

            float tx = mp.x + boxOffsetX;
            float ty = mp.y + boxOffsetY;

            sf::RectangleShape box;
            box.setFillColor(sf::Color(0, 0, 0, 180));
            box.setSize({bounds.width + static_cast<float>(2*textOffsetX), bounds.height + static_cast<float>(4*textOffsetY)});
            box.setPosition(tx, ty);

            text.setPosition(tx + textOffsetX, ty + textOffsetY);

            window.draw(box);
            window.draw(text);
        }

        window.display();

        if (!paused) {
            if (currentFrame < frames.size()-1) currentFrame++;
            else paused = true;
        }

        // --- Dragging ---
        if (event.type == sf::Event::MouseButtonPressed &&
            event.mouseButton.button == sf::Mouse::Left) {
            dragging = true;
            window.setMouseCursor(grabbing);
            dragStart = window.mapPixelToCoords(sf::Mouse::getPosition(window), view);
        }

        if (event.type == sf::Event::MouseButtonReleased &&
            event.mouseButton.button == sf::Mouse::Left) {
            window.setMouseCursor(point);
            dragging = false;
        }

        if (event.type == sf::Event::MouseMoved && dragging) {
            sf::Vector2f newPos = window.mapPixelToCoords(sf::Mouse::getPosition(window), view);
            sf::Vector2f delta = dragStart - newPos;
            view.move(delta);
            window.setView(view);
        }

    }
}
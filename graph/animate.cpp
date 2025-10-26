#include <SFML/Graphics.hpp>
#include <vector>
#include <vector>

#include "animate.hpp"
#include "physarum.hpp"

using namespace std;



int main() {
    sf::RenderWindow window(sf::VideoMode(800, 600), "Slime Mold Animation");
    window.setFramerateLimit(60);

    // Load saved generation
    
    vector<Frame> frames;

    size_t currentFrame = 0;
    bool paused = false;

    while (window.isOpen()) {
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

        if (!paused && currentFrame < frames.size()-1)
            currentFrame++;

        window.clear();

        // Draw objects


        // Display info text
        sf::Font font;
        font.loadFromFile("Arial.ttf"); // or your font
        sf::Text info("Frame: " + std::to_string(currentFrame), font, 20);
        info.setFillColor(sf::Color::White);
        info.setPosition(10, 10);
        window.draw(info);

        window.display();
    }
}

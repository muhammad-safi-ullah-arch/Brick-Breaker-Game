#include <SFML/Graphics.hpp>
#include <vector>
#include <optional>

int main() {
    // Create window (800x600)
    sf::RenderWindow window(sf::VideoMode({800, 600}), "SFML Brick Breaker");
    window.setFramerateLimit(60); // Smooth gameplay

    // ------------------- PLAYER 1 PADDLE (Bottom) -------------------
    sf::RectangleShape paddle({100.f, 20.f});
    paddle.setFillColor(sf::Color::Blue);
    paddle.setOrigin({50.f, 10.f});  // Center origin
    paddle.setPosition({400.f, 550.f}); // Bottom center

    
    // ------------------- PLAYER 2 PADDLE (TOP) -------------------
    sf::RectangleShape paddle2({100.f, 20.f}); 
    paddle2.setFillColor(sf::Color::Green);
    paddle2.setOrigin({50.f, 10.f});
    paddle2.setPosition({400.f, 50.f}); // Top side

    // ------------------- DIVIDING LINE -------------------
    sf::RectangleShape dividingLine({800.f, 3.f}); // full width line
    dividingLine.setFillColor(sf::Color::White); 
    dividingLine.setPosition({0.f, 300.f}); // middle of screen (600/2)
    

    // ------------------- BALL -------------------
    sf::CircleShape ball(10.f);
    ball.setFillColor(sf::Color::White);
    ball.setOrigin({10.f, 10.f}); // Center origin
    ball.setPosition({400.f, 350.f}); // Not start in middle 

    // Ball movement speed
    sf::Vector2f ballVelocity{4.f, -4.f};

    // ------------------- BRICKS -------------------
    struct Brick {
        sf::RectangleShape shape;
        bool destroyed = false; // Track if brick is broken
    };

    std::vector<Brick> bricks;

    // Create brick grid (8 columns x 6 rows)
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 6; ++j) {   
            Brick b;

            b.shape.setSize({90.f, 30.f});
            b.shape.setFillColor(sf::Color::Red);

            // Add border to bricks
            b.shape.setOutlineThickness(2.f);
            b.shape.setOutlineColor(sf::Color::Black);

            // Position bricks in grid
            // Split bricks into top (0-2) and bottom (3-5)
            if (j < 3) {                                    // V6 Changes
                // TOP 3 ROWS (Player 2 side)
                b.shape.setPosition({i * 100.f + 5.f, j * 35.f + 180.f}); 
            } else {
                // BOTTOM 3 ROWS (Player 1 side)
                b.shape.setPosition({i * 100.f + 5.f, (j - 3) * 35.f + 350.f});
            } 

            bricks.push_back(b);
        }
    }

    // ================= GAME LOOP =================
    while (window.isOpen()) {

        // -------- EVENT HANDLING --------
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

                // ********** INPUT (PADDLE CONTROL) **********
                
        // -------- PLAYER 1 CONTROLS --------
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) &&
            paddle.getPosition().x > 50)
        {
            paddle.move({-7.f, 0.f}); // Move left
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) &&
            paddle.getPosition().x < 750)
        {
            paddle.move({7.f, 0.f}); // Move right
        }
        
        // -------- PLAYER 2 CONTROLS --------
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) &&
            paddle2.getPosition().x > 50)
        {
            paddle2.move({-7.f, 0.f}); // Move left  
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) &&
            paddle2.getPosition().x < 750)
        {
            paddle2.move({7.f, 0.f}); // Move right
        }

        // -------- BALL MOVEMENT --------
        ball.move(ballVelocity);

        sf::Vector2f pos = ball.getPosition();


                // ******** COLLISIONS **********

        // -------- WALL COLLISIONS --------
        if (pos.x < 10 || pos.x > 790)
            ballVelocity.x = -ballVelocity.x; // Left/right bounce

        if (pos.y < 10)
            ballVelocity.y = -ballVelocity.y; // Top bounce

        if (pos.y > 600)
            window.close(); // Game over


        // -------- PLAYER 1 (BOTTOM) PADDLE COLISSION --------
        if (ball.getGlobalBounds().findIntersection(paddle.getGlobalBounds())) {
            ballVelocity.y = -abs(ballVelocity.y); // always go UP
        }

         // -------- DIVIDING LINE COLLISION --------
        if (ball.getGlobalBounds().findIntersection(dividingLine.getGlobalBounds())) {
    
            // Only bounce if moving toward the line
            if (ballVelocity.y < 0 && ball.getPosition().y > 300) {
                ballVelocity.y = -ballVelocity.y;
            }
            else if (ballVelocity.y > 0 && ball.getPosition().y < 300) {
                ballVelocity.y = -ballVelocity.y;
            }
        }

        // -------- PLAYER 2 (TOP) PADDLE COLISSION--------
        if (ball.getGlobalBounds().findIntersection(paddle2.getGlobalBounds())) {
            ballVelocity.y = abs(ballVelocity.y); // always go DOWN
        }                                           

        // -------- BRICK COLLISION --------
        for (auto& b : bricks) {
            if (!b.destroyed &&
                ball.getGlobalBounds().findIntersection(b.shape.getGlobalBounds()))
            {
                b.destroyed = true;      // Break brick
                ballVelocity.y = -ballVelocity.y; // Bounce
                break;
            }
        }

        // -------- RENDERING --------
        window.clear();

        window.draw(paddle);
        window.draw(paddle2); 
        window.draw(dividingLine); 
        window.draw(ball);

        for (const auto& b : bricks) {
            if (!b.destroyed)
                window.draw(b.shape);
        }

        window.display();
    }

    return 0;
}
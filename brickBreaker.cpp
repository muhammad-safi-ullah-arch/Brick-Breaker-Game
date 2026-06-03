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
    ball.setPosition({400.f, 550.f}); // V7 ---> Not start in middle 

    // Ball movement speed
    sf::Vector2f ballVelocity{4.f, -4.f};

    // ------------------- SECOND BALL (Player 2) -------------------
    sf::CircleShape ball2(10.f); 
    ball2.setFillColor(sf::Color::Yellow); 
    ball2.setOrigin({10.f, 10.f});
    ball2.setPosition({400.f, 50.f}); // V7 NEW: second ball above dividing line

    sf::Vector2f ballVelocity2{4.f, 4.f}; // V7 NEW: separate velocity for player 2


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
            if (j < 3) {                                   
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

        ball2.move(ballVelocity2); // V7 NEW: move second ball


                // ******** COLLISIONS **********

            // -------- WALL COLLISIONS --------

        // -------- First BALL WALL COLLISIONS --------
        if (pos.x < 10 || pos.x > 790)
            ballVelocity.x = -ballVelocity.x; // Left/right bounce

        if (pos.y < 10)
            ballVelocity.y = -ballVelocity.y; // Top bounce

        if (pos.y > 600)
            window.close(); // Game over

        // -------- SECOND BALL WALL COLLISIONS --------
        sf::Vector2f pos2 = ball2.getPosition(); // V7 NEW

        if (pos2.x < 10 || pos2.x > 790)
            ballVelocity2.x = -ballVelocity2.x;

        if (pos2.y > 590)
            ballVelocity2.y = -ballVelocity2.y; // V7 NEW: bounce from bottom

            if (pos2.y < 0)
                window.close(); // V8 NEW: top player loses


        // -------- PLAYER 1 (BOTTOM) PADDLE COLISSION --------
        if (ball.getGlobalBounds().findIntersection(paddle.getGlobalBounds())) {
            ballVelocity.y = -abs(ballVelocity.y); // always go UP
        }

        // -------- PLAYER 2 PADDLE COLLISION (BALL 2) --------
        if (ball2.getGlobalBounds().findIntersection(paddle2.getGlobalBounds())) {
            ballVelocity2.y = abs(ballVelocity2.y); // V7 NEW
        }

         // -------- DIVIDING LINE COLLISION (Ball 1) --------
        if (ball.getGlobalBounds().findIntersection(dividingLine.getGlobalBounds())) {
    
            // Only bounce if moving toward the line
            if (ballVelocity.y < 0 && ball.getPosition().y > 300) {
                ballVelocity.y = -ballVelocity.y;
            }
            else if (ballVelocity.y > 0 && ball.getPosition().y < 300) {
                ballVelocity.y = -ballVelocity.y;
            }
        }

        // -------- DIVIDING LINE COLLISION (BALL 2) --------
        if (ball2.getGlobalBounds().findIntersection(dividingLine.getGlobalBounds())) {
            ballVelocity2.y = -ballVelocity2.y; // V8 NEW
        }

        // -------- PLAYER 2 (TOP) PADDLE COLISSION--------
        if (ball.getGlobalBounds().findIntersection(paddle2.getGlobalBounds())) {
            ballVelocity.y = abs(ballVelocity.y); // always go DOWN
        }                                           

        // -------- BRICK COLLISION (Ball1) --------
        for (auto& b : bricks) {
            if (!b.destroyed &&
                ball.getGlobalBounds().findIntersection(b.shape.getGlobalBounds()))
            {
                b.destroyed = true;      // Break brick
                ballVelocity.y = -ballVelocity.y; // Bounce
                break;
            }
            // -------- BRICK COLLISION (BALL 2) --------
            if (!b.destroyed &&
                ball2.getGlobalBounds().findIntersection(b.shape.getGlobalBounds()))
            {
                b.destroyed = true;
                ballVelocity2.y = -ballVelocity2.y; // V7 NEW
            }
        }

        // -------- RENDERING --------
        window.clear();

        window.draw(paddle);
        window.draw(paddle2); 
        window.draw(dividingLine); 
        window.draw(ball);
        window.draw(ball2); // V7.... New

        for (const auto& b : bricks) {
            if (!b.destroyed)
                window.draw(b.shape);
        }

        window.display();
    }

    return 0;
}
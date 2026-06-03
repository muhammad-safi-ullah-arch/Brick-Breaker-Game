#include <SFML/Graphics.hpp>
#include <vector>
#include <optional>

int main() {
    // Creating window (800x600)
    sf::RenderWindow window(sf::VideoMode({800, 600}), "SFML Brick Breaker");
    window.setFramerateLimit(60); // Smooth gameplay

    // ------------------- PADDLE -------------------
    sf::RectangleShape paddle({100.f, 20.f});
    paddle.setFillColor(sf::Color::Blue);
    paddle.setOrigin({50.f, 10.f});  // Center origin
    paddle.setPosition({400.f, 550.f}); // Bottom center

    // ------------------- BALL -------------------
    sf::CircleShape ball(10.f);
    ball.setFillColor(sf::Color::White);
    ball.setOrigin({10.f, 10.f}); // Center origin
    ball.setPosition({400.f, 300.f}); // Start in middle

    // Ball movement speed
    sf::Vector2f ballVelocity{4.f, -4.f};

    // ------------------- BRICKS -------------------
    struct Brick {
        sf::RectangleShape shape;
        bool destroyed = false; // Track if brick is broken
    };

    std::vector<Brick> bricks;

    // Create brick grid (8 columns x 5 rows)
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 5; ++j) {
            Brick b;

            b.shape.setSize({90.f, 30.f});
            b.shape.setFillColor(sf::Color::Red);

            // Add border to bricks
            b.shape.setOutlineThickness(2.f);
            b.shape.setOutlineColor(sf::Color::Black);

            // Position bricks in grid
            b.shape.setPosition({i * 100.f + 5.f, j * 40.f + 50.f}); 
           
            bricks.push_back(b);
        }
    }
            /*
            x=i⋅(brick width+gap)+offsetx
	        y=j⋅(brick height+gap)+offsety

	         X-Coordinate
            i * 100.f + 5.f
            i → column number (0,1,2,...)
            100.f → horizontal spacing between bricks
            +5.f → small margin from left edge

            So:

            Brick 0 → x = 5
            Brick 1 → x = 105
            Brick 2 → x = 205 */
    

    // ================= GAME LOOP =================
    while (window.isOpen()) {

        // -------- EVENT HANDLING --------
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        // -------- INPUT (PADDLE CONTROL) --------
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

        // -------- BALL MOVEMENT --------
        ball.move(ballVelocity);

        sf::Vector2f pos = ball.getPosition();

        // -------- WALL COLLISIONS --------
        if (pos.x < 10 || pos.x > 790)
            ballVelocity.x = -ballVelocity.x; // Left/right bounce

        if (pos.y < 10)
            ballVelocity.y = -ballVelocity.y; // Top bounce

        if (pos.y > 600)
            window.close(); // Game over

        // -------- PADDLE COLLISION --------
        if (ball.getGlobalBounds().findIntersection(paddle.getGlobalBounds())) {
            ballVelocity.y = -ballVelocity.y;
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
        window.draw(ball);

        for (const auto& b : bricks) {
            if (!b.destroyed)
                window.draw(b.shape);
        }

        window.display();
    }

    return 0;
}
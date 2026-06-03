#include <SFML/Graphics.hpp>
#include <optional>
#include <cmath>

int main() {
    sf::RenderWindow window(sf::VideoMode({800, 600}), "2 Player Paddle Game");
    window.setFramerateLimit(60);

    // PADDLE 1 (BOTTOM)
    sf::RectangleShape paddle({100.f, 20.f});
    paddle.setFillColor(sf::Color::Blue);
    paddle.setOrigin({50.f, 10.f});
    paddle.setPosition({400.f, 550.f});

    // PADDLE 2 (TOP)
    sf::RectangleShape paddle2({100.f, 20.f});
    paddle2.setFillColor(sf::Color::Green);
    paddle2.setOrigin({50.f, 10.f});
    paddle2.setPosition({400.f, 50.f});

    // BALL 1
    sf::CircleShape ball(10.f);
    ball.setOrigin({10.f, 10.f});
    ball.setPosition({400.f, 550.f});
    sf::Vector2f vel{4.f, -4.f};

    // BALL 2
    sf::CircleShape ball2(10.f);
    ball2.setOrigin({10.f, 10.f});
    ball2.setPosition({400.f, 50.f});
    sf::Vector2f vel2{4.f, 4.f};

    // LIVES
    int lives1 = 3;
    int lives2 = 3;

    // FONT (SFML 3 style)
    sf::Font font("arial.ttf");

    // TEXT
    sf::Text t1(font, "Player 1 Lives: 3");
    sf::Text t2(font, "Player 2 Lives: 3");

    t1.setCharacterSize(20);
    t2.setCharacterSize(20);

    t1.setPosition({10.f, 10.f});
    t2.setPosition({520.f, 10.f});

    while (window.isOpen()) {

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        // GAME OVER CHECK
        bool gameOver = (lives1 == 0 || lives2 == 0);

        if (!gameOver) {

            // CONTROLS
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) && paddle.getPosition().x > 50)
                paddle.move({-7, 0});

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) && paddle.getPosition().x < 750)
                paddle.move({7, 0});

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) && paddle2.getPosition().x > 50)
                paddle2.move({-7, 0});

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) && paddle2.getPosition().x < 750)
                paddle2.move({7, 0});

            // MOVE BALLS
            ball.move(vel);
            ball2.move(vel2);

            // WALL COLLISION (BALL 1)
            if (ball.getPosition().x < 10 || ball.getPosition().x > 790)
                vel.x = -vel.x;

            if (ball.getPosition().y < 10)
                vel.y = -vel.y;

            // PLAYER 1 LOSES LIFE
            if (ball.getPosition().y > 600) {
                if (lives1 > 0) lives1--;
                ball.setPosition({400.f, 550.f});
            }

            // WALL COLLISION (BALL 2)
            if (ball2.getPosition().x < 10 || ball2.getPosition().x > 790)
                vel2.x = -vel2.x;

            if (ball2.getPosition().y > 590)
                vel2.y = -vel2.y;

            // PLAYER 2 LOSES LIFE
            if (ball2.getPosition().y < 0) {
                if (lives2 > 0) lives2--;
                ball2.setPosition({400.f, 50.f});
            }

            // PADDLE COLLISION
            if (ball.getGlobalBounds().findIntersection(paddle.getGlobalBounds()))
                vel.y = -std::abs(vel.y);

            if (ball2.getGlobalBounds().findIntersection(paddle2.getGlobalBounds()))
                vel2.y = std::abs(vel2.y);
        }

        // TEXT UPDATE
        if (!gameOver) {
            t1.setString("Player 1 Lives: " + std::to_string(lives1));
            t2.setString("Player 2 Lives: " + std::to_string(lives2));
        } else {
            if (lives1 == 0) {
                t1.setString("Player 1 LOST!");
                t2.setString("Player 2 WINS!");
            } else {
                t2.setString("Player 2 LOST!");
                t1.setString("Player 1 WINS!");
            }
        }

        // DRAW
        window.clear();

        window.draw(paddle);
        window.draw(paddle2);
        window.draw(ball);
        window.draw(ball2);
        window.draw(t1);
        window.draw(t2);

        window.display();
    }

    return 0;
}
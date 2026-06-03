#include <SFML/Graphics.hpp>
#include <optional>
#include <cstdlib>
#include <ctime>

int main() {
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Turn-Based Paddle Game");
    window.setFramerateLimit(60);

    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // PADDLES
    sf::RectangleShape p1({100.f, 20.f});
    p1.setOrigin({50.f, 10.f});
    p1.setPosition({400.f, 550.f});
    p1.setFillColor(sf::Color::Blue);

    sf::RectangleShape p2({100.f, 20.f});
    p2.setOrigin({50.f, 10.f});
    p2.setPosition({400.f, 50.f});
    p2.setFillColor(sf::Color::Green);

    // BALL
    sf::CircleShape ball(10.f);
    ball.setOrigin({10.f, 10.f});

    // FONT
    sf::Font font("arial.ttf");

    sf::Text t1(font, "", 20);
    sf::Text t2(font, "", 20);

    t1.setPosition({10.f, 10.f});
    t2.setPosition({600.f, 10.f});

    int lives1 = 3, lives2 = 3;

    // GAME STATE
    int currentPlayer = (std::rand() % 2) + 1;
    bool isMoving = true;
    bool ballLaunched = false;
    bool gameOver = false;

    float paddleSpeed = 5.f;
    float ballSpeed = 6.f;
    sf::Vector2f ballVel(0, 0);

    while (window.isOpen()) {

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();

            if (!gameOver && event->is<sf::Event::KeyPressed>()) {
                if (event->getIf<sf::Event::KeyPressed>()->code == sf::Keyboard::Key::Space) {

                    if (!ballLaunched) {
                        isMoving = false;
                        ballLaunched = true;

                        if (currentPlayer == 1)
                            ballVel = {0, -ballSpeed};
                        else
                            ballVel = {0, ballSpeed};
                    }
                }
            }
        }

        if (!gameOver) {

            // AUTO MOVE
            if (isMoving) {
                if (currentPlayer == 1) {
                    p1.move({paddleSpeed, 0});
                    if (p1.getPosition().x > 750 || p1.getPosition().x < 50)
                        paddleSpeed = -paddleSpeed;
                } else {
                    p2.move({paddleSpeed, 0});
                    if (p2.getPosition().x > 750 || p2.getPosition().x < 50)
                        paddleSpeed = -paddleSpeed;
                }
            }

            // BALL STICK
            if (!ballLaunched) {
                if (currentPlayer == 1)
                    ball.setPosition({p1.getPosition().x, p1.getPosition().y - 20});
                else
                    ball.setPosition({p2.getPosition().x, p2.getPosition().y + 20});
            }

            // MOVE BALL
            if (ballLaunched)
                ball.move(ballVel);

            // HIT CHECK
            if (currentPlayer == 1 &&
                ball.getGlobalBounds().findIntersection(p2.getGlobalBounds())) {

                if (lives2 > 0) lives2--;

                if (lives2 == 0) gameOver = true;

                ballLaunched = false;
                isMoving = true;

                p2.setPosition({static_cast<float>(50 + rand() % 700), 50});
            }
            else if (currentPlayer == 2 &&
                     ball.getGlobalBounds().findIntersection(p1.getGlobalBounds())) {

                if (lives1 > 0) lives1--;

                if (lives1 == 0) gameOver = true;

                ballLaunched = false;
                isMoving = true;

                p1.setPosition({static_cast<float>(50 + rand() % 700), 550});
            }

            // MISS
            if (ball.getPosition().y < 0 || ball.getPosition().y > 600) {
                ballLaunched = false;
                isMoving = true;

                currentPlayer = (currentPlayer == 1) ? 2 : 1;
            }
        }

        // TEXT UPDATE
        t1.setString("P1 Lives: " + std::to_string(lives1));
        t2.setString("P2 Lives: " + std::to_string(lives2));

        if (gameOver) {
            if (lives1 == 0) {
                t1.setString("P1 LOST");
                t2.setString("P2 WINS");
            } else {
                t2.setString("P2 LOST");
                t1.setString("P1 WINS");
            }
        }

        window.clear();
        window.draw(p1);
        window.draw(p2);
        window.draw(ball);
        window.draw(t1);
        window.draw(t2);
        window.display();
    }

    return 0;
}
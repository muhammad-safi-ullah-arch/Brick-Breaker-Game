#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <optional>
#include <string>
#include <cmath>
#include <iostream>

// --- Game States ---
enum class State { TITLE, BREAKER, WAITING, FACEOFF, GAME_OVER };

// --- Physics Helper ---
void enforceMinimumVerticalAngle(sf::Vector2f& vel, float speed) {
    float minY = speed * 0.2f; 
    if (std::abs(vel.y) < minY) {
        vel.y = (vel.y > 0) ? minY : -minY;
        float newMag = std::sqrt(vel.x * vel.x + vel.y * vel.y);
        vel.x = (vel.x / newMag) * speed;
        vel.y = (vel.y / newMag) * speed;
    }
}

// --- Player Data Structure ---
struct Player {
    sf::RectangleShape paddle;
    sf::CircleShape ball;
    int lives = 6; 
    bool isBot = false;
    bool ballAttached = true;
    sf::Vector2f ballVel{ 0.f, 0.f };
    bool bricksFinished = false;
    
    int sizeLevel = 4; 

    float getCurrentWidth() const { return sizeLevel * 60.f; }

    void updateSize(int deltaLevel) {
        sizeLevel += deltaLevel;
        if (sizeLevel < 1) sizeLevel = 1; 
        if (sizeLevel > 7) sizeLevel = 7; 
        
        paddle.setSize({ getCurrentWidth(), 20.f });
        paddle.setOrigin({ getCurrentWidth() / 2.f, 10.f });
    }

    void resetBall(float startY, bool isTop) {
        ballAttached = true;
        ballVel = { 0.f, 0.f };
        paddle.setPosition({ 960.f, startY });
        float yOffset = isTop ? 20.f : -20.f;
        ball.setPosition({ 960.f, startY + yOffset });
    }

    void checkWallCollisions(bool isSinglePlayer, bool isPlayer1) {
        if (ball.getPosition().x < 250.f) { 
            ball.setPosition({250.f, ball.getPosition().y}); 
            ballVel.x = std::abs(ballVel.x); 
        } else if (ball.getPosition().x > 1670.f) { 
            ball.setPosition({1670.f, ball.getPosition().y}); 
            ballVel.x = -std::abs(ballVel.x); 
        }
        
        if (isPlayer1) {
            float ceiling = isSinglePlayer ? 0.f : 540.f;
            if (ball.getPosition().y < ceiling && ballVel.y < 0) ballVel.y *= -1; 
        } else {
            if (ball.getPosition().y > 540.f && ballVel.y > 0) ballVel.y *= -1; 
        }
    }

    void handlePaddleCollision(bool isPlayer1) {
        if (ball.getGlobalBounds().findIntersection(paddle.getGlobalBounds())) {
            float currentSpeed = std::sqrt(ballVel.x * ballVel.x + ballVel.y * ballVel.y);
            // Bounce up if P1 (bottom), bounce down if P2 (top)
            ballVel.y = isPlayer1 ? -std::abs(ballVel.y) : std::abs(ballVel.y); 
            
            float hitOffset = ball.getPosition().x - paddle.getPosition().x;
            float normalizedOffset = hitOffset / (getCurrentWidth() / 2.f); 
            ballVel.x += normalizedOffset * 4.0f; 
            
            float newMagnitude = std::sqrt(ballVel.x * ballVel.x + ballVel.y * ballVel.y);
            ballVel.x = (ballVel.x / newMagnitude) * currentSpeed;
            ballVel.y = (ballVel.y / newMagnitude) * currentSpeed;
            enforceMinimumVerticalAngle(ballVel, currentSpeed); 
        }
    }
};

// --- Brick Structure ---
struct Brick {
    sf::RectangleShape shape;
    bool destroyed = false;
    int type = 0; 
    bool isPlayer1Side = false;
};

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    
    // --- FULLSCREEN WINDOW SETUP ---
    sf::RenderWindow window(sf::VideoMode({ 1920, 1080 }), "Paddle Royale", sf::State::Fullscreen);
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.openFromFile("C:\\Windows\\Fonts\\arial.ttf")) { 
        std::cerr << "Warning: arial.ttf not found.\n";
    }

    // --- AUDIO SETUP ---
    sf::Music bgMusic;
    bool isMuted = false;
    if (!bgMusic.openFromFile("Snowy.mp3")) {
        std::cerr << "Warning: Snowy.mp3 not found.\n";
    } else {
        bgMusic.setLooping(true); 
        bgMusic.setVolume(50.f);
        bgMusic.play();
    }

    // --- STARFIELD BACKGROUND SETUP ---
    std::vector<sf::CircleShape> stars;
    for (int i = 0; i < 250; ++i) {
        float radius = (std::rand() % 30) / 20.f + 0.5f; 
        sf::CircleShape star(radius);
        star.setPosition({ static_cast<float>(std::rand() % 1920), static_cast<float>(std::rand() % 1080) });
        int alpha = std::rand() % 155 + 100;
        star.setFillColor(sf::Color(255, 255, 255, alpha));
        stars.push_back(star);
    }

    State gameState = State::TITLE;
    bool isPaused = false;
    bool isSinglePlayer = false;
    bool controlsSwapped = false; 
    
    // UI Panels (Walls)
    sf::RectangleShape leftWall({ 240.f, 1080.f }); leftWall.setFillColor(sf::Color(40, 40, 40));
    sf::RectangleShape rightWall({ 240.f, 1080.f }); rightWall.setPosition({ 1680.f, 0.f }); rightWall.setFillColor(sf::Color(40, 40, 40));

    // UI Texts
    sf::Text p1Title(font, "PLAYER 1", 35); p1Title.setPosition({ 40.f, 850.f }); p1Title.setFillColor(sf::Color::Blue);
    sf::Text p1LivesText(font, "Lives: 6", 30); p1LivesText.setPosition({ 40.f, 900.f });
    sf::Text p1SizeText(font, "Size: 4", 25); p1SizeText.setPosition({ 40.f, 950.f });
    
    sf::Text p2Title(font, "PLAYER 2", 35); p2Title.setPosition({ 1700.f, 50.f }); p2Title.setFillColor(sf::Color::Green);
    sf::Text p2LivesText(font, "Lives: 6", 30); p2LivesText.setPosition({ 1700.f, 100.f });
    sf::Text p2SizeText(font, "Size: 4", 25); p2SizeText.setPosition({ 1700.f, 150.f });
    
    sf::Text centerText(font, "", 50); 
    sf::Text titleText(font, "PADDLE ROYALE", 80); titleText.setPosition({ 650.f, 200.f });
    sf::Text controlsText(font, "", 30); controlsText.setPosition({ 750.f, 700.f });
    sf::Text swapText(font, "Press TAB to switch player controls", 25); swapText.setPosition({ 770.f, 900.f }); swapText.setFillColor(sf::Color::Yellow);

    // --- TITLE MENU BUTTONS ---
    sf::RectangleShape btnSPRect({ 400.f, 70.f }); btnSPRect.setPosition({ 760.f, 380.f }); btnSPRect.setFillColor(sf::Color(60, 60, 60));
    sf::Text btnSPText(font, "Single Player Mode", 35); btnSPText.setPosition({ 810.f, 395.f });

    sf::RectangleShape btnMPRect({ 400.f, 70.f }); btnMPRect.setPosition({ 760.f, 480.f }); btnMPRect.setFillColor(sf::Color(60, 60, 60));
    sf::Text btnMPText(font, "Two Player Mode", 35); btnMPText.setPosition({ 825.f, 495.f });

    sf::RectangleShape btnTitleExitRect({ 400.f, 70.f }); btnTitleExitRect.setPosition({ 760.f, 580.f }); btnTitleExitRect.setFillColor(sf::Color(180, 50, 50));
    sf::Text btnTitleExitText(font, "Exit Game", 35); btnTitleExitText.setPosition({ 875.f, 595.f });

    sf::RectangleShape btnTitleMuteRect({ 140.f, 50.f }); btnTitleMuteRect.setPosition({ 1700.f, 980.f }); btnTitleMuteRect.setFillColor(sf::Color(80, 80, 80));
    sf::Text btnTitleMuteText(font, "MUTE", 25); 

    // --- PAUSE MENU BUTTONS ---
    sf::Text pauseText(font, "PAUSED", 45); pauseText.setPosition({ 870.f, 350.f });
    
    sf::RectangleShape btnReturnRect({ 550.f, 40.f }); btnReturnRect.setPosition({ 685.f, 430.f }); btnReturnRect.setFillColor(sf::Color(180, 50, 50));
    sf::Text btnReturnText(font, "RETURN TO TITLE SCREEN", 22); btnReturnText.setPosition({ 805.f, 437.f });

    sf::RectangleShape btnResumeRect({ 350.f, 100.f }); btnResumeRect.setPosition({ 785.f, 500.f }); btnResumeRect.setFillColor(sf::Color(50, 180, 50));
    sf::Text btnResumeText(font, "RESUME", 50); btnResumeText.setPosition({ 860.f, 515.f });

    sf::RectangleShape btnPauseMuteRect({ 250.f, 45.f }); btnPauseMuteRect.setPosition({ 835.f, 630.f }); btnPauseMuteRect.setFillColor(sf::Color(80, 80, 80));
    sf::Text btnPauseMuteText(font, "MUTE AUDIO", 25);

    // --- GAME OVER BUTTONS ---
    sf::RectangleShape btnPlayAgainRect({ 300.f, 60.f }); btnPlayAgainRect.setPosition({ 810.f, 550.f }); btnPlayAgainRect.setFillColor(sf::Color(50, 180, 50));
    sf::Text btnPlayAgainText(font, "PLAY AGAIN", 30); btnPlayAgainText.setPosition({ 865.f, 560.f });

    sf::RectangleShape btnGameOverReturnRect({ 450.f, 60.f }); btnGameOverReturnRect.setPosition({ 735.f, 640.f }); btnGameOverReturnRect.setFillColor(sf::Color(180, 50, 50));
    sf::Text btnGameOverReturnText(font, "RETURN TO TITLE SCREEN", 25); btnGameOverReturnText.setPosition({ 790.f, 655.f });

    // --- TEXT CENTERING HELPERS ---
    auto alignTextToCenter = [](sf::Text& text, const sf::RectangleShape& bg) {
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin({ bounds.position.x + bounds.size.x / 2.0f, 
                         bounds.position.y + bounds.size.y / 2.0f });
        text.setPosition({ bg.getPosition().x + bg.getSize().x / 2.0f, 
                           bg.getPosition().y + bg.getSize().y / 2.0f });
    };

    auto alignTextToPoint = [](sf::Text& text, sf::Vector2f point) {
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin({ bounds.position.x + bounds.size.x / 2.0f, 
                         bounds.position.y + bounds.size.y / 2.0f });
        text.setPosition(point);
    };

    alignTextToCenter(btnTitleMuteText, btnTitleMuteRect);
    alignTextToCenter(btnPauseMuteText, btnPauseMuteRect);
    alignTextToPoint(pauseText, { 960.f, 350.f });

    Player p1, p2;
    p1.paddle.setFillColor(sf::Color::Blue); p2.paddle.setFillColor(sf::Color::Green);
    p1.ball.setRadius(10.f); p1.ball.setOrigin({ 10.f, 10.f }); p1.ball.setFillColor(sf::Color::White);
    p2.ball.setRadius(10.f); p2.ball.setOrigin({ 10.f, 10.f }); p2.ball.setFillColor(sf::Color::Yellow);

    sf::RectangleShape dividingLine({ 1440.f, 3.f });
    dividingLine.setFillColor(sf::Color::White);
    dividingLine.setPosition({ 240.f, 540.f });

    std::vector<Brick> bricks;
    sf::Clock waitClock;
    float countdownTime = 60.f;

    int activePlayer = 1;
    bool faceoffBallLaunched = false;
    float faceoffPaddleSpeed = 12.f; 

    auto toggleAudio = [&]() {
        isMuted = !isMuted;
        bgMusic.setVolume(isMuted ? 0.f : 50.f);
        btnTitleMuteText.setString(isMuted ? "UNMUTE" : "MUTE");
        btnPauseMuteText.setString(isMuted ? "UNMUTE AUDIO" : "MUTE AUDIO");
        alignTextToCenter(btnTitleMuteText, btnTitleMuteRect);
        alignTextToCenter(btnPauseMuteText, btnPauseMuteRect);
    };

    auto resetGame = [&]() {
        p1.sizeLevel = 4; p1.updateSize(0); 
        p1.resetBall(1020.f, false); 
        p1.bricksFinished = false;

        if (isSinglePlayer) {
            p1.lives = 3;
        } else {
            p1.lives = 6; p2.lives = 6; 
            p2.sizeLevel = 4; p2.updateSize(0);
            p2.resetBall(60.f, true);    
            p2.bricksFinished = false;
        }
        
        bricks.clear();
        for (int i = 0; i < 12; ++i) {
            for (int j = 0; j < 12; ++j) { 
                Brick b;
                b.shape.setSize({ 120.f, 35.f });
                b.shape.setOutlineThickness(-3.f);
                b.shape.setOutlineColor(sf::Color(25, 25, 25)); 

                b.isPlayer1Side = isSinglePlayer ? true : (j >= 6);
                
                // Unified special brick pattern
                int row = j % 6; 
                bool isSpecial = false;
                if (row == 1 && (i == 2 || i == 9)) isSpecial = true; 
                if (row == 3 && i == 5) isSpecial = true;              
                if (row == 4 && (i == 3 || i == 8)) isSpecial = true; 

                if (isSpecial) {
                    b.type = (std::rand() % 2 == 0) ? 1 : 2; 
                    b.shape.setFillColor((b.type == 1) ? sf::Color::Red : sf::Color::Green);
                } else {
                    b.shape.setFillColor(sf::Color::White);
                }

                float yPos;
                if (isSinglePlayer) yPos = j * 35.f + 100.f;
                else if (!b.isPlayer1Side) yPos = j * 35.f + 300.f;
                else yPos = (j - 6) * 35.f + 570.f;

                b.shape.setPosition({ i * 120.f + 240.f, yPos }); 
                bricks.push_back(b);
            }
        }
    };

    auto setFaceoffTurn = [&](int turnPlayer) {
        activePlayer = turnPlayer;
        faceoffBallLaunched = false;
        if (activePlayer == 1) {
            p1.paddle.setPosition({ 960.f, 1020.f });
            p2.paddle.setPosition({ static_cast<float>(300 + rand() % 1300), 60.f });
            p1.ball.setPosition({ p1.paddle.getPosition().x, 1000.f });
        } else {
            p2.paddle.setPosition({ 960.f, 60.f });
            p1.paddle.setPosition({ static_cast<float>(300 + rand() % 1300), 1020.f });
            p2.ball.setPosition({ p2.paddle.getPosition().x, 80.f });
        }
    };

    // ================= GAME LOOP =================
    while (window.isOpen()) {
        // -------- EVENT HANDLING --------
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();

            if (const auto* mouseEvent = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseEvent->button == sf::Mouse::Button::Left) {
                    sf::Vector2f mousePos = window.mapPixelToCoords(mouseEvent->position);

                    if (gameState == State::TITLE) {
                        if (btnSPRect.getGlobalBounds().contains(mousePos)) {
                            isSinglePlayer = true; p2.isBot = false; resetGame(); gameState = State::BREAKER;
                        }
                        if (btnMPRect.getGlobalBounds().contains(mousePos)) {
                            isSinglePlayer = false; p2.isBot = false; resetGame(); gameState = State::BREAKER;
                        }
                        if (btnTitleExitRect.getGlobalBounds().contains(mousePos)) {
                            window.close();
                        }
                        if (btnTitleMuteRect.getGlobalBounds().contains(mousePos)) {
                            toggleAudio();
                        }
                    } else if (isPaused) {
                        if (btnReturnRect.getGlobalBounds().contains(mousePos)) {
                            isPaused = false; gameState = State::TITLE;
                        }
                        if (btnResumeRect.getGlobalBounds().contains(mousePos)) {
                            isPaused = false;
                        }
                        if (btnPauseMuteRect.getGlobalBounds().contains(mousePos)) {
                            toggleAudio();
                        }
                    } else if (gameState == State::GAME_OVER) {
                        if (btnPlayAgainRect.getGlobalBounds().contains(mousePos)) {
                            resetGame(); gameState = State::BREAKER;
                        }
                        if (btnGameOverReturnRect.getGlobalBounds().contains(mousePos)) {
                            gameState = State::TITLE;
                        }
                    }
                }
            }

            if (event->is<sf::Event::KeyPressed>()) {
                auto code = event->getIf<sf::Event::KeyPressed>()->code;

                if (code == sf::Keyboard::Key::Escape && gameState != State::TITLE) {
                    isPaused = !isPaused;
                }

                if (gameState == State::TITLE) {
                    if (code == sf::Keyboard::Key::Tab) controlsSwapped = !controlsSwapped;
                }

                if (gameState == State::FACEOFF && !isPaused && !faceoffBallLaunched) {
                    if ((activePlayer == 1 && code == sf::Keyboard::Key::Space) ||
                        (activePlayer == 2 && code == sf::Keyboard::Key::Space && !p2.isBot)) {
                        faceoffBallLaunched = true;
                        if (activePlayer == 1) p1.ballVel = { 0.f, -15.f }; 
                        else p2.ballVel = { 0.f, 15.f };
                    }
                }

                if (gameState == State::GAME_OVER) {
                    if (code == sf::Keyboard::Key::Enter) { gameState = State::TITLE; }
                    if (code == sf::Keyboard::Key::Space) { resetGame(); gameState = State::BREAKER; }
                }
            }
        }

        // -------- STATE LOGIC --------
        if (gameState == State::TITLE) {
            std::string p1Ctrl = controlsSwapped ? "A/D keys" : "Left/Right arrow keys";
            std::string p2Ctrl = controlsSwapped ? "Left/Right arrow keys" : "A/D keys";
            controlsText.setString("Movement:\nPlayer 1: " + p1Ctrl + "\nPlayer 2: " + p2Ctrl + "\nShoot (Faceoff): Spacebar");
            
            sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
            btnSPRect.setFillColor(btnSPRect.getGlobalBounds().contains(mPos) ? sf::Color(90, 90, 90) : sf::Color(60, 60, 60));
            btnMPRect.setFillColor(btnMPRect.getGlobalBounds().contains(mPos) ? sf::Color(90, 90, 90) : sf::Color(60, 60, 60));
            btnTitleExitRect.setFillColor(btnTitleExitRect.getGlobalBounds().contains(mPos) ? sf::Color(210, 70, 70) : sf::Color(180, 50, 50));
            btnTitleMuteRect.setFillColor(btnTitleMuteRect.getGlobalBounds().contains(mPos) ? sf::Color(110, 110, 110) : sf::Color(80, 80, 80));

            window.clear(sf::Color(8, 8, 16)); 
            for (auto& star : stars) {
                if (std::rand() % 100 < 2) {
                    int alpha = star.getFillColor().a + (std::rand() % 50 - 25);
                    if (alpha < 50) alpha = 50; if (alpha > 255) alpha = 255;
                    star.setFillColor(sf::Color(255, 255, 255, alpha));
                }
                window.draw(star);
            }

            window.draw(titleText); 
            window.draw(btnSPRect); window.draw(btnSPText);
            window.draw(btnMPRect); window.draw(btnMPText);
            window.draw(btnTitleExitRect); window.draw(btnTitleExitText);
            window.draw(btnTitleMuteRect); window.draw(btnTitleMuteText);
            window.draw(controlsText); window.draw(swapText);
            window.display();
            continue;
        }

        if (!isPaused) {
            sf::Keyboard::Key p1Left = controlsSwapped ? sf::Keyboard::Key::A : sf::Keyboard::Key::Left;
            sf::Keyboard::Key p1Right = controlsSwapped ? sf::Keyboard::Key::D : sf::Keyboard::Key::Right;
            sf::Keyboard::Key p2Left = controlsSwapped ? sf::Keyboard::Key::Left : sf::Keyboard::Key::A;
            sf::Keyboard::Key p2Right = controlsSwapped ? sf::Keyboard::Key::Right : sf::Keyboard::Key::D;

            if (gameState == State::BREAKER || gameState == State::WAITING) {
                // P1 Movement
                if (!p1.bricksFinished || gameState == State::WAITING) {
                    if (sf::Keyboard::isKeyPressed(p1Left) && p1.paddle.getPosition().x > 240.f + p1.getCurrentWidth() / 2.f) {
                        p1.paddle.move({ -12.f, 0.f }); 
                        if (p1.ballAttached) { p1.ballAttached = false; p1.ballVel = { -8.f, -8.f }; } 
                    }
                    if (sf::Keyboard::isKeyPressed(p1Right) && p1.paddle.getPosition().x < 1680.f - (p1.getCurrentWidth() / 2.f)) {
                        p1.paddle.move({ 12.f, 0.f });
                        if (p1.ballAttached) { p1.ballAttached = false; p1.ballVel = { 8.f, -8.f }; }
                    }
                }

                // P2 Movement
                if (!isSinglePlayer && (!p2.bricksFinished || gameState == State::WAITING)) {
                    if (!p2.isBot) {
                        if (sf::Keyboard::isKeyPressed(p2Left) && p2.paddle.getPosition().x > 240.f + p2.getCurrentWidth() / 2.f) {
                            p2.paddle.move({ -12.f, 0.f });
                            if (p2.ballAttached) { p2.ballAttached = false; p2.ballVel = { -8.f, 8.f }; }
                        }
                        if (sf::Keyboard::isKeyPressed(p2Right) && p2.paddle.getPosition().x < 1680.f - (p2.getCurrentWidth() / 2.f)) {
                            p2.paddle.move({ 12.f, 0.f });
                            if (p2.ballAttached) { p2.ballAttached = false; p2.ballVel = { 8.f, 8.f }; }
                        }
                    }
                }

                // Ball Positions
                if (p1.ballAttached) p1.ball.setPosition({ p1.paddle.getPosition().x, 1000.f });
                else if (!p1.bricksFinished) p1.ball.move(p1.ballVel);
                
                if (!isSinglePlayer) {
                    if (p2.ballAttached) p2.ball.setPosition({ p2.paddle.getPosition().x, 80.f });
                    else if (!p2.bricksFinished) p2.ball.move(p2.ballVel);
                }

                // Wall Collisions
                if (!p1.bricksFinished) p1.checkWallCollisions(isSinglePlayer, true);
                if (!isSinglePlayer && !p2.bricksFinished) p2.checkWallCollisions(isSinglePlayer, false);

                // Dynamic Paddle Collisions
                if (!p1.bricksFinished) p1.handlePaddleCollision(true);
                if (!isSinglePlayer && !p2.bricksFinished) p2.handlePaddleCollision(false);

                // Life checks
                if (!p1.bricksFinished && p1.ball.getPosition().y > 1080.f) {
                    p1.lives--;
                    if (p1.lives <= 0) { 
                        centerText.setString(isSinglePlayer ? "GAME OVER" : "P2 WINS"); 
                        gameState = State::GAME_OVER; 
                    }
                    else p1.resetBall(1020.f, false);
                }
                
                if (!isSinglePlayer && !p2.bricksFinished && p2.ball.getPosition().y < 0.f) {
                    p2.lives--;
                    if (p2.lives <= 0) { centerText.setString("P1 WINS"); gameState = State::GAME_OVER; }
                    else p2.resetBall(60.f, true);
                }

                // Brick Collisions
                int p1BricksLeft = 0, p2BricksLeft = 0;
                bool p1BouncedThisFrame = false, p2BouncedThisFrame = false;
                
                for (auto& b : bricks) {
                    if (!b.destroyed) {
                        if (b.isPlayer1Side) p1BricksLeft++; else p2BricksLeft++;

                        // Player 1 Brick Collision
                        if (b.isPlayer1Side && !p1.bricksFinished && p1.ball.getGlobalBounds().findIntersection(b.shape.getGlobalBounds())) {
                            b.destroyed = true; 
                            if (!p1BouncedThisFrame) { p1.ballVel.y *= -1; p1BouncedThisFrame = true; }
                            
                            if (b.type == 1) p1.updateSize(-1);
                            else if (b.type == 2) {
                                if (isSinglePlayer) p1.updateSize(1); else p2.updateSize(1);
                            }
                        }
                        
                        // Player 2 Brick Collision
                        if (!isSinglePlayer && !b.isPlayer1Side && !p2.bricksFinished && p2.ball.getGlobalBounds().findIntersection(b.shape.getGlobalBounds())) {
                            b.destroyed = true; 
                            if (!p2BouncedThisFrame) { p2.ballVel.y *= -1; p2BouncedThisFrame = true; }
                            
                            if (b.type == 1) p2.updateSize(-1); else if (b.type == 2) p1.updateSize(1); 
                        }
                    }
                }

                p1.bricksFinished = (p1BricksLeft == 0);
                if (!isSinglePlayer) p2.bricksFinished = (p2BricksLeft == 0);

                // Win Conditions
                if (isSinglePlayer) {
                    if (p1.bricksFinished) {
                        centerText.setString("YOU WIN!"); 
                        gameState = State::GAME_OVER;
                    }
                } else {
                    if (p1.bricksFinished && p2.bricksFinished) {
                        gameState = State::FACEOFF;
                        setFaceoffTurn(p1.lives < p2.lives ? 1 : (p2.lives < p1.lives ? 2 : (rand() % 2 + 1)));
                    } else if (gameState == State::BREAKER && (p1.bricksFinished || p2.bricksFinished)) {
                        gameState = State::WAITING;
                        waitClock.restart();
                    }

                    if (gameState == State::WAITING) {
                        float timeLeft = countdownTime - waitClock.getElapsedTime().asSeconds();
                        centerText.setString("Waiting: " + std::to_string((int)timeLeft));
                        alignTextToPoint(centerText, { 960.f, 500.f });

                        if (timeLeft <= 0) {
                            if (!p1.bricksFinished) p1.lives--;
                            if (!p2.bricksFinished) p2.lives--;
                            if (p1.lives <= 0) { centerText.setString("P2 WINS"); gameState = State::GAME_OVER; }
                            else if (p2.lives <= 0) { centerText.setString("P1 WINS"); gameState = State::GAME_OVER; }
                            else {
                                gameState = State::FACEOFF;
                                setFaceoffTurn(p1.lives < p2.lives ? 1 : (p2.lives < p1.lives ? 2 : (rand() % 2 + 1)));
                            }
                        }
                    }
                }
            } 
            else if (gameState == State::FACEOFF) { 
                centerText.setString("FACEOFF MODE");
                alignTextToPoint(centerText, { 960.f, 450.f });

                if (!faceoffBallLaunched) {
                    if (activePlayer == 1) {
                        p1.paddle.move({ faceoffPaddleSpeed, 0 });
                        if (p1.paddle.getPosition().x > 1680.f - (p1.getCurrentWidth() / 2.f) || p1.paddle.getPosition().x < 240.f + p1.getCurrentWidth() / 2.f) faceoffPaddleSpeed *= -1;
                        p1.ball.setPosition({ p1.paddle.getPosition().x, 1000.f });
                    } else {
                        p2.paddle.move({ faceoffPaddleSpeed, 0 });
                        if (p2.paddle.getPosition().x > 1680.f - (p2.getCurrentWidth() / 2.f) || p2.paddle.getPosition().x < 240.f + p2.getCurrentWidth() / 2.f) faceoffPaddleSpeed *= -1;
                        p2.ball.setPosition({ p2.paddle.getPosition().x, 80.f });

                        if (p2.isBot && abs(p2.paddle.getPosition().x - p1.paddle.getPosition().x) < 15.f) {
                            faceoffBallLaunched = true;
                            p2.ballVel = { 0.f, 15.f }; 
                        }
                    }
                } else {
                    if (activePlayer == 1) {
                        p1.ball.move(p1.ballVel);
                        if (p1.ball.getGlobalBounds().findIntersection(p2.paddle.getGlobalBounds())) {
                            p2.lives--;
                            if (p2.lives <= 0) { centerText.setString("P1 WINS"); gameState = State::GAME_OVER; }
                            else setFaceoffTurn(1); 
                        } else if (p1.ball.getPosition().y < 0) {
                            setFaceoffTurn(2); 
                        }
                    } else {
                        p2.ball.move(p2.ballVel);
                        if (p2.ball.getGlobalBounds().findIntersection(p1.paddle.getGlobalBounds())) {
                            p1.lives--;
                            if (p1.lives <= 0) { centerText.setString("P2 WINS"); gameState = State::GAME_OVER; }
                            else setFaceoffTurn(2);
                        } else if (p2.ball.getPosition().y > 1080.f) {
                            setFaceoffTurn(1);
                        }
                    }
                }
            }
            else if (gameState == State::GAME_OVER) {
                alignTextToPoint(centerText, { 960.f, 400.f });
                
                sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                btnPlayAgainRect.setFillColor(btnPlayAgainRect.getGlobalBounds().contains(mPos) ? sf::Color(70, 210, 70) : sf::Color(50, 180, 50));
                btnGameOverReturnRect.setFillColor(btnGameOverReturnRect.getGlobalBounds().contains(mPos) ? sf::Color(210, 70, 70) : sf::Color(180, 50, 50));
            }
        }

        // -------- RENDERING --------
        window.clear(sf::Color(8, 8, 16)); 

        for (auto& star : stars) {
            if (!isPaused && std::rand() % 100 < 2) { 
                int alpha = star.getFillColor().a + (std::rand() % 50 - 25);
                if (alpha < 50) alpha = 50; if (alpha > 255) alpha = 255;
                star.setFillColor(sf::Color(255, 255, 255, alpha));
            }
            window.draw(star);
        }
        
        if (gameState == State::BREAKER || gameState == State::WAITING) {
            if (!isSinglePlayer) window.draw(dividingLine);
            for (const auto& b : bricks) { if (!b.destroyed) window.draw(b.shape); }
        }

        if (!p1.bricksFinished || gameState == State::FACEOFF) {
            window.draw(p1.paddle);
            if (gameState == State::BREAKER || gameState == State::WAITING || (gameState == State::FACEOFF && activePlayer == 1)) {
                window.draw(p1.ball);
            }
        }

        if (!isSinglePlayer) {
            if (!p2.bricksFinished || gameState == State::FACEOFF) {
                window.draw(p2.paddle);
                if (gameState == State::BREAKER || gameState == State::WAITING || (gameState == State::FACEOFF && activePlayer == 2)) {
                    window.draw(p2.ball);
                }
            }
        }

        window.draw(leftWall);
        window.draw(rightWall);

        p1LivesText.setString("Lives: " + std::to_string(p1.lives));
        p1SizeText.setString("Size: " + std::to_string(p1.sizeLevel));
        window.draw(p1Title); window.draw(p1LivesText); window.draw(p1SizeText);

        if (!isSinglePlayer) {
            p2LivesText.setString("Lives: " + std::to_string(p2.lives));
            p2SizeText.setString("Size: " + std::to_string(p2.sizeLevel));
            window.draw(p2Title); window.draw(p2LivesText); window.draw(p2SizeText);
        }

        if (gameState == State::WAITING || gameState == State::FACEOFF || gameState == State::GAME_OVER) window.draw(centerText);
        
        if (gameState == State::GAME_OVER) {
            window.draw(btnPlayAgainRect); window.draw(btnPlayAgainText);
            window.draw(btnGameOverReturnRect); window.draw(btnGameOverReturnText);
        }

        if (isPaused) {
            sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
            btnReturnRect.setFillColor(btnReturnRect.getGlobalBounds().contains(mPos) ? sf::Color(210, 70, 70) : sf::Color(180, 50, 50));
            btnResumeRect.setFillColor(btnResumeRect.getGlobalBounds().contains(mPos) ? sf::Color(70, 210, 70) : sf::Color(50, 180, 50));
            btnPauseMuteRect.setFillColor(btnPauseMuteRect.getGlobalBounds().contains(mPos) ? sf::Color(110, 110, 110) : sf::Color(80, 80, 80));

            sf::RectangleShape pauseBg({ 650.f, 400.f }); 
            pauseBg.setFillColor(sf::Color(30, 30, 30, 230)); 
            pauseBg.setOutlineThickness(4.f); pauseBg.setOutlineColor(sf::Color::White);
            pauseBg.setPosition({ 635.f, 310.f });

            window.draw(pauseBg); 
            window.draw(pauseText);
            window.draw(btnReturnRect); window.draw(btnReturnText);
            window.draw(btnResumeRect); window.draw(btnResumeText);
            window.draw(btnPauseMuteRect); window.draw(btnPauseMuteText);
        }

        window.display();
    }
    return 0;
}
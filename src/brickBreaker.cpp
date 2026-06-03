#include <SFML/Graphics.hpp>                                     // Imports SFML graphics components (window, shapes, etc.)
#include <SFML/Audio.hpp>                                        // Imports SFML audio components (music, sound)
#include <vector>                                                // Imports standard vector container for dynamic arrays
#include <cstdlib>                                               // Imports C standard library (used for rand, srand)
#include <ctime>                                                 // Imports C time library (used to seed random number generator)
#include <optional>                                              // Imports optional for SFML 3 event polling
#include <string>                                                // Imports standard string manipulation
#include <cmath>                                                 // Imports math functions like std::abs and std::sqrt
#include <iostream>                                              // Imports input/output stream for console warnings

// ===> GAME STATES <===
enum class State { TITLE, BREAKER, WAITING, FACEOFF, GAME_OVER }; // Defines the five distinct modes of the game engine

// ===> PHYSICS HELPER <===
void enforceMinimumVerticalAngle(sf::Vector2f& vel, float speed) { // Prevents the ball from getting stuck in horizontal motion
    float minY = speed * 0.2f;                                   // Calculates a safe minimum vertical speed threshold
    if (std::abs(vel.y) < minY) {                                // Checks if current vertical speed is below the safe threshold
        vel.y = (vel.y > 0) ? minY : -minY;                      // Forces Y speed to the minimum safe limit while keeping direction
        float newMag = std::sqrt(vel.x * vel.x + vel.y * vel.y); // Recalculates the total speed vector magnitude
        vel.x = (vel.x / newMag) * speed;                        // Normalizes and reapplies total speed to X to maintain velocity
        vel.y = (vel.y / newMag) * speed;                        // Normalizes and reapplies total speed to Y to maintain velocity
    }
}

// ==> PLAYER DATA STRUCTURE <===
struct Player {                                                  // A blueprint holding all data and logic for a single player
    sf::RectangleShape paddle;                                   // The visual rectangle representing the player's paddle
    sf::CircleShape ball;                                        // The visual circle representing the player's ball
    int lives = 6;                                               // The starting number of lives for the player
    bool ballAttached = true;                                    // Flag checking if the ball is stuck to the paddle before launch
    sf::Vector2f ballVel{ 0.f, 0.f };                            // The X and Y velocity of the ball
    bool bricksFinished = false;                                 // Flag tracking if the player has destroyed all their bricks
    
    int sizeLevel = 4;                                           // The current size tier of the paddle (1 to 7)

    float getCurrentWidth() const { return sizeLevel * 60.f; }   // Calculates pixel width based on size level

    void updateSize(int deltaLevel) {                            // Modifies the paddle size up or down
        sizeLevel += deltaLevel;                                 // Applies the size change (+1 or -1)
        if (sizeLevel < 1) sizeLevel = 1;                        // Prevents paddle from shrinking below level 1
        if (sizeLevel > 7) sizeLevel = 7;                        // Prevents paddle from growing above level 7
        
        paddle.setSize({ getCurrentWidth(), 20.f });             // Applies the new calculated width to the rectangle shape
        paddle.setOrigin({ getCurrentWidth() / 2.f, 10.f });     // Re-centers the origin point of the shape for proper math
    }

    void resetBall(float startY, bool isTop) {                   // Resets the ball and paddle positions after a life is lost
        ballAttached = true;                                     // Re-attaches the ball to the paddle
        ballVel = { 0.f, 0.f };                                  // Zeroes out the ball's movement speed
        paddle.setPosition({ 960.f, startY });                   // Centers the paddle horizontally at the specified Y level
        float yOffset = isTop ? 20.f : -20.f;                    // Determines if ball should sit below (P2) or above (P1) paddle
        ball.setPosition({ 960.f, startY + yOffset });           // Places the ball exactly on the paddle
    }

    void checkWallCollisions(bool isSinglePlayer, bool isPlayer1) { // Handles ball bouncing off screen boundaries
        if (ball.getPosition().x < 250.f) {                      // If ball hits the left UI panel boundary
            ball.setPosition({250.f, ball.getPosition().y});     // Force ball out of the wall to prevent clipping
            ballVel.x = std::abs(ballVel.x);                     // Reverse horizontal velocity to bounce right
        } else if (ball.getPosition().x > 1670.f) {              // If ball hits the right UI panel boundary
            ball.setPosition({1670.f, ball.getPosition().y});    // Force ball out of the wall to prevent clipping
            ballVel.x = -std::abs(ballVel.x);                    // Reverse horizontal velocity to bounce left
        }
        
        if (isPlayer1) {                                         // If calculating for Player 1 (bottom)
            float ceiling = isSinglePlayer ? 0.f : 540.f;        // Ceiling is top of screen in SP, center line in MP
            if (ball.getPosition().y < ceiling && ballVel.y < 0) ballVel.y *= -1; // Bounce down if ceiling is hit
        } else {                                                 // If calculating for Player 2 (top)
            if (ball.getPosition().y > 540.f && ballVel.y > 0) ballVel.y *= -1; // Bounce up if center line is hit
        }
    }

    void handlePaddleCollision(bool isPlayer1) {                 // Handles dynamic ball-to-paddle bouncing
        if (ball.getGlobalBounds().findIntersection(paddle.getGlobalBounds())) { // Checks if ball and paddle shapes overlap
            float currentSpeed = std::sqrt(ballVel.x * ballVel.x + ballVel.y * ballVel.y); // Calculates absolute current speed
            
            ballVel.y = isPlayer1 ? -std::abs(ballVel.y) : std::abs(ballVel.y); // Force bounce UP for P1, DOWN for P2
            
            float hitOffset = ball.getPosition().x - paddle.getPosition().x; // Calculate distance from paddle center
            float normalizedOffset = hitOffset / (getCurrentWidth() / 2.f); // Create a -1.0 to 1.0 ratio based on hit point
            ballVel.x += normalizedOffset * 4.0f;                // Apply ratio to steer ball left or right based on impact
            
            float newMagnitude = std::sqrt(ballVel.x * ballVel.x + ballVel.y * ballVel.y); // Recalculate speed after steering
            ballVel.x = (ballVel.x / newMagnitude) * currentSpeed; // Re-normalize X to maintain original speed limit
            ballVel.y = (ballVel.y / newMagnitude) * currentSpeed; // Re-normalize Y to maintain original speed limit
            enforceMinimumVerticalAngle(ballVel, currentSpeed);  // Pass through safety check to prevent flatlining
        }
    }
};

// ===> BRICK STRUCTURE <===
struct Brick {                                                   // Blueprint for a destructible wall brick
    sf::RectangleShape shape;                                    // Visual rectangle object
    bool destroyed = false;                                      // Flag tracking if the brick is broken
    int type = 0;                                                // 0=Normal, 1=Bad(Red), 2=Good(Green)
    bool isPlayer1Side = false;                                  // Tracks whose side the brick belongs to
};

// ===> MAIN PROGRAM START <===
int main() {                                                     // Entry point
    std::srand(static_cast<unsigned>(std::time(nullptr)));       // Seeds the random number generator using system time
    
    // ===> FULLSCREEN WINDOW SETUP <===
    sf::RenderWindow window(sf::VideoMode({ 1920, 1080 }), "Paddle Royale", sf::State::Fullscreen); // Create 1080p full screen
    window.setFramerateLimit(60);                                // Lock rendering to 60 frames per second

    sf::Font font;                                               // Create a font object
    if (!font.openFromFile("C:\\Windows\\Fonts\\arial.ttf")) {   // Load standard Arial font
        std::cerr << "Warning: arial.ttf not found.\n";          // Print error to console if font is missing
    }

    // ===> AUDIO SETUP <===
    sf::Music bgMusic;                                           // Create music object
    bool isMuted = false;                                        // Flag to track audio state
    if (!bgMusic.openFromFile("Snowy.mp3")) {                    // Load MP3 from disk
        std::cerr << "Warning: Snowy.mp3 not found.\n";          // Print error if missing
    } else {
        bgMusic.setLooping(true);                                // Set track to loop infinitely
        bgMusic.setVolume(50.f);                                 // Set volume to 50%
        bgMusic.play();                                          // Start music playback
    }

    // ===> STARFIELD BACKGROUND SETUP <==
    std::vector<sf::CircleShape> stars;                          // Array to hold background star objects
    for (int i = 0; i < 250; ++i) {                              // Loop 250 times to generate starfield
        float radius = (std::rand() % 30) / 20.f + 0.5f;         // Calculate random radius size
        sf::CircleShape star(radius);                            // Create circle shape
        star.setPosition({ static_cast<float>(std::rand() % 1920), static_cast<float>(std::rand() % 1080) }); // Random X/Y
        int alpha = std::rand() % 155 + 100;                     // Calculate random starting transparency
        star.setFillColor(sf::Color(255, 255, 255, alpha));      // Apply color and transparency
        stars.push_back(star);                                   // Insert star into array
    }

    // ===> GAME VARIABLES <===
    State gameState = State::TITLE;                              // Initialize game to title screen
    bool isPaused = false;                                       // Flag to track pause state
    bool isSinglePlayer = false;                                 // Flag tracking selected game mode
    bool controlsSwapped = false;                                // Flag to track if keyboard controls are inverted
    
    // ===> UI PANELS (WALLS) <===
    sf::RectangleShape leftWall({ 240.f, 1080.f }); leftWall.setFillColor(sf::Color(40, 40, 40)); // Left dark UI border
    sf::RectangleShape rightWall({ 240.f, 1080.f }); rightWall.setPosition({ 1680.f, 0.f }); rightWall.setFillColor(sf::Color(40, 40, 40)); // Right dark UI border

    // ===> UI TEXTS (PLAYER 1) <===
    sf::Text p1Title(font, "PLAYER 1", 35); p1Title.setPosition({ 40.f, 850.f }); p1Title.setFillColor(sf::Color::Blue); // P1 Name
    sf::Text p1LivesText(font, "Lives: 6", 30); p1LivesText.setPosition({ 40.f, 900.f }); // P1 Lives display
    sf::Text p1SizeText(font, "Size: 4", 25); p1SizeText.setPosition({ 40.f, 950.f });    // P1 Paddle size display
    
    // ===> UI TEXTS (PLAYER 2) <===
    sf::Text p2Title(font, "PLAYER 2", 35); p2Title.setPosition({ 1700.f, 50.f }); p2Title.setFillColor(sf::Color::Green); // P2 Name
    sf::Text p2LivesText(font, "Lives: 6", 30); p2LivesText.setPosition({ 1700.f, 100.f }); // P2 Lives display
    sf::Text p2SizeText(font, "Size: 4", 25); p2SizeText.setPosition({ 1700.f, 150.f });    // P2 Paddle size display
    
    // ===> MISCELLANEOUS UI TEXTS <===
    sf::Text centerText(font, "", 50);                           // Dynamic text for center screen messages (Wins, Timers)
    sf::Text titleText(font, "PADDLE ROYALE", 80); titleText.setPosition({ 650.f, 200.f }); // Main title banner
    sf::Text controlsText(font, "", 30); controlsText.setPosition({ 750.f, 700.f }); // Instructions text
    sf::Text swapText(font, "Press TAB to switch player controls", 25); swapText.setPosition({ 770.f, 900.f }); swapText.setFillColor(sf::Color::Yellow); // Tab hint

    // ===> TITLE MENU BUTTONS <===
    sf::RectangleShape btnSPRect({ 400.f, 70.f }); btnSPRect.setPosition({ 760.f, 380.f }); btnSPRect.setFillColor(sf::Color(60, 60, 60)); // SP Button Background
    sf::Text btnSPText(font, "Single Player Mode", 35); btnSPText.setPosition({ 810.f, 395.f }); // SP Button Label

    sf::RectangleShape btnMPRect({ 400.f, 70.f }); btnMPRect.setPosition({ 760.f, 480.f }); btnMPRect.setFillColor(sf::Color(60, 60, 60)); // MP Button Background
    sf::Text btnMPText(font, "Two Player Mode", 35); btnMPText.setPosition({ 825.f, 495.f }); // MP Button Label

    sf::RectangleShape btnTitleExitRect({ 400.f, 70.f }); btnTitleExitRect.setPosition({ 760.f, 580.f }); btnTitleExitRect.setFillColor(sf::Color(180, 50, 50)); // Exit Button Background
    sf::Text btnTitleExitText(font, "Exit Game", 35); btnTitleExitText.setPosition({ 875.f, 595.f }); // Exit Button Label

    sf::RectangleShape btnTitleMuteRect({ 140.f, 50.f }); btnTitleMuteRect.setPosition({ 1700.f, 980.f }); btnTitleMuteRect.setFillColor(sf::Color(80, 80, 80)); // Mute Button Background
    sf::Text btnTitleMuteText(font, "MUTE", 25);                 // Mute Button Label

    // ===> PAUSE MENU BUTTONS <===
    sf::Text pauseText(font, "PAUSED", 45); pauseText.setPosition({ 870.f, 350.f }); // Pause title
    
    sf::RectangleShape btnReturnRect({ 550.f, 40.f }); btnReturnRect.setPosition({ 685.f, 430.f }); btnReturnRect.setFillColor(sf::Color(180, 50, 50)); // Return to title background
    sf::Text btnReturnText(font, "RETURN TO TITLE SCREEN", 22); btnReturnText.setPosition({ 805.f, 437.f }); // Return to title label

    sf::RectangleShape btnResumeRect({ 350.f, 100.f }); btnResumeRect.setPosition({ 785.f, 500.f }); btnResumeRect.setFillColor(sf::Color(50, 180, 50)); // Resume background
    sf::Text btnResumeText(font, "RESUME", 50); btnResumeText.setPosition({ 860.f, 515.f }); // Resume label

    sf::RectangleShape btnPauseMuteRect({ 250.f, 45.f }); btnPauseMuteRect.setPosition({ 835.f, 630.f }); btnPauseMuteRect.setFillColor(sf::Color(80, 80, 80)); // Pause Mute background
    sf::Text btnPauseMuteText(font, "MUTE AUDIO", 25);           // Pause Mute label

    // ===> GAME OVER BUTTONS <===
    sf::RectangleShape btnPlayAgainRect({ 300.f, 60.f }); btnPlayAgainRect.setPosition({ 810.f, 550.f }); btnPlayAgainRect.setFillColor(sf::Color(50, 180, 50)); // Play again background
    sf::Text btnPlayAgainText(font, "PLAY AGAIN", 30); btnPlayAgainText.setPosition({ 865.f, 560.f }); // Play again label

    sf::RectangleShape btnGameOverReturnRect({ 450.f, 60.f }); btnGameOverReturnRect.setPosition({ 735.f, 640.f }); btnGameOverReturnRect.setFillColor(sf::Color(180, 50, 50)); // Game over return background
    sf::Text btnGameOverReturnText(font, "RETURN TO TITLE SCREEN", 25); btnGameOverReturnText.setPosition({ 790.f, 655.f }); // Game over return label

    // ===> TEXT CENTERING HELPERS (LAMBDAS) <===
    auto alignTextToCenter = [](sf::Text& text, const sf::RectangleShape& bg) { // Aligns text precisely in center of a rect
        sf::FloatRect bounds = text.getLocalBounds();            // Get width/height of the text block
        text.setOrigin({ bounds.position.x + bounds.size.x / 2.0f, // Set X origin to horizontal center
                         bounds.position.y + bounds.size.y / 2.0f }); // Set Y origin to vertical center
        text.setPosition({ bg.getPosition().x + bg.getSize().x / 2.0f, // Match text X position to center of background
                           bg.getPosition().y + bg.getSize().y / 2.0f }); // Match text Y position to center of background
    };

    auto alignTextToPoint = [](sf::Text& text, sf::Vector2f point) { // Aligns text explicitly over a specific coordinate point
        sf::FloatRect bounds = text.getLocalBounds();            // Get width/height of the text block
        text.setOrigin({ bounds.position.x + bounds.size.x / 2.0f, // Set X origin to horizontal center
                         bounds.position.y + bounds.size.y / 2.0f }); // Set Y origin to vertical center
        text.setPosition(point);                                 // Move text exactly to provided coordinates
    };

    // ===> INITIAL ALIGNMENTS <===
    alignTextToCenter(btnTitleMuteText, btnTitleMuteRect);       // Center mute text on title menu
    alignTextToCenter(btnPauseMuteText, btnPauseMuteRect);       // Center mute text on pause menu
    alignTextToPoint(pauseText, { 960.f, 350.f });               // Center the "PAUSED" banner

    // ===> PLAYER INITIALIZATION <===
    Player p1, p2;                                               // Using Player structs for player 1 and player 2
    p1.paddle.setFillColor(sf::Color::Blue); p2.paddle.setFillColor(sf::Color::Green); // Color paddles
    p1.ball.setRadius(10.f); p1.ball.setOrigin({ 10.f, 10.f }); p1.ball.setFillColor(sf::Color::White); // Format P1 ball
    p2.ball.setRadius(10.f); p2.ball.setOrigin({ 10.f, 10.f }); p2.ball.setFillColor(sf::Color::Yellow); // Format P2 ball

    sf::RectangleShape dividingLine({ 1440.f, 3.f });            // Create the center dividing line for multiplayer
    dividingLine.setFillColor(sf::Color::White);                 // Color it white
    dividingLine.setPosition({ 240.f, 540.f });                  // Position it exactly in the middle of screen

    // ===> SYSTEM VARIABLES <===
    std::vector<Brick> bricks;                                   // Array to hold the playable wall
    sf::Clock waitClock;                                         // Stopwatch for Phase transitions
    float countdownTime = 60.f;                                  // Timer duration for Waiting Phase

    int activePlayer = 1;                                        // Tracks whose turn it is during Faceoff Phase
    bool faceoffBallLaunched = false;                            // Checks if ball is attached or flying in Faceoff
    float faceoffPaddleSpeed = 20.f;                             // Speed of the automated paddle in Faceoff

    // ===> GAME LOGIC HELPERS (LAMBDAS) <===
    auto toggleAudio = [&]() {                                   // Toggles the music on or off
        isMuted = !isMuted;                                      // Flip boolean to True
        bgMusic.setVolume(isMuted ? 0.f : 50.f);                 // Adjust volume, if True, then volume is zero, otherwise it is 50%
        btnTitleMuteText.setString(isMuted ? "UNMUTE" : "MUTE"); // Update title menu text
        btnPauseMuteText.setString(isMuted ? "UNMUTE AUDIO" : "MUTE AUDIO"); // Update pause menu text
        alignTextToCenter(btnTitleMuteText, btnTitleMuteRect);   // Re-center updated text
        alignTextToCenter(btnPauseMuteText, btnPauseMuteRect);   // Re-center updated text
    };

    auto resetGame = [&]() {                                     // Completely resets board state for a new game
        p1.sizeLevel = 4; p1.updateSize(0);                      // Reset P1 paddle size
        p1.resetBall(1020.f, false);                             // Reset P1 ball and position
        p1.bricksFinished = false;                               // Reset P1 progress

        if (isSinglePlayer) {                                    // If SP Mode
            p1.lives = 3;                                        // Start with 3 lives
        } else {                                                 // If MP Mode
            p1.lives = 6; p2.lives = 6;                          // Both players get 6 lives
            p2.sizeLevel = 4; p2.updateSize(0);                  // Reset P2 paddle size
            p2.resetBall(60.f, true);                            // Reset P2 ball and position
            p2.bricksFinished = false;                           // Reset P2 progress
        }
        
        bricks.clear();                                          // Wipe existing brick wall
        for (int i = 0; i < 12; ++i) {                           // Loop 12 columns
            for (int j = 0; j < 12; ++j) {                       // Loop 12 rows
                Brick b;                                         // Create temporary brick
                b.shape.setSize({ 120.f, 35.f });                // Set brick size
                b.shape.setOutlineThickness(-3.f);               // Add inner border for visual separation
                b.shape.setOutlineColor(sf::Color(25, 25, 25));  // Color inner border dark gray

                b.isPlayer1Side = isSinglePlayer ? true : (j >= 6); // Assign ownership based on row position
                
                // Unified special brick pattern
                int row = j % 6;                                 // Use modulo to mirror pattern on both halves
                bool isSpecial = false;                          // Flag for special brick spawn
                if (row == 1 && (i == 2 || i == 9)) isSpecial = true; // Hardcoded special spawn points
                if (row == 3 && i == 5) isSpecial = true;        // Hardcoded special spawn points      
                if (row == 4 && (i == 3 || i == 8)) isSpecial = true; // Hardcoded special spawn points

                if (isSpecial) {                                 // If designated as a special block
                    b.type = (std::rand() % 2 == 0) ? 1 : 2;     // Randomly assign it as Good (2) or Bad (1)
                    b.shape.setFillColor((b.type == 1) ? sf::Color::Red : sf::Color::Green); // Color based on type
                } else {                                         // If normal block
                    b.shape.setFillColor(sf::Color::White);      // Color white
                }

                float yPos;                                      // Variable to store vertical position
                if (isSinglePlayer) yPos = j * 35.f + 100.f;     // Map to top in SP
                else if (!b.isPlayer1Side) yPos = j * 35.f + 300.f; // Map P2's half to top in MP
                else yPos = (j - 6) * 35.f + 570.f;              // Map P1's half below center line in MP

                b.shape.setPosition({ i * 120.f + 240.f, yPos }); // Set final calculated screen position
                bricks.push_back(b);                             // Insert brick into main wall array
            }
        }
    };

    auto setFaceoffTurn = [&](int turnPlayer) {                  // Sets up board for the active Faceoff phase
        activePlayer = turnPlayer;                               // Set whose turn to attack it is
        faceoffBallLaunched = false;                             // Lock the ball to the paddle
        if (activePlayer == 1) {                                 // If it's P1's turn
            p1.paddle.setPosition({ 960.f, 1020.f });            // Center P1 paddle
            p2.paddle.setPosition({ static_cast<float>(300 + rand() % 1300), 60.f }); // Randomize P2 target position
            p1.ball.setPosition({ p1.paddle.getPosition().x, 1000.f }); // Attach ball to P1
        } else {                                                 // If it's P2's turn
            p2.paddle.setPosition({ 960.f, 60.f });              // Center P2 paddle
            p1.paddle.setPosition({ static_cast<float>(300 + rand() % 1300), 1020.f }); // Randomize P1 target position
            p2.ball.setPosition({ p2.paddle.getPosition().x, 80.f }); // Attach ball to P2
        }
    };

    // =========================================================================================
    // THE MAIN GAME LOOP
    // =========================================================================================
    while (window.isOpen()) {                                    // Run continuously as long as window is active
        
        // ==========> EVENT HANDLING (SINGLE TRIGGERS) <==========
        while (const std::optional event = window.pollEvent()) { // Query OS for new events
            if (event->is<sf::Event::Closed>()) window.close();  // Close window if OS kill command received

            if (const auto* mouseEvent = event->getIf<sf::Event::MouseButtonPressed>()) { // If mouse clicked
                if (mouseEvent->button == sf::Mouse::Button::Left) { // If left click specifically
                    sf::Vector2f mousePos = window.mapPixelToCoords(mouseEvent->position); // Get mouse X/Y coordinates

                    if (gameState == State::TITLE) {             // If clicking during Title Menu
                        if (btnSPRect.getGlobalBounds().contains(mousePos)) { // Clicked SP Button
                            isSinglePlayer = true; resetGame(); gameState = State::BREAKER; // Launch SP Mode
                        }
                        if (btnMPRect.getGlobalBounds().contains(mousePos)) { // Clicked MP Button
                            isSinglePlayer = false; resetGame(); gameState = State::BREAKER; // Launch MP Mode
                        }
                        if (btnTitleExitRect.getGlobalBounds().contains(mousePos)) { // Clicked Exit Button
                            window.close();                      // Terminate app
                        }
                        if (btnTitleMuteRect.getGlobalBounds().contains(mousePos)) { // Clicked Mute Button
                            toggleAudio();                       // Run toggle audio lambda
                        }
                    } else if (isPaused) {                       // If clicking during Pause Menu
                        if (btnReturnRect.getGlobalBounds().contains(mousePos)) { // Clicked Return Button
                            isPaused = false; gameState = State::TITLE; // Exit to main menu
                        }
                        if (btnResumeRect.getGlobalBounds().contains(mousePos)) { // Clicked Resume Button
                            isPaused = false;                    // Unpause game
                        }
                        if (btnPauseMuteRect.getGlobalBounds().contains(mousePos)) { // Clicked Pause Mute Button
                            toggleAudio();                       // Run toggle audio lambda
                        }
                    } else if (gameState == State::GAME_OVER) {  // If clicking during Game Over screen
                        if (btnPlayAgainRect.getGlobalBounds().contains(mousePos)) { // Clicked Play Again
                            resetGame(); gameState = State::BREAKER; // Restart the level
                        }
                        if (btnGameOverReturnRect.getGlobalBounds().contains(mousePos)) { // Clicked Return
                            gameState = State::TITLE;            // Exit to main menu
                        }
                    }
                }
            }

            if (event->is<sf::Event::KeyPressed>()) {            // If a key is pressed
                auto code = event->getIf<sf::Event::KeyPressed>()->code; // Extract specific key code

                if (code == sf::Keyboard::Key::Escape && gameState != State::TITLE) { // If Escape key pressed outside Title
                    isPaused = !isPaused;                        // Toggle pause state
                }

                if (gameState == State::TITLE) {                 // If pressing keys in Title Menu
                    if (code == sf::Keyboard::Key::Tab) controlsSwapped = !controlsSwapped; // Swap input layout using the Tab key
                }

                if (gameState == State::FACEOFF && !isPaused && !faceoffBallLaunched) { // If waiting to shoot in Faceoff
                    if ((activePlayer == 1 && code == sf::Keyboard::Key::Space) || // If P1 hits Space
                        (activePlayer == 2 && code == sf::Keyboard::Key::Space)) { // Or P2 hits Space
                        faceoffBallLaunched = true;              // Launch the ball
                        if (activePlayer == 1) p1.ballVel = { 0.f, -15.f }; // P1 shoots straight up
                        else p2.ballVel = { 0.f, 15.f };         // P2 shoots straight down
                    }
                }
            }
        }

        // ==========> STATE MACHINE: TITLE SCREEN <==========
        if (gameState == State::TITLE) {                         // Process Title screen logic
            std::string p1Ctrl = controlsSwapped ? "A/D keys" : "Left/Right arrow keys"; // Update P1 instruction string
            std::string p2Ctrl = controlsSwapped ? "Left/Right arrow keys" : "A/D keys"; // Update P2 instruction string
            controlsText.setString("Movement:\nPlayer 1: " + p1Ctrl + "\nPlayer 2: " + p2Ctrl + "\nShoot (Faceoff): Spacebar"); // Combine instructions
            
            sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window)); // Get mouse coordinates for hover effects
            btnSPRect.setFillColor(btnSPRect.getGlobalBounds().contains(mPos) ? sf::Color(90, 90, 90) : sf::Color(60, 60, 60)); // Highlight SP button on hover
            btnMPRect.setFillColor(btnMPRect.getGlobalBounds().contains(mPos) ? sf::Color(90, 90, 90) : sf::Color(60, 60, 60)); // Highlight MP button on hover
            btnTitleExitRect.setFillColor(btnTitleExitRect.getGlobalBounds().contains(mPos) ? sf::Color(210, 70, 70) : sf::Color(180, 50, 50)); // Highlight Exit button on hover
            btnTitleMuteRect.setFillColor(btnTitleMuteRect.getGlobalBounds().contains(mPos) ? sf::Color(110, 110, 110) : sf::Color(80, 80, 80)); // Highlight Mute button on hover

            window.clear(sf::Color(8, 8, 16));                   // Clear screen to space blue
            for (auto& star : stars) {                           // Iterate background stars
                if (std::rand() % 100 < 2) {                     // 2% chance per frame to twinkle
                    int alpha = star.getFillColor().a + (std::rand() % 50 - 25); // Shift transparency up or down
                    if (alpha < 50) alpha = 50; if (alpha > 255) alpha = 255; // Clamp alpha between 50 and 255
                    star.setFillColor(sf::Color(255, 255, 255, alpha)); // Apply new alpha
                }
                window.draw(star);                               // Draw star
            }

            window.draw(titleText);                              // Draw main banner
            window.draw(btnSPRect); window.draw(btnSPText);      // Draw SP button
            window.draw(btnMPRect); window.draw(btnMPText);      // Draw MP button
            window.draw(btnTitleExitRect); window.draw(btnTitleExitText); // Draw Exit button
            window.draw(btnTitleMuteRect); window.draw(btnTitleMuteText); // Draw Mute button
            window.draw(controlsText); window.draw(swapText);    // Draw instructions
            window.display();                                    // Push to monitor
            continue;                                            // Skip the rest of the loop while in Title
        }

        // ==========> MAIN GAME PHYSICS LOGIC <==========
        if (!isPaused) {                                         // Only run physics if game is not paused
            sf::Keyboard::Key p1Left = controlsSwapped ? sf::Keyboard::Key::A : sf::Keyboard::Key::Left; // Set P1 left key
            sf::Keyboard::Key p1Right = controlsSwapped ? sf::Keyboard::Key::D : sf::Keyboard::Key::Right; // Set P1 right key
            sf::Keyboard::Key p2Left = controlsSwapped ? sf::Keyboard::Key::Left : sf::Keyboard::Key::A; // Set P2 left key
            sf::Keyboard::Key p2Right = controlsSwapped ? sf::Keyboard::Key::Right : sf::Keyboard::Key::D; // Set P2 right key

            if (gameState == State::BREAKER || gameState == State::WAITING) { // If in active breakout modes
                
                // ===> P1 Movement Logic <===
                if (!p1.bricksFinished || gameState == State::WAITING) { // Only move if P1 hasn't won their half (or waiting)
                    if (sf::Keyboard::isKeyPressed(p1Left) && p1.paddle.getPosition().x > 240.f + p1.getCurrentWidth() / 2.f) { // If left held & not at boundary
                        p1.paddle.move({ -12.f, 0.f });          // Move P1 paddle left
                        if (p1.ballAttached) { p1.ballAttached = false; p1.ballVel = { -8.f, -8.f }; } // Launch ball if attached
                    }
                    if (sf::Keyboard::isKeyPressed(p1Right) && p1.paddle.getPosition().x < 1680.f - (p1.getCurrentWidth() / 2.f)) { // If right held & not at boundary
                        p1.paddle.move({ 12.f, 0.f });           // Move P1 paddle rights
                        if (p1.ballAttached) { p1.ballAttached = false; p1.ballVel = { 8.f, -8.f }; } // Launch ball if attached
                    }
                }

                // ===> P2 Movement Logic <===
                if (!isSinglePlayer && (!p2.bricksFinished || gameState == State::WAITING)) { // Only move if P2 exists and hasn't won half
                    if (sf::Keyboard::isKeyPressed(p2Left) && p2.paddle.getPosition().x > 240.f + p2.getCurrentWidth() / 2.f) { // If left held & not at boundary
                        p2.paddle.move({ -12.f, 0.f });      // Move P2 paddle left
                        if (p2.ballAttached) { p2.ballAttached = false; p2.ballVel = { -8.f, 8.f }; } // Launch ball if attached
                    }
                    if (sf::Keyboard::isKeyPressed(p2Right) && p2.paddle.getPosition().x < 1680.f - (p2.getCurrentWidth() / 2.f)) { // If right held & not at boundary
                        p2.paddle.move({ 12.f, 0.f });       // Move P2 paddle right
                        if (p2.ballAttached) { p2.ballAttached = false; p2.ballVel = { 8.f, 8.f }; } // Launch ball if attached
                    }
                }

                // ===> Apply Ball Vectors <===
                if (p1.ballAttached) p1.ball.setPosition({ p1.paddle.getPosition().x, 1000.f }); // Pin P1 ball to paddle
                else if (!p1.bricksFinished) p1.ball.move(p1.ballVel); // Or apply velocity vector if detached
                
                if (!isSinglePlayer) {                           // If Multiplayer
                    if (p2.ballAttached) p2.ball.setPosition({ p2.paddle.getPosition().x, 80.f }); // Pin P2 ball to paddle
                    else if (!p2.bricksFinished) p2.ball.move(p2.ballVel); // Or apply velocity vector if detached
                }

                // ===> Engine Physics Execution <===
                if (!p1.bricksFinished) p1.checkWallCollisions(isSinglePlayer, true); // Run P1 wall bounce logic
                if (!isSinglePlayer && !p2.bricksFinished) p2.checkWallCollisions(isSinglePlayer, false); // Run P2 wall bounce logic

                if (!p1.bricksFinished) p1.handlePaddleCollision(true); // Run dynamic P1 paddle bounce logic
                if (!isSinglePlayer && !p2.bricksFinished) p2.handlePaddleCollision(false); // Run dynamic P2 paddle bounce logic

                // ===> Life and Death Checks <===
                if (!p1.bricksFinished && p1.ball.getPosition().y > 1080.f) { // If P1 ball falls below screen
                    p1.lives--;                                  // Deduct P1 life
                    if (p1.lives <= 0) {                         // If P1 out of lives
                        centerText.setString(isSinglePlayer ? "GAME OVER" : "P2 WINS"); // Determine Game Over string
                        gameState = State::GAME_OVER;            // Shift to End State
                    }
                    else p1.resetBall(1020.f, false);            // Else reset P1 position
                }
                
                if (!isSinglePlayer && !p2.bricksFinished && p2.ball.getPosition().y < 0.f) { // If P2 ball flies above screen
                    p2.lives--;                                  // Deduct P2 life
                    if (p2.lives <= 0) { centerText.setString("P1 WINS"); gameState = State::GAME_OVER; } // End game if P2 dies
                    else p2.resetBall(60.f, true);               // Else reset P2 position
                }

                // ===> Brick Collision Processing <===
                int p1BricksLeft = 0, p2BricksLeft = 0;          // Temporary counters for remaining wall health
                bool p1BouncedThisFrame = false, p2BouncedThisFrame = false; // Flags to prevent double-bounces in corners
                
                for (auto& b : bricks) {                         // Iterate through entire wall
                    if (!b.destroyed) {                          // Skip already dead bricks
                        if (b.isPlayer1Side) p1BricksLeft++; else p2BricksLeft++; // Tally remaining bricks for win condition

                        // Player 1 Brick Intersections
                        if (b.isPlayer1Side && !p1.bricksFinished && p1.ball.getGlobalBounds().findIntersection(b.shape.getGlobalBounds())) { // If P1 hits brick
                            b.destroyed = true;                  // Flag brick as dead
                            if (!p1BouncedThisFrame) { p1.ballVel.y *= -1; p1BouncedThisFrame = true; } // Bounce P1 vertically once
                            
                            if (b.type == 1) p1.updateSize(-1);  // If red brick, shrink P1
                            else if (b.type == 2) {              // If green brick
                                if (isSinglePlayer) p1.updateSize(1); else p2.updateSize(1); // Grow P1 in SP, Grow P2 in MP
                            }
                        }
                        
                        // Player 2 Brick Intersections
                        if (!isSinglePlayer && !b.isPlayer1Side && !p2.bricksFinished && p2.ball.getGlobalBounds().findIntersection(b.shape.getGlobalBounds())) { // If P2 hits brick
                            b.destroyed = true;                  // Flag brick as dead
                            if (!p2BouncedThisFrame) { p2.ballVel.y *= -1; p2BouncedThisFrame = true; } // Bounce P2 vertically once
                            
                            if (b.type == 1) p2.updateSize(-1); else if (b.type == 2) p1.updateSize(1); // Shrink P2 on red, Grow P1 on green
                        }
                    }
                }

                // ===> State Transition: Breaker to Waiting/Faceoff <===
                p1.bricksFinished = (p1BricksLeft == 0);         // Check if P1 wall is gone
                if (!isSinglePlayer) p2.bricksFinished = (p2BricksLeft == 0); // Check if P2 wall is gone

                if (isSinglePlayer) {                            // Single player win logic
                    if (p1.bricksFinished) {                     // If P1 cleared board
                        centerText.setString("YOU WIN!");        // Set victory string
                        gameState = State::GAME_OVER;            // Shift to end state
                    }
                } else {                                         // Multiplayer transition logic
                    if (p1.bricksFinished && p2.bricksFinished) { // If both players cleared walls
                        gameState = State::FACEOFF;              // Instantly trigger PVP mode
                        setFaceoffTurn(p1.lives < p2.lives ? 1 : (p2.lives < p1.lives ? 2 : (rand() % 2 + 1))); //  attaPlayer with lower lives attacks first, otherwise a random player will attack
                    } else if (gameState == State::BREAKER && (p1.bricksFinished || p2.bricksFinished)) { // If only ONE cleared wall
                        gameState = State::WAITING;              // Enter Waiting state
                        waitClock.restart();                     // Start the countdown timer
                    }

                    if (gameState == State::WAITING) {           // During the waiting countdown phase
                        float timeLeft = countdownTime - waitClock.getElapsedTime().asSeconds(); // Calculate time remaining
                        centerText.setString("Waiting: " + std::to_string((int)timeLeft)); // Update timer UI string
                        alignTextToPoint(centerText, { 960.f, 500.f }); // Center timer text on screen

                        if (timeLeft <= 0) {                     // Once timer hits zero
                            if (!p1.bricksFinished) p1.lives--;  // Penalize P1 if wall unfinished
                            if (!p2.bricksFinished) p2.lives--;  // Penalize P2 if wall unfinished
                            if (p1.lives <= 0) { centerText.setString("P2 WINS"); gameState = State::GAME_OVER; } // End if P1 dies from penalty
                            else if (p2.lives <= 0) { centerText.setString("P1 WINS"); gameState = State::GAME_OVER; } // End if P2 dies from penalty
                            else {                               // If both survived the penalty
                                gameState = State::FACEOFF;      // Trigger PVP mode
                                setFaceoffTurn(p1.lives < p2.lives ? 1 : (p2.lives < p1.lives ? 2 : (rand() % 2 + 1))); // Setup turns
                            }
                        }
                    }
                }
            } 
            // ==========> STATE MACHINE: FACEOFF (PVP) <==========
            else if (gameState == State::FACEOFF) {              // If in active combat phase
                centerText.setString("FACEOFF MODE");            // Set combat banner text
                alignTextToPoint(centerText, { 960.f, 450.f });  // Align banner

                if (!faceoffBallLaunched) {                      // Pre-launch aiming phase
                    if (activePlayer == 1) {                     // If P1's turn
                        p1.paddle.move({ faceoffPaddleSpeed, 0 }); // Auto-pan P1 paddle
                        if (p1.paddle.getPosition().x > 1680.f - (p1.getCurrentWidth() / 2.f) || p1.paddle.getPosition().x < 240.f + p1.getCurrentWidth() / 2.f) faceoffPaddleSpeed *= -1; // Bounce off UI walls
                        p1.ball.setPosition({ p1.paddle.getPosition().x, 1000.f }); // Keep ball attached while panning
                    } else {                                     // If P2's turn
                        p2.paddle.move({ faceoffPaddleSpeed, 0 }); // Auto-pan P2 paddle
                        if (p2.paddle.getPosition().x > 1680.f - (p2.getCurrentWidth() / 2.f) || p2.paddle.getPosition().x < 240.f + p2.getCurrentWidth() / 2.f) faceoffPaddleSpeed *= -1; // Bounce off UI walls
                        p2.ball.setPosition({ p2.paddle.getPosition().x, 80.f }); // Keep ball attached while panning
                    }
                } else {                                         // Post-launch combat phase
                    if (activePlayer == 1) {                     // P1 projectile flying
                        p1.ball.move(p1.ballVel);                // Move P1 ball
                        if (p1.ball.getGlobalBounds().findIntersection(p2.paddle.getGlobalBounds())) { // Check if it hits P2
                            p2.lives--;                          // Damage P2
                            if (p2.lives <= 0) { centerText.setString("P1 WINS"); gameState = State::GAME_OVER; } // End if P2 dead
                            else setFaceoffTurn(1);              // P1 gets another turn if hit lands
                        } else if (p1.ball.getPosition().y < 0) { // If ball misses and flies off screen
                            setFaceoffTurn(2);                   // Turn passes to P2
                        }
                    } else {                                     // P2 projectile flying
                        p2.ball.move(p2.ballVel);                // Move P2 ball
                        if (p2.ball.getGlobalBounds().findIntersection(p1.paddle.getGlobalBounds())) { // Check if it hits P1
                            p1.lives--;                          // Damage P1
                            if (p1.lives <= 0) { centerText.setString("P2 WINS"); gameState = State::GAME_OVER; } // End if P1 dead
                            else setFaceoffTurn(2);              // P2 gets another turn if hit lands
                        } else if (p2.ball.getPosition().y > 1080.f) { // If ball misses and flies off screen
                            setFaceoffTurn(1);                   // Turn passes to P1
                        }
                    }
                }
            }
            // ========> STATE MACHINE: GAME OVER <==========
            else if (gameState == State::GAME_OVER) {            // End screen logic
                alignTextToPoint(centerText, { 960.f, 400.f });  // Align the win/loss banner
                
                sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window)); // Get mouse for hovers
                btnPlayAgainRect.setFillColor(btnPlayAgainRect.getGlobalBounds().contains(mPos) ? sf::Color(70, 210, 70) : sf::Color(50, 180, 50)); // Highlight replay button if mouse hovers over it
                btnGameOverReturnRect.setFillColor(btnGameOverReturnRect.getGlobalBounds().contains(mPos) ? sf::Color(210, 70, 70) : sf::Color(180, 50, 50)); // Highlight return button if mouse hovers over it
            }
        }

        // ========> RENDERING PHASE <==========
        window.clear(sf::Color(8, 8, 16));                       // Erase previous frame, draw background

        for (auto& star : stars) {                               // Iterate stars
            if (!isPaused && std::rand() % 100 < 2) {            // Apply twinkle if not paused (2% chance)
                int alpha = star.getFillColor().a + (std::rand() % 50 - 25); // Shift alpha
                if (alpha < 50) alpha = 50; if (alpha > 255) alpha = 255; // Clamp alpha transparency between 50 and 255
                star.setFillColor(sf::Color(255, 255, 255, alpha)); // Apply alpha transparency
            }
            window.draw(star);                                   // Draw star
        }
        
        if (gameState == State::BREAKER || gameState == State::WAITING) { // Render Breakout objects
            if (!isSinglePlayer) window.draw(dividingLine);      // Draw center line in MP
            for (const auto& b : bricks) { if (!b.destroyed) window.draw(b.shape); } // Draw alive bricks
        }

        if (!p1.bricksFinished || gameState == State::FACEOFF) { // Render P1 objects
            window.draw(p1.paddle);                              // Draw P1 paddle
            if (gameState == State::BREAKER || gameState == State::WAITING || (gameState == State::FACEOFF && activePlayer == 1)) { // Conditional ball rendering
                window.draw(p1.ball);                            // Draw P1 ball
            }
        }

        if (!isSinglePlayer) {                                   // Render P2 objects in MP
            if (!p2.bricksFinished || gameState == State::FACEOFF) { // If P2 is active
                window.draw(p2.paddle);                          // Draw P2 paddle
                if (gameState == State::BREAKER || gameState == State::WAITING || (gameState == State::FACEOFF && activePlayer == 2)) { // Conditional ball rendering
                    window.draw(p2.ball);                        // Draw P2 ball
                }
            }
        }

        window.draw(leftWall);                                   // Draw left UI panel
        window.draw(rightWall);                                  // Draw right UI panel

        p1LivesText.setString("Lives: " + std::to_string(p1.lives)); // Update P1 life string
        p1SizeText.setString("Size: " + std::to_string(p1.sizeLevel)); // Update P1 size string
        window.draw(p1Title); window.draw(p1LivesText); window.draw(p1SizeText); // Draw P1 stats

        if (!isSinglePlayer) {                                   // Draw P2 UI
            p2LivesText.setString("Lives: " + std::to_string(p2.lives)); // Update P2 life string
            p2SizeText.setString("Size: " + std::to_string(p2.sizeLevel)); // Update P2 size string
            window.draw(p2Title); window.draw(p2LivesText); window.draw(p2SizeText); // Draw P2 stats
        }

        if (gameState == State::WAITING || gameState == State::FACEOFF || gameState == State::GAME_OVER) window.draw(centerText); // Draw large center banners
        
        if (gameState == State::GAME_OVER) {                     // Render Game Over Menu
            window.draw(btnPlayAgainRect); window.draw(btnPlayAgainText); // Draw Play Again button
            window.draw(btnGameOverReturnRect); window.draw(btnGameOverReturnText); // Draw Return button
        }

        if (isPaused) {                                          // Render Pause Menu overlay
            sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window)); // Get mouse for hovers
            btnReturnRect.setFillColor(btnReturnRect.getGlobalBounds().contains(mPos) ? sf::Color(210, 70, 70) : sf::Color(180, 50, 50)); // Highlight Return
            btnResumeRect.setFillColor(btnResumeRect.getGlobalBounds().contains(mPos) ? sf::Color(70, 210, 70) : sf::Color(50, 180, 50)); // Highlight Resume
            btnPauseMuteRect.setFillColor(btnPauseMuteRect.getGlobalBounds().contains(mPos) ? sf::Color(110, 110, 110) : sf::Color(80, 80, 80)); // Highlight Mute

            sf::RectangleShape pauseBg({ 650.f, 400.f });        // Create semi-transparent overlay
            pauseBg.setFillColor(sf::Color(30, 30, 30, 230));    // Set dark color and transparency to 230 for semi-transparent overlay
            pauseBg.setOutlineThickness(4.f); pauseBg.setOutlineColor(sf::Color::White); // Set white border
            pauseBg.setPosition({ 635.f, 310.f });               // Position overlay

            window.draw(pauseBg);                                // Draw overlay
            window.draw(pauseText);                              // Draw PAUSED text
            window.draw(btnReturnRect); window.draw(btnReturnText); // Draw Return button
            window.draw(btnResumeRect); window.draw(btnResumeText); // Draw Resume button
            window.draw(btnPauseMuteRect); window.draw(btnPauseMuteText); // Draw Mute button
        }

        window.display();                                        // Push final composite image to monitor
    }
    return 0;                                                    // Safely close C++ application
}
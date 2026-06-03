// ============================================================
//  BRICK BREAKER DUO — Main Source File
//  A two-player (or one-player) split-screen brick breaker
//  built with SFML.
//
//  Screens / States:
//    StartScreen  → Choose 1 or 2 players, swap controls, mute
//    Settings     → FPS slider (60–120), back button
//    Playing      → Actual brick breaker game
//    GameOver     → Shows who lost, Try Again / Main Menu buttons
//
//  Controls (default, 2-player):
//    Player 1 (bottom) : LEFT / RIGHT arrow keys
//    Player 2 (top)    : A / D keys
//    TAB               : Swap controls (start screen only)
//    M                 : Toggle music mute (all screens)
//
//  1-Player mode:
//    Only the bottom paddle (P1) is active.
//    The top is a simple CPU paddle that follows ball 2.
//    Ball 2 still exists; if it escapes the top the CPU "loses"
//    (i.e. it resets rather than ending the game).
//
//  Music: Place "undertale.ogg" (or "music.ogg") next to the
//         executable to enable background music.
// ============================================================

#include <SFML/Graphics.hpp>   // Window, shapes, text, rendering
#include <SFML/Audio.hpp>      // Music / sound support
#include <vector>              // std::vector for bricks & stars
#include <optional>            // std::optional for event polling
#include <algorithm>           // std::shuffle, std::clamp
#include <random>              // std::random_device, std::mt19937
#include <string>              // std::string for HUD labels
#include <cmath>               // std::sin, std::abs
#include <cstdint>             // uint8_t

int main() {

    // ----------------------------------------------------------
    //  WINDOW SETUP  —  1920 × 1080 fullscreen-style window
    // ----------------------------------------------------------
    sf::RenderWindow window(sf::VideoMode({1920, 960}), "Brick Breaker Duo");
    window.setFramerateLimit(60); // Initial FPS cap — changed by settings slider

    // Convenience scale factors so all original 800×600 coordinates
    // are mapped to the 1920×960 viewport automatically.
    const float SX = 1920.f / 800.f; // Horizontal scale  (2.4×)
    const float SY =  960.f / 600.f; // Vertical scale    (1.6×)

    // Helper: scale a point from "design space" (800×600) to screen space
    auto sp = [&](float x, float y) -> sf::Vector2f {
        return {x * SX, y * SY};
    };
    // Helper: scale a size
    auto ss = [&](float w, float h) -> sf::Vector2f {
        return {w * SX, h * SY};
    };

    // ----------------------------------------------------------
    //  FONT LOADING
    // ----------------------------------------------------------
    sf::Font font;
    bool fontLoaded = font.openFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf");
    if (!fontLoaded) fontLoaded = font.openFromFile("/System/Library/Fonts/Helvetica.ttc");
    if (!fontLoaded) fontLoaded = font.openFromFile("C:/Windows/Fonts/arial.ttf");

    // ----------------------------------------------------------
    //  BACKGROUND — STARFIELD  (shared across all screens)
    // ----------------------------------------------------------
    std::random_device rdBg;
    std::mt19937 gBg(rdBg());
    std::uniform_real_distribution<float> distX(0.f, 1920.f);
    std::uniform_real_distribution<float> distY(0.f,  960.f);
    std::uniform_int_distribution<int>    distSize(1, 4);

    std::vector<sf::CircleShape> stars;
    for (int i = 0; i < 200; ++i) {
        sf::CircleShape star(static_cast<float>(distSize(gBg)));
        star.setFillColor(sf::Color(255, 255, 255, 100 + (gBg() % 155)));
        star.setPosition({distX(gBg), distY(gBg)});
        stars.push_back(star);
    }

    sf::RectangleShape bgTop(ss(800.f, 300.f));
    bgTop.setFillColor(sf::Color(10, 5, 35));
    bgTop.setPosition(sp(0.f, 0.f));

    sf::RectangleShape bgBottom(ss(800.f, 300.f));
    bgBottom.setFillColor(sf::Color(5, 20, 50));
    bgBottom.setPosition(sp(0.f, 300.f));

    // Helper: draw the shared background every frame
    auto drawBackground = [&]() {
        window.draw(bgTop);
        window.draw(bgBottom);
        for (const auto& s : stars) window.draw(s);
    };

    // ----------------------------------------------------------
    //  AUDIO
    // ----------------------------------------------------------
    sf::Music music;
    bool musicLoaded = music.openFromFile("Snowy.mp3");
    if (!musicLoaded) musicLoaded = music.openFromFile("music.ogg");
    if (musicLoaded) { music.setLooping(true); music.setVolume(50.f); music.play(); }
    bool isMuted = false;

    // ----------------------------------------------------------
    //  GAME STATE
    // ----------------------------------------------------------
    enum class GameState { StartScreen, Settings, Playing, GameOver };
    GameState gameState = GameState::StartScreen;

    // ----------------------------------------------------------
    //  PLAYER MODE
    // ----------------------------------------------------------
    int  playerMode    = 2;    // 1 or 2 players — chosen on start screen
    bool controlsSwapped = false;

    // ----------------------------------------------------------
    //  FPS SETTING  (60–120, step 1, controlled by slider)
    // ----------------------------------------------------------
    int  targetFPS     = 60;   // Current FPS cap
    int  minFPS        = 60;
    int  maxFPS        = 120;

    // ----------------------------------------------------------
    //  GAME-OVER INFO
    // ----------------------------------------------------------
    std::string loserName; // "Player 1" or "Player 2" or "CPU"

    // ----------------------------------------------------------
    //  HELPER: centre a text object horizontally on the screen
    // ----------------------------------------------------------
    auto centreH = [&](sf::Text& t, float y) {
        sf::FloatRect b = t.getLocalBounds();
        t.setOrigin({b.size.x / 2.f, 0.f});
        t.setPosition({960.f, y}); // 960 = half of 1920
    };

    // ============================================================
    //  LAMBDA: build control label strings
    // ============================================================
    auto makeControlsText = [&](bool swapped) -> std::pair<std::string, std::string> {
        if (!swapped) return {"P1: LEFT / RIGHT", "P2: A / D"};
        else          return {"P1: A / D",        "P2: LEFT / RIGHT"};
    };

    // ============================================================
    //  ——— START SCREEN ELEMENTS ———
    // ============================================================
    sf::Text startTitle(font, "BRICK BREAKER DUO", static_cast<unsigned int>(80 * SY / 1.6f));
    startTitle.setFillColor(sf::Color(220, 220, 255));
    startTitle.setStyle(sf::Text::Bold);
    centreH(startTitle, 100.f * SY);

    sf::RectangleShape startDivider({900.f * SX / 2.4f, 3.f});
    startDivider.setFillColor(sf::Color(150, 150, 220, 180));
    startDivider.setOrigin({startDivider.getSize().x / 2.f, 1.f});
    startDivider.setPosition({960.f, 230.f * SY});

    sf::Text startSubtitle(font, "Choose your mode", static_cast<unsigned int>(28 * SY / 1.6f));
    startSubtitle.setFillColor(sf::Color(160, 160, 200));
    centreH(startSubtitle, 245.f * SY);

    // --- 1P / 2P buttons ---
    // Button backgrounds
    sf::RectangleShape btn1P(ss(160.f, 55.f));
    btn1P.setFillColor(sf::Color(40, 80, 180));
    btn1P.setOutlineThickness(3.f);
    btn1P.setOutlineColor(sf::Color(100, 150, 255));
    btn1P.setOrigin({btn1P.getSize().x / 2.f, btn1P.getSize().y / 2.f});
    btn1P.setPosition({720.f * SX / 2.4f, 340.f * SY});

    sf::RectangleShape btn2P(ss(160.f, 55.f));
    btn2P.setFillColor(sf::Color(40, 80, 180));
    btn2P.setOutlineThickness(3.f);
    btn2P.setOutlineColor(sf::Color(100, 150, 255));
    btn2P.setOrigin({btn2P.getSize().x / 2.f, btn2P.getSize().y / 2.f});
    btn2P.setPosition({1080.f * SX / 2.4f, 340.f * SY});

    sf::Text lbl1P(font, "1 PLAYER", static_cast<unsigned int>(22 * SY / 1.6f));
    lbl1P.setFillColor(sf::Color::White);
    lbl1P.setStyle(sf::Text::Bold);
    { sf::FloatRect b = lbl1P.getLocalBounds(); lbl1P.setOrigin({b.size.x/2.f, b.size.y/2.f}); }
    lbl1P.setPosition(btn1P.getPosition());

    sf::Text lbl2P(font, "2 PLAYERS", static_cast<unsigned int>(22 * SY / 1.6f));
    lbl2P.setFillColor(sf::Color::White);
    lbl2P.setStyle(sf::Text::Bold);
    { sf::FloatRect b = lbl2P.getLocalBounds(); lbl2P.setOrigin({b.size.x/2.f, b.size.y/2.f}); }
    lbl2P.setPosition(btn2P.getPosition());

    // Controls display on start screen
    sf::Text startP1Label(font, "", static_cast<unsigned int>(26 * SY / 1.6f));
    startP1Label.setFillColor(sf::Color(100, 180, 255));

    sf::Text startP2Label(font, "", static_cast<unsigned int>(26 * SY / 1.6f));
    startP2Label.setFillColor(sf::Color(100, 255, 140));

    sf::Text startSwapHint(font, "[ TAB ] to swap controls", static_cast<unsigned int>(20 * SY / 1.6f));
    startSwapHint.setFillColor(sf::Color(180, 180, 180));
    centreH(startSwapHint, 460.f * SY);

    // Pulsing prompt
    sf::Text startPrompt(font, "Press  ENTER  or  SPACE  to  Start", static_cast<unsigned int>(30 * SY / 1.6f));
    startPrompt.setFillColor(sf::Color(255, 220, 80));
    startPrompt.setStyle(sf::Text::Bold);
    centreH(startPrompt, 530.f * SY);

    // Mute hint
    sf::Text startMuteHint(font, "[ M ]  Mute / Unmute", static_cast<unsigned int>(18 * SY / 1.6f));
    startMuteHint.setFillColor(sf::Color(160, 160, 160));
    { sf::FloatRect b = startMuteHint.getLocalBounds();
      startMuteHint.setOrigin({b.size.x, 0.f});
      startMuteHint.setPosition({1900.f, 920.f * SY}); }

    // Settings button on start screen
    sf::RectangleShape btnSettings(ss(140.f, 45.f));
    btnSettings.setFillColor(sf::Color(50, 50, 100));
    btnSettings.setOutlineThickness(2.f);
    btnSettings.setOutlineColor(sf::Color(120, 120, 200));
    btnSettings.setPosition({20.f, 20.f});

    sf::Text lblSettings(font, "SETTINGS", static_cast<unsigned int>(20 * SY / 1.6f));
    lblSettings.setFillColor(sf::Color(200, 200, 255));
    { sf::FloatRect b = lblSettings.getLocalBounds();
      lblSettings.setOrigin({b.size.x/2.f, b.size.y/2.f});
      lblSettings.setPosition({btnSettings.getPosition().x + btnSettings.getSize().x/2.f,
                                btnSettings.getPosition().y + btnSettings.getSize().y/2.f}); }

    sf::Clock pulseClock;

    // ============================================================
    //  ——— SETTINGS SCREEN ELEMENTS ———
    // ============================================================
    sf::Text settingsTitle(font, "SETTINGS", static_cast<unsigned int>(60 * SY / 1.6f));
    settingsTitle.setFillColor(sf::Color(220, 220, 255));
    settingsTitle.setStyle(sf::Text::Bold);
    centreH(settingsTitle, 80.f * SY);

    // FPS slider track
    float sliderTrackX     = 560.f * SX / 2.4f;  // left edge X
    float sliderTrackY     = 340.f * SY;
    float sliderTrackW     = 800.f * SX / 2.4f;
    float sliderTrackH     =   8.f * SY;

    sf::RectangleShape sliderTrack({sliderTrackW, sliderTrackH});
    sliderTrack.setFillColor(sf::Color(80, 80, 140));
    sliderTrack.setOutlineThickness(2.f);
    sliderTrack.setOutlineColor(sf::Color(150, 150, 220));
    sliderTrack.setPosition({sliderTrackX, sliderTrackY});

    // Slider knob (draggable circle)
    float sliderKnobRadius = 18.f * SY / 1.6f;
    sf::CircleShape sliderKnob(sliderKnobRadius);
    sliderKnob.setFillColor(sf::Color(100, 180, 255));
    sliderKnob.setOutlineThickness(3.f);
    sliderKnob.setOutlineColor(sf::Color::White);
    sliderKnob.setOrigin({sliderKnobRadius, sliderKnobRadius});

    // Helper: reposition knob based on current targetFPS
    auto updateSliderKnob = [&]() {
        float t = float(targetFPS - minFPS) / float(maxFPS - minFPS);
        float knobX = sliderTrackX + t * sliderTrackW;
        float knobY = sliderTrackY + sliderTrackH / 2.f;
        sliderKnob.setPosition({knobX, knobY});
    };
    updateSliderKnob();

    bool sliderDragging = false; // true while mouse button is held on knob

    sf::Text fpsLabel(font, "", static_cast<unsigned int>(28 * SY / 1.6f));
    fpsLabel.setFillColor(sf::Color(220, 220, 255));
    centreH(fpsLabel, 280.f * SY);

    sf::Text fpsMinLabel(font, "60", static_cast<unsigned int>(22 * SY / 1.6f));
    fpsMinLabel.setFillColor(sf::Color(160, 160, 160));
    fpsMinLabel.setPosition({sliderTrackX - 40.f, sliderTrackY - 6.f});

    sf::Text fpsMaxLabel(font, "120", static_cast<unsigned int>(22 * SY / 1.6f));
    fpsMaxLabel.setFillColor(sf::Color(160, 160, 160));
    fpsMaxLabel.setPosition({sliderTrackX + sliderTrackW + 10.f, sliderTrackY - 6.f});

    // Helper: refresh fpsLabel string
    auto updateFpsLabel = [&]() {
        fpsLabel.setString("FPS Cap:  " + std::to_string(targetFPS));
        centreH(fpsLabel, 280.f * SY);
    };
    updateFpsLabel();

    // Back button (settings → start screen)
    sf::RectangleShape btnBack(ss(140.f, 45.f));
    btnBack.setFillColor(sf::Color(80, 30, 30));
    btnBack.setOutlineThickness(2.f);
    btnBack.setOutlineColor(sf::Color(220, 80, 80));
    btnBack.setPosition({20.f, 20.f});

    sf::Text lblBack(font, "< BACK", static_cast<unsigned int>(20 * SY / 1.6f));
    lblBack.setFillColor(sf::Color(255, 160, 160));
    { sf::FloatRect b = lblBack.getLocalBounds();
      lblBack.setOrigin({b.size.x/2.f, b.size.y/2.f});
      lblBack.setPosition({btnBack.getPosition().x + btnBack.getSize().x/2.f,
                            btnBack.getPosition().y + btnBack.getSize().y/2.f}); }

    // ============================================================
    //  ——— IN-GAME HUD ELEMENTS ———
    // ============================================================
    sf::Text titleText(font, "BRICK BREAKER DUO", static_cast<unsigned int>(36 * SY / 1.6f));
    titleText.setFillColor(sf::Color(220, 220, 255));
    titleText.setStyle(sf::Text::Bold);
    centreH(titleText, 4.f * SY);

    sf::Text p1ControlsText(font, "", static_cast<unsigned int>(18 * SY / 1.6f));
    p1ControlsText.setFillColor(sf::Color(100, 180, 255));
    p1ControlsText.setPosition(sp(8.f, 575.f));

    sf::Text p2ControlsText(font, "", static_cast<unsigned int>(18 * SY / 1.6f));
    p2ControlsText.setFillColor(sf::Color(100, 255, 140));
    p2ControlsText.setPosition(sp(8.f, 560.f));

    sf::Text swapHintText(font, "[TAB] Swap Controls", static_cast<unsigned int>(16 * SY / 1.6f));
    swapHintText.setFillColor(sf::Color(180, 180, 180));
    swapHintText.setPosition(sp(8.f, 545.f));

    sf::Text muteText(font, "[M] Mute", static_cast<unsigned int>(16 * SY / 1.6f));
    muteText.setFillColor(sf::Color(180, 180, 180));
    { sf::FloatRect b = muteText.getLocalBounds();
      muteText.setOrigin({b.size.x, 0.f});
      muteText.setPosition(sp(792.f, 4.f)); }

    sf::Text muteIndicator(font, "", static_cast<unsigned int>(16 * SY / 1.6f));
    muteIndicator.setFillColor(sf::Color(255, 80, 80));
    muteIndicator.setPosition(sp(680.f, 18.f));

    // CPU label (shown top-centre in 1-player mode)
    sf::Text cpuLabel(font, "CPU", static_cast<unsigned int>(22 * SY / 1.6f));
    cpuLabel.setFillColor(sf::Color(255, 180, 80));
    { sf::FloatRect b = cpuLabel.getLocalBounds();
      cpuLabel.setOrigin({b.size.x/2.f, 0.f});
      cpuLabel.setPosition({960.f, 30.f * SY}); }

    // ============================================================
    //  UNIFIED CONTROLS UPDATE LAMBDA
    // ============================================================
    auto updateControlsDisplay = [&]() {
        auto [p1, p2] = makeControlsText(controlsSwapped);
        p1ControlsText.setString(p1);
        p2ControlsText.setString(p2);

        startP1Label.setString(p1);
        { sf::FloatRect b = startP1Label.getLocalBounds();
          startP1Label.setOrigin({b.size.x/2.f, 0.f});
          startP1Label.setPosition({960.f, 390.f * SY}); }

        startP2Label.setString(p2);
        { sf::FloatRect b = startP2Label.getLocalBounds();
          startP2Label.setOrigin({b.size.x/2.f, 0.f});
          startP2Label.setPosition({960.f, 425.f * SY}); }
    };
    updateControlsDisplay();

    // Helper: highlight the selected mode button
    auto updateModeButtons = [&]() {
        btn1P.setFillColor(playerMode == 1 ? sf::Color(20, 120, 255) : sf::Color(40, 80, 180));
        btn2P.setFillColor(playerMode == 2 ? sf::Color(20, 120, 255) : sf::Color(40, 80, 180));
        btn1P.setOutlineColor(playerMode == 1 ? sf::Color::White : sf::Color(100, 150, 255));
        btn2P.setOutlineColor(playerMode == 2 ? sf::Color::White : sf::Color(100, 150, 255));
    };
    updateModeButtons();

    // ============================================================
    //  ——— GAME-OVER SCREEN ELEMENTS ———
    // ============================================================
    sf::Text gameOverTitle(font, "", static_cast<unsigned int>(60 * SY / 1.6f));
    gameOverTitle.setFillColor(sf::Color(255, 80, 80));
    gameOverTitle.setStyle(sf::Text::Bold);

    sf::Text gameOverSub(font, "", static_cast<unsigned int>(30 * SY / 1.6f));
    gameOverSub.setFillColor(sf::Color(220, 180, 80));

    // Try Again button
    sf::RectangleShape btnTryAgain(ss(200.f, 60.f));
    btnTryAgain.setFillColor(sf::Color(30, 120, 60));
    btnTryAgain.setOutlineThickness(3.f);
    btnTryAgain.setOutlineColor(sf::Color(80, 220, 120));
    btnTryAgain.setOrigin({btnTryAgain.getSize().x/2.f, btnTryAgain.getSize().y/2.f});
    btnTryAgain.setPosition({700.f * SX / 2.4f, 500.f * SY});

    sf::Text lblTryAgain(font, "TRY AGAIN", static_cast<unsigned int>(26 * SY / 1.6f));
    lblTryAgain.setFillColor(sf::Color::White);
    lblTryAgain.setStyle(sf::Text::Bold);
    { sf::FloatRect b = lblTryAgain.getLocalBounds();
      lblTryAgain.setOrigin({b.size.x/2.f, b.size.y/2.f});
      lblTryAgain.setPosition(btnTryAgain.getPosition()); }

    // Main Menu button
    sf::RectangleShape btnMainMenu(ss(200.f, 60.f));
    btnMainMenu.setFillColor(sf::Color(100, 40, 140));
    btnMainMenu.setOutlineThickness(3.f);
    btnMainMenu.setOutlineColor(sf::Color(200, 100, 255));
    btnMainMenu.setOrigin({btnMainMenu.getSize().x/2.f, btnMainMenu.getSize().y/2.f});
    btnMainMenu.setPosition({1100.f * SX / 2.4f, 500.f * SY});

    sf::Text lblMainMenu(font, "MAIN MENU", static_cast<unsigned int>(26 * SY / 1.6f));
    lblMainMenu.setFillColor(sf::Color::White);
    lblMainMenu.setStyle(sf::Text::Bold);
    { sf::FloatRect b = lblMainMenu.getLocalBounds();
      lblMainMenu.setOrigin({b.size.x/2.f, b.size.y/2.f});
      lblMainMenu.setPosition(btnMainMenu.getPosition()); }

    // ============================================================
    //  ——— GAME OBJECTS ———
    // ============================================================

    // --- Paddles ---
    sf::RectangleShape paddle(ss(100.f, 20.f));
    paddle.setFillColor(sf::Color(60, 120, 255));
    paddle.setOrigin({paddle.getSize().x/2.f, paddle.getSize().y/2.f});
    paddle.setPosition(sp(400.f, 550.f));

    sf::RectangleShape paddle2(ss(100.f, 20.f));
    paddle2.setFillColor(sf::Color(60, 220, 100));
    paddle2.setOrigin({paddle2.getSize().x/2.f, paddle2.getSize().y/2.f});
    paddle2.setPosition(sp(400.f, 50.f));

    // --- Dividing line ---
    sf::RectangleShape dividingLine(ss(800.f, 3.f));
    dividingLine.setFillColor(sf::Color(200, 200, 200, 160));
    dividingLine.setPosition(sp(0.f, 300.f));

    // --- Ball 1 (P1, white) ---
    float ballR = 10.f * SX / 2.4f;
    sf::CircleShape ball(ballR);
    ball.setFillColor(sf::Color::White);
    ball.setOrigin({ballR, ballR});
    ball.setPosition(sp(400.f, 530.f));
    sf::Vector2f ballVelocity{4.f * SX / 2.4f, -4.f * SY / 1.6f};

    // --- Ball 2 (P2 / CPU, yellow) ---
    sf::CircleShape ball2(ballR);
    ball2.setFillColor(sf::Color::Yellow);
    ball2.setOrigin({ballR, ballR});
    ball2.setPosition(sp(400.f, 70.f));
    sf::Vector2f ballVelocity2{4.f * SX / 2.4f, 4.f * SY / 1.6f};

    // --- Bricks ---
    struct Brick {
        sf::RectangleShape shape;
        bool destroyed = false;
    };
    std::vector<Brick> bricks;

    std::vector<sf::Color> topColors    = {sf::Color::Green, sf::Color::Green, sf::Color::Red, sf::Color::Red};
    std::vector<sf::Color> bottomColors = {sf::Color::Green, sf::Color::Green, sf::Color::Red, sf::Color::Red};

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(topColors.begin(),    topColors.end(),    g);
    std::shuffle(bottomColors.begin(), bottomColors.end(), g);

    int topIndex = 0, bottomIndex = 0;

    // Helper: (re)build the brick grid — called at start and on Try Again
    auto buildBricks = [&]() {
        bricks.clear();
        topIndex = 0; bottomIndex = 0;
        std::shuffle(topColors.begin(),    topColors.end(),    g);
        std::shuffle(bottomColors.begin(), bottomColors.end(), g);

        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 6; ++j) {
                Brick b;
                b.shape.setSize(ss(90.f, 30.f));
                b.shape.setFillColor(sf::Color(200, 200, 200));

                bool isTop = (j < 3);
                int  row   = isTop ? j : j - 3;

                bool isSpecial = false;
                if (row == 1 && (i == 0 || i == 7)) isSpecial = true;
                if (row == 0 &&  i == 3)             isSpecial = true;
                if (row == 2 &&  i == 3)             isSpecial = true;

                if (isSpecial) {
                    if (isTop) b.shape.setFillColor(topColors[topIndex++]);
                    else       b.shape.setFillColor(bottomColors[bottomIndex++]);
                }

                b.shape.setOutlineThickness(2.f);
                b.shape.setOutlineColor(sf::Color::Black);

                if (j < 3)
                    b.shape.setPosition(sp(i * 100.f + 5.f, j * 35.f + 180.f));
                else
                    b.shape.setPosition(sp(i * 100.f + 5.f, (j - 3) * 35.f + 350.f));

                bricks.push_back(b);
            }
        }
    };
    buildBricks();

    // Helper: reset ball & paddle positions for a fresh game
    auto resetGame = [&]() {
        paddle.setPosition(sp(400.f, 550.f));
        paddle2.setPosition(sp(400.f, 50.f));
        ball.setPosition(sp(400.f, 530.f));
        ball2.setPosition(sp(400.f, 70.f));
        ballVelocity  = {4.f * SX / 2.4f, -4.f * SY / 1.6f};
        ballVelocity2 = {4.f * SX / 2.4f,  4.f * SY / 1.6f};
        buildBricks();
    };

    // Clamp helpers for paddle X boundaries
    const float paddleHalfW = paddle.getSize().x / 2.f;
    const float paddleMinX  = paddleHalfW + 10.f * SX / 2.4f;
    const float paddleMaxX  = 1920.f - paddleHalfW - 10.f * SX / 2.4f;

    // ============================================================
    //  MAIN GAME LOOP
    // ============================================================
    while (window.isOpen()) {

        // ----------------------------------------------------------
        //  EVENT HANDLING  (shared across all states)
        // ----------------------------------------------------------
        while (const std::optional event = window.pollEvent()) {

            if (event->is<sf::Event::Closed>())
                window.close();

            // ---- Mouse button press ----
            if (const auto* mb = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mb->button == sf::Mouse::Button::Left) {
                    sf::Vector2f mp(static_cast<float>(mb->position.x),
                                    static_cast<float>(mb->position.y));

                    // --- Start screen clicks ---
                    if (gameState == GameState::StartScreen) {
                        // 1P button
                        if (btn1P.getGlobalBounds().contains(mp)) {
                            playerMode = 1; updateModeButtons();
                        }
                        // 2P button
                        if (btn2P.getGlobalBounds().contains(mp)) {
                            playerMode = 2; updateModeButtons();
                        }
                        // Settings button
                        if (btnSettings.getGlobalBounds().contains(mp)) {
                            gameState = GameState::Settings;
                        }
                    }

                    // --- Settings screen clicks ---
                    if (gameState == GameState::Settings) {
                        // Back button
                        if (btnBack.getGlobalBounds().contains(mp)) {
                            gameState = GameState::StartScreen;
                        }
                        // Start dragging slider knob
                        if (sliderKnob.getGlobalBounds().contains(mp)) {
                            sliderDragging = true;
                        }
                    }

                    // --- Game-over screen clicks ---
                    if (gameState == GameState::GameOver) {
                        if (btnTryAgain.getGlobalBounds().contains(mp)) {
                            resetGame();
                            gameState = GameState::Playing;
                        }
                        if (btnMainMenu.getGlobalBounds().contains(mp)) {
                            resetGame();
                            gameState = GameState::StartScreen;
                        }
                    }
                }
            }

            // ---- Mouse button release ----
            if (const auto* mr = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (mr->button == sf::Mouse::Button::Left)
                    sliderDragging = false;
            }

            // ---- Mouse move — drag slider ----
            if (const auto* mm = event->getIf<sf::Event::MouseMoved>()) {
                if (sliderDragging && gameState == GameState::Settings) {
                    float mx = static_cast<float>(mm->position.x);
                    float t  = (mx - sliderTrackX) / sliderTrackW;
                    t = std::clamp(t, 0.f, 1.f);
                    targetFPS = minFPS + static_cast<int>(std::round(t * float(maxFPS - minFPS)));
                    window.setFramerateLimit(static_cast<unsigned int>(targetFPS));
                    updateSliderKnob();
                    updateFpsLabel();
                }
            }

            // ---- Key presses ----
            if (const auto* ke = event->getIf<sf::Event::KeyPressed>()) {

                // M — mute, works everywhere
                if (ke->code == sf::Keyboard::Key::M) {
                    isMuted = !isMuted;
                    if (musicLoaded) music.setVolume(isMuted ? 0.f : 50.f);
                    muteIndicator.setString(isMuted ? "MUTED" : "");
                }

                // TAB — swap controls on start screen only
                if (ke->code == sf::Keyboard::Key::Tab &&
                    gameState == GameState::StartScreen)
                {
                    controlsSwapped = !controlsSwapped;
                    updateControlsDisplay();
                }

                // ENTER / SPACE — start game from start screen
                if (gameState == GameState::StartScreen) {
                    if (ke->code == sf::Keyboard::Key::Enter ||
                        ke->code == sf::Keyboard::Key::Space)
                    {
                        resetGame();
                        gameState = GameState::Playing;
                    }
                }
            }
        }

        // ==========================================================
        //  STATE: START SCREEN
        // ==========================================================
        if (gameState == GameState::StartScreen) {
            float elapsed = pulseClock.getElapsedTime().asSeconds();
            float alpha   = 160.f + 95.f * std::sin(elapsed * 3.f);
            startPrompt.setFillColor(sf::Color(255, 220, 80, static_cast<uint8_t>(alpha)));

            window.clear(sf::Color(8, 8, 24));
            drawBackground();

            if (fontLoaded) {
                window.draw(startTitle);
                window.draw(startDivider);
                window.draw(startSubtitle);
                window.draw(btn1P);   window.draw(lbl1P);
                window.draw(btn2P);   window.draw(lbl2P);
                window.draw(startP1Label);
                window.draw(startP2Label);
                window.draw(startSwapHint);
                window.draw(startPrompt);
                window.draw(startMuteHint);
                window.draw(btnSettings); window.draw(lblSettings);
            }
            window.display();
            continue;
        }

        // ==========================================================
        //  STATE: SETTINGS
        // ==========================================================
        if (gameState == GameState::Settings) {
            window.clear(sf::Color(8, 8, 24));
            drawBackground();

            if (fontLoaded) {
                window.draw(settingsTitle);
                window.draw(fpsLabel);
                window.draw(fpsMinLabel);
                window.draw(fpsMaxLabel);
                window.draw(sliderTrack);
                window.draw(sliderKnob);
                window.draw(btnBack); window.draw(lblBack);
            }
            window.display();
            continue;
        }

        // ==========================================================
        //  STATE: GAME OVER
        // ==========================================================
        if (gameState == GameState::GameOver) {
            float elapsed = pulseClock.getElapsedTime().asSeconds();
            float alpha   = 160.f + 95.f * std::sin(elapsed * 3.f);

            // Animate the subtitle (Try Again / Main Menu prompt)
            gameOverSub.setFillColor(sf::Color(220, 180, 80, static_cast<uint8_t>(alpha)));

            window.clear(sf::Color(8, 8, 24));
            drawBackground();

            if (fontLoaded) {
                window.draw(gameOverTitle);
                window.draw(gameOverSub);
                window.draw(btnTryAgain);  window.draw(lblTryAgain);
                window.draw(btnMainMenu);  window.draw(lblMainMenu);
            }
            window.display();
            continue;
        }

        // ==========================================================
        //  STATE: PLAYING
        // ==========================================================

        // --- Resolve key bindings based on swap state ---
        sf::Keyboard::Key p1Left  = controlsSwapped ? sf::Keyboard::Key::A    : sf::Keyboard::Key::Left;
        sf::Keyboard::Key p1Right = controlsSwapped ? sf::Keyboard::Key::D    : sf::Keyboard::Key::Right;
        sf::Keyboard::Key p2Left  = controlsSwapped ? sf::Keyboard::Key::Left : sf::Keyboard::Key::A;
        sf::Keyboard::Key p2Right = controlsSwapped ? sf::Keyboard::Key::Right: sf::Keyboard::Key::D;

        // --- Player 1 paddle input ---
        if (sf::Keyboard::isKeyPressed(p1Left)  && paddle.getPosition().x > paddleMinX)
            paddle.move({-7.f * SX / 2.4f, 0.f});
        if (sf::Keyboard::isKeyPressed(p1Right) && paddle.getPosition().x < paddleMaxX)
            paddle.move({ 7.f * SX / 2.4f, 0.f});

        // --- Player 2 paddle input  OR  simple CPU AI ---
        if (playerMode == 2) {
            // Human controls
            if (sf::Keyboard::isKeyPressed(p2Left)  && paddle2.getPosition().x > paddleMinX)
                paddle2.move({-7.f * SX / 2.4f, 0.f});
            if (sf::Keyboard::isKeyPressed(p2Right) && paddle2.getPosition().x < paddleMaxX)
                paddle2.move({ 7.f * SX / 2.4f, 0.f});
        } else {
            // CPU: track ball 2 at a capped speed
            float cpuSpeed = 5.f * SX / 2.4f;
            float diff     = ball2.getPosition().x - paddle2.getPosition().x;
            if (diff >  cpuSpeed) paddle2.move({ cpuSpeed, 0.f});
            else if (diff < -cpuSpeed) paddle2.move({-cpuSpeed, 0.f});
            // Clamp CPU paddle to screen
            float cx = paddle2.getPosition().x;
            if (cx < paddleMinX) paddle2.setPosition({paddleMinX, paddle2.getPosition().y});
            if (cx > paddleMaxX) paddle2.setPosition({paddleMaxX, paddle2.getPosition().y});
        }

        // --- Move balls ---
        ball.move(ballVelocity);
        ball2.move(ballVelocity2);

        sf::Vector2f pos  = ball.getPosition();
        sf::Vector2f pos2 = ball2.getPosition();

        // --- Ball 1 wall collisions ---
        if (pos.x < 10.f * SX/2.4f || pos.x > 1910.f * SX/2.4f)
            ballVelocity.x = -ballVelocity.x;
        if (pos.y < 10.f * SY/1.6f)
            ballVelocity.y = -ballVelocity.y;

        // Ball 1 exits bottom → Player 1 loses
        if (pos.y > 960.f) {
            loserName = "Player 1";
            gameOverTitle.setString(loserName + " Lost!");
            gameOverSub.setString("Choose an option below");
            centreH(gameOverTitle, 200.f * SY);
            centreH(gameOverSub,   310.f * SY);
            pulseClock.restart();
            gameState = GameState::GameOver;
            continue;
        }

        // --- Ball 2 wall collisions ---
        if (pos2.x < 10.f * SX/2.4f || pos2.x > 1910.f * SX/2.4f)
            ballVelocity2.x = -ballVelocity2.x;
        if (pos2.y > 950.f * SY/1.6f)
            ballVelocity2.y = -ballVelocity2.y;

        // Ball 2 exits top → Player 2 (or CPU) loses
        if (pos2.y < 0.f) {
            loserName = (playerMode == 1) ? "CPU" : "Player 2";
            gameOverTitle.setString(loserName + " Lost!");
            gameOverSub.setString("Choose an option below");
            centreH(gameOverTitle, 200.f * SY);
            centreH(gameOverSub,   310.f * SY);
            pulseClock.restart();
            gameState = GameState::GameOver;
            continue;
        }

        // --- Paddle collisions ---
        if (ball.getGlobalBounds().findIntersection(paddle.getGlobalBounds()))
            ballVelocity.y = -std::abs(ballVelocity.y);

        if (ball2.getGlobalBounds().findIntersection(paddle2.getGlobalBounds()))
            ballVelocity2.y = std::abs(ballVelocity2.y);

        if (ball.getGlobalBounds().findIntersection(paddle2.getGlobalBounds()))
            ballVelocity.y = std::abs(ballVelocity.y);

        // --- Dividing line collisions ---
        if (ball.getGlobalBounds().findIntersection(dividingLine.getGlobalBounds())) {
            if (ballVelocity.y < 0 && ball.getPosition().y > sp(0,300).y)
                ballVelocity.y = -ballVelocity.y;
            else if (ballVelocity.y > 0 && ball.getPosition().y < sp(0,300).y)
                ballVelocity.y = -ballVelocity.y;
        }
        if (ball2.getGlobalBounds().findIntersection(dividingLine.getGlobalBounds()))
            ballVelocity2.y = -ballVelocity2.y;

        // --- Brick collisions ---
        for (auto& b : bricks) {
            if (!b.destroyed &&
                ball.getGlobalBounds().findIntersection(b.shape.getGlobalBounds()))
            {
                b.destroyed    = true;
                ballVelocity.y = -ballVelocity.y;
                break;
            }
            if (!b.destroyed &&
                ball2.getGlobalBounds().findIntersection(b.shape.getGlobalBounds()))
            {
                b.destroyed     = true;
                ballVelocity2.y = -ballVelocity2.y;
            }
        }

        // --- Render gameplay ---
        window.clear(sf::Color(8, 8, 24));
        drawBackground();

        window.draw(dividingLine);
        window.draw(paddle);
        window.draw(paddle2);
        window.draw(ball);
        window.draw(ball2);

        for (const auto& b : bricks)
            if (!b.destroyed) window.draw(b.shape);

        if (fontLoaded) {
            window.draw(titleText);
            window.draw(swapHintText);
            // In 1-player mode hide P2 control label, show CPU label instead
            if (playerMode == 2) {
                window.draw(p2ControlsText);
                window.draw(swapHintText);
            } else {
                window.draw(cpuLabel);
            }
            window.draw(p1ControlsText);
            window.draw(muteText);
            window.draw(muteIndicator);
        }

        window.display();
    }

    return 0;
}
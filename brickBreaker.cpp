#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <optional>
#include <string>
#include <cmath>

int main() {
    const float W = 1920.f;
    const float H = 1080.f;
    sf::RenderWindow window(sf::VideoMode({(unsigned int)W, (unsigned int)H}), " BRICK BREAKER ");
    window.setFramerateLimit(60);

    // ===================== AUDIO =====================
    sf::Music music;
    bool musicLoaded = false;
    bool musicMuted = false;

    if (music.openFromFile("Snowy.mp3")) {
        musicLoaded = true;
        music.setLooping(true);
        music.setVolume(50.f);
        music.play();
    }

    // ===================== FONTS =====================
    sf::Font font;
    bool fontLoaded = font.openFromFile("ARIAL.ttf");

    // ===================== GAME STATE =====================
    enum class GameState { StartScreen, Playing, GameOver };
    GameState gameState = GameState::StartScreen;
    bool controlsSwapped = false;

    // ===================== BACKGROUND =====================
    struct Star { sf::CircleShape shape; float speed; };
    std::vector<Star> stars;
    srand(42);
    for (int i = 0; i < 250; ++i) {
        Star s;
        float r = (rand() % 3 == 0) ? 3.f : 1.5f;
        s.shape.setRadius(r);
        s.shape.setOrigin({r, r});
        s.shape.setPosition({(float)(rand() % 1920), (float)(rand() % 1080)});
        uint8_t brightness = 150 + rand() % 105;
        s.shape.setFillColor(sf::Color(brightness, brightness, brightness, 180 + rand() % 75));
        s.speed = 0.3f + (rand() % 10) * 0.07f;
        stars.push_back(s);
    }

    sf::RectangleShape bgTop({W, H / 2.f});
    bgTop.setFillColor(sf::Color(5, 5, 30));
    sf::RectangleShape bgBottom({W, H / 2.f});
    bgBottom.setPosition({0.f, H / 2.f});
    bgBottom.setFillColor(sf::Color(10, 0, 40));

    // ===================== PADDLE =====================
    sf::RectangleShape paddle({180.f, 28.f});
    paddle.setFillColor(sf::Color(80, 160, 255));
    paddle.setOutlineThickness(2.f);
    paddle.setOutlineColor(sf::Color(180, 220, 255));
    paddle.setOrigin({90.f, 14.f});
    paddle.setPosition({W / 2.f, H - 100.f});

    // ===================== BALL =====================
    sf::CircleShape ball(14.f);
    ball.setFillColor(sf::Color(255, 240, 100));
    ball.setOutlineThickness(2.f);
    ball.setOutlineColor(sf::Color(255, 200, 50));
    ball.setOrigin({14.f, 14.f});
    ball.setPosition({W / 2.f, H / 2.f});
    sf::Vector2f ballVelocity{7.f, -7.f};

    // ===================== BRICKS =====================
    struct Brick { sf::RectangleShape shape; bool destroyed = false; };
    std::vector<Brick> bricks;

    // 5 distinct row colors
    sf::Color rowColors[6] = {
        sf::Color(255, 80,  80),
        sf::Color(255, 160, 60),
        sf::Color(255, 230, 60),
        sf::Color(80,  220, 100),
        sf::Color(80,  160, 255),
        sf::Color(180, 80,  255)
    };

    auto resetBricks = [&]() {
        bricks.clear();
        // 12 columns x 6 rows, centered horizontally
        // Each brick: 130w x 40h, gap: 10px
        // Total width: 12*140 - 10 = 1670, start at (1920-1670)/2 = 125
        for (int i = 0; i < 12; ++i) {
            for (int j = 0; j < 6; ++j) {
                Brick b;
                b.shape.setSize({130.f, 40.f});
                b.shape.setFillColor(rowColors[j]);
                b.shape.setOutlineThickness(2.f);
                b.shape.setOutlineColor(sf::Color(0, 0, 0, 120));
                // Centered grid
                b.shape.setPosition({125.f + i * 140.f, 120.f + j * 58.f});
                bricks.push_back(b);
            }
        }
    };
    resetBricks();

    // ===================== UI HELPER =====================
    auto makeBtn = [](sf::Vector2f size, sf::Vector2f pos,
                      sf::Color fill = sf::Color(50, 50, 120)) {
        sf::RectangleShape btn(size);
        btn.setFillColor(fill);
        btn.setOutlineThickness(3.f);
        btn.setOutlineColor(sf::Color(150, 150, 255));
        btn.setPosition(pos);
        return btn;
    };

    // ---- In-game HUD buttons ----
    sf::RectangleShape swapBtn = makeBtn({220.f, 50.f}, {60.f,  H - 75.f});
    sf::RectangleShape muteBtn = makeBtn({180.f, 50.f}, {W - 240.f, H - 75.f});

    // ---- Start screen ----
    sf::RectangleShape playBtn = makeBtn({280.f, 80.f},
        {W / 2.f - 140.f, H / 2.f + 20.f}, sf::Color(40, 40, 110));
    playBtn.setOutlineColor(sf::Color(180, 180, 255));
    playBtn.setOutlineThickness(3.f);

    // ---- Game Over screen ----
    sf::RectangleShape retryBtn = makeBtn({280.f, 80.f},
        {W / 2.f - 140.f, H / 2.f + 20.f}, sf::Color(40, 40, 110));
    sf::RectangleShape menuBtn  = makeBtn({280.f, 60.f},
        {W / 2.f - 140.f, H / 2.f + 120.f}, sf::Color(30, 30, 80));

    // Decorative brick strip on start screen
    std::vector<sf::RectangleShape> decoStrip;
    for (int i = 0; i < 13; ++i) {
        sf::RectangleShape r({130.f, 30.f});
        r.setFillColor(rowColors[i % 6]);
        r.setOutlineThickness(2.f);
        r.setOutlineColor(sf::Color(0,0,0,80));
        r.setPosition({125.f + i * 140.f, H - 160.f});
        decoStrip.push_back(r);
    }

    // ===================== ANIMATION =====================
    float pulse = 0.f;

    // ===================== GAME LOOP =====================
    while (window.isOpen()) {
        sf::Vector2f mousePos(sf::Mouse::getPosition(window));

        // ---- Events ----
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();

            if (const auto* mp = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mp->button != sf::Mouse::Button::Left) continue;
                sf::Vector2f click((float)mp->position.x, (float)mp->position.y);

                if (gameState == GameState::StartScreen) {
                    if (playBtn.getGlobalBounds().contains(click)) {
                        gameState = GameState::Playing;
                        paddle.setPosition({W / 2.f, H - 100.f});
                        ball.setPosition({W / 2.f, H / 2.f});
                        ballVelocity = {7.f, -7.f};
                        resetBricks();
                    }
                }
                else if (gameState == GameState::Playing) {
                    if (swapBtn.getGlobalBounds().contains(click))
                        controlsSwapped = !controlsSwapped;
                    if (muteBtn.getGlobalBounds().contains(click)) {
                        musicMuted = !musicMuted;
                        if (musicLoaded)
                            music.setVolume(musicMuted ? 0.f : 50.f);
                    }
                }
                else if (gameState == GameState::GameOver) {
                    if (retryBtn.getGlobalBounds().contains(click)) {
                        gameState = GameState::Playing;
                        paddle.setPosition({W / 2.f, H - 100.f});
                        ball.setPosition({W / 2.f, H / 2.f});
                        ballVelocity = {7.f, -7.f};
                        resetBricks();
                    }
                    if (menuBtn.getGlobalBounds().contains(click)) {
                        gameState = GameState::StartScreen;
                        paddle.setPosition({W / 2.f, H - 100.f});
                        ball.setPosition({W / 2.f, H / 2.f});
                        ballVelocity = {7.f, -7.f};
                        resetBricks();
                    }
                }
            }
        }

        // ---- Game Logic ----
        if (gameState == GameState::Playing) {
            sf::Keyboard::Key kLeft  = controlsSwapped ? sf::Keyboard::Key::A    : sf::Keyboard::Key::Left;
            sf::Keyboard::Key kRight = controlsSwapped ? sf::Keyboard::Key::D    : sf::Keyboard::Key::Right;
            if (sf::Keyboard::isKeyPressed(kLeft)  && paddle.getPosition().x > 100.f)
                paddle.move({-12.f, 0.f});
            if (sf::Keyboard::isKeyPressed(kRight) && paddle.getPosition().x < W - 100.f)
                paddle.move({12.f,  0.f});

            ball.move(ballVelocity);
            sf::Vector2f bp = ball.getPosition();

            // Wall collisions
            if (bp.x - 14.f < 0.f)    { ballVelocity.x =  std::abs(ballVelocity.x); }
            if (bp.x + 14.f > W)       { ballVelocity.x = -std::abs(ballVelocity.x); }
            if (bp.y - 14.f < 0.f)     { ballVelocity.y =  std::abs(ballVelocity.y); }
            if (bp.y > H + 30.f)       { gameState = GameState::GameOver; }

            // Paddle collision
            if (ball.getGlobalBounds().findIntersection(paddle.getGlobalBounds()))
                ballVelocity.y = -std::abs(ballVelocity.y);

            // ---- BRICK COLLISION (fixed) ----
            for (auto& b : bricks) {
                if (b.destroyed) continue;
                auto hit = ball.getGlobalBounds().findIntersection(b.shape.getGlobalBounds());
                if (!hit) continue;

                b.destroyed = true;

                // Determine bounce axis from overlap size
                float ox = hit->size.x;
                float oy = hit->size.y;
                if (ox < oy)
                    ballVelocity.x = -ballVelocity.x;   // hit left/right face
                else
                    ballVelocity.y = -ballVelocity.y;   // hit top/bottom face
                break;
            }
        }

        // ---- Stars ----
        for (auto& s : stars) {
            s.shape.move({0.f, s.speed});
            if (s.shape.getPosition().y > H)
                s.shape.setPosition({(float)(rand() % 1920), 0.f});
        }

        // ---- Pulse ----
        pulse += 0.04f;
        uint8_t p = (uint8_t)(180 + 75 * std::sin(pulse));

        // ---- Hover effects ----
        auto hoverBtn = [&](sf::RectangleShape& btn) {
            btn.setFillColor(btn.getGlobalBounds().contains(mousePos)
                ? sf::Color(90, 90, 190) : sf::Color(50, 50, 120));
        };
        hoverBtn(playBtn);
        hoverBtn(retryBtn);
        hoverBtn(menuBtn);
        hoverBtn(swapBtn);
        hoverBtn(muteBtn);

        // ===================== RENDER =====================
        window.clear(sf::Color(5, 5, 20));
        window.draw(bgTop);
        window.draw(bgBottom);
        for (auto& s : stars) window.draw(s.shape);

        // ---- START SCREEN ----
        if (gameState == GameState::StartScreen) {
            for (auto& d : decoStrip) window.draw(d);

            if (fontLoaded) {
                // Big title
                sf::Text title(font, "??? COSMIC BRICK BREAKER ???", 72);
                title.setFillColor(sf::Color(p, p, 255));
                title.setOutlineColor(sf::Color(60, 60, 180));
                title.setOutlineThickness(4.f);
                {
                    sf::FloatRect tb = title.getLocalBounds();
                    title.setOrigin({tb.size.x / 2.f, tb.size.y / 2.f});
                    title.setPosition({W / 2.f, H / 2.f - 220.f});
                }
                window.draw(title);

                // Tagline
                sf::Text tag(font, "Smash every brick across the cosmos", 28);
                tag.setFillColor(sf::Color(160, 160, 220));
                {
                    sf::FloatRect tb = tag.getLocalBounds();
                    tag.setOrigin({tb.size.x / 2.f, tb.size.y / 2.f});
                    tag.setPosition({W / 2.f, H / 2.f - 130.f});
                }
                window.draw(tag);

                // Controls hint
                sf::Text hint(font,
                    "Player 1: Left / Right Arrow Keys        Player 2: A / D Keys\n"
                    "                  Use  \"Swap Controls\"  to switch assignments",
                    24);
                hint.setFillColor(sf::Color(140, 140, 200));
                {
                    sf::FloatRect tb = hint.getLocalBounds();
                    hint.setOrigin({tb.size.x / 2.f, tb.size.y / 2.f});
                    hint.setPosition({W / 2.f, H / 2.f - 50.f});
                }
                window.draw(hint);

                // PLAY button
                window.draw(playBtn);
                sf::Text playTxt(font, "PLAY", 40);
                playTxt.setFillColor(sf::Color(230, 230, 255));
                {
                    sf::FloatRect tb = playTxt.getLocalBounds();
                    playTxt.setOrigin({tb.size.x / 2.f, tb.size.y / 2.f});
                    playTxt.setPosition({W / 2.f, H / 2.f + 60.f});
                }
                window.draw(playTxt);
            }
        }

        // ---- PLAYING ----
        else if (gameState == GameState::Playing) {
            window.draw(paddle);
            window.draw(ball);
            for (const auto& b : bricks)
                if (!b.destroyed) window.draw(b.shape);

            if (fontLoaded) {
                // HUD title
                sf::Text hudTitle(font, "??? COSMIC BRICK BREAKER ???", 32);
                hudTitle.setFillColor(sf::Color(p, p, 255));
                hudTitle.setOutlineColor(sf::Color(60, 60, 160));
                hudTitle.setOutlineThickness(2.f);
                {
                    sf::FloatRect tb = hudTitle.getLocalBounds();
                    hudTitle.setOrigin({tb.size.x / 2.f, 0.f});
                    hudTitle.setPosition({W / 2.f, 14.f});
                }
                window.draw(hudTitle);

                // Controls display
                std::string p1k = controlsSwapped ? "A / D Keys"          : "Left / Right Arrow";
                std::string p2k = controlsSwapped ? "Left / Right Arrow"  : "A / D Keys";
                sf::Text ctrl(font, "Player 1: " + p1k + "          Player 2: " + p2k, 22);
                ctrl.setFillColor(sf::Color(180, 180, 220));
                {
                    sf::FloatRect tb = ctrl.getLocalBounds();
                    ctrl.setOrigin({tb.size.x / 2.f, 0.f});
                    ctrl.setPosition({W / 2.f, H - 68.f});
                }
                window.draw(ctrl);

                // Swap button
                window.draw(swapBtn);
                sf::Text swapTxt(font, "Swap Controls", 22);
                swapTxt.setFillColor(sf::Color(200, 200, 255));
                swapTxt.setPosition({72.f, H - 64.f});
                window.draw(swapTxt);

                // Mute button
                window.draw(muteBtn);
                sf::Text muteTxt(font, musicMuted ? "Music: OFF" : "Music: ON", 22);
                muteTxt.setFillColor(musicMuted ? sf::Color(255,120,120) : sf::Color(120,255,120));
                muteTxt.setPosition({W - 230.f, H - 64.f});
                window.draw(muteTxt);
            }
        }

        // ---- GAME OVER ----
        else if (gameState == GameState::GameOver) {
            // Frozen board behind overlay
            window.draw(paddle);
            for (const auto& b : bricks)
                if (!b.destroyed) window.draw(b.shape);

            sf::RectangleShape overlay({W, H});
            overlay.setFillColor(sf::Color(0, 0, 0, 170));
            window.draw(overlay);

            if (fontLoaded) {
                sf::Text goTitle(font, "GAME OVER", 90);
                goTitle.setFillColor(sf::Color(255, 70, 70));
                goTitle.setOutlineColor(sf::Color(120, 0, 0));
                goTitle.setOutlineThickness(5.f);
                {
                    sf::FloatRect tb = goTitle.getLocalBounds();
                    goTitle.setOrigin({tb.size.x / 2.f, tb.size.y / 2.f});
                    goTitle.setPosition({W / 2.f, H / 2.f - 160.f});
                }
                window.draw(goTitle);

                sf::Text goSub(font, "The ball escaped the void...", 32);
                goSub.setFillColor(sf::Color(200, 140, 140));
                {
                    sf::FloatRect tb = goSub.getLocalBounds();
                    goSub.setOrigin({tb.size.x / 2.f, tb.size.y / 2.f});
                    goSub.setPosition({W / 2.f, H / 2.f - 60.f});
                }
                window.draw(goSub);

                window.draw(retryBtn);
                sf::Text retryTxt(font, "PLAY AGAIN", 36);
                retryTxt.setFillColor(sf::Color(220, 220, 255));
                {
                    sf::FloatRect tb = retryTxt.getLocalBounds();
                    retryTxt.setOrigin({tb.size.x / 2.f, tb.size.y / 2.f});
                    retryTxt.setPosition({W / 2.f, H / 2.f + 60.f});
                }
                window.draw(retryTxt);

                window.draw(menuBtn);
                sf::Text menuTxt(font, "MAIN MENU", 30);
                menuTxt.setFillColor(sf::Color(180, 180, 240));
                {
                    sf::FloatRect tb = menuTxt.getLocalBounds();
                    menuTxt.setOrigin({tb.size.x / 2.f, tb.size.y / 2.f});
                    menuTxt.setPosition({W / 2.f, H / 2.f + 150.f});
                }
                window.draw(menuTxt);
            }
        }

        window.display();
    }
    return 0;
}
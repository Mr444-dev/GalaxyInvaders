#include <SFML/Graphics.hpp>
#include <vector>
#include <list>
#include <random>
#include <string>
#include <cmath>
#include <ctime>
#include <iostream> // Dla komunikatów DEBUG

// --- Stałe ---
const float SCREEN_WIDTH = 800.0f;
const float SCREEN_HEIGHT = 600.0f;
const float PLAYER_SPEED = 250.0f;
const float BULLET_SPEED = 500.0f;
const float ENEMY_SPEED = 35.0f;
const float ENEMY_DROP_DISTANCE = 1.0f;
const float ENEMY_BULLET_SPEED = 250.0f;
const float ENEMY_SHOOT_INTERVAL = 1.5f;
const float PLAYER_SHOOT_INTERVAL = 0.4f;

bool moveEnemiesDown = false; // Flaga do przesuwania wrogów w dół

// --- Stany Gry ---
enum class GameState { MainMenu, Playing, GameOver, LevelWon }; // Dodano MainMenu

// --- Struktura Cząsteczki ---
struct Particle {
    sf::CircleShape shape;
    sf::Vector2f velocity;
    float lifetime;
};

// --- Funkcja tworzenia eksplozji wroga ---
void createEnemyExplosion(std::vector<Particle>& particleSystem, sf::Vector2f position) {
    static std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<> velDist(-60.0f, 60.0f); // Slightly slower particles
    std::uniform_real_distribution<> lifeDist(0.3f, 0.8f);  // Shorter lifetime
    std::uniform_int_distribution<> colorCompDist(50, 150); // Grayish/Greenish tones

    int numParticles = 25; // Fewer particles than player explosion
    for (int i = 0; i < numParticles; ++i) {
        Particle p;
        p.shape.setRadius(static_cast<float>(rand() % 2 + 1)); // Smaller particles
        // Example: Greenish/Grayish color
        p.shape.setFillColor(sf::Color(colorCompDist(gen) / 2, colorCompDist(gen), colorCompDist(gen) / 2, 200));
        p.shape.setPosition(position);
        p.velocity = sf::Vector2f(velDist(gen), velDist(gen));
        p.lifetime = lifeDist(gen);
        particleSystem.push_back(p);
    }
}

// --- Funkcja tworzenia eksplozji ---
void createPlayerExplosion(std::vector<Particle>& particleSystem, sf::Vector2f position) {
    static std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<> velDist(-90.0f, 90.0f);
    std::uniform_real_distribution<> lifeDist(0.4f, 1.2f);
    std::uniform_int_distribution<> colorCompDist(100, 255);

    int numParticles = 40;
    for (int i = 0; i < numParticles; ++i) {
        Particle p;
        p.shape.setRadius(static_cast<float>(rand() % 3 + 1));
        p.shape.setFillColor(sf::Color(colorCompDist(gen), colorCompDist(gen) / 2, 0, 220));
        p.shape.setPosition(position);
        p.velocity = sf::Vector2f(velDist(gen), velDist(gen));
        p.lifetime = lifeDist(gen);
        particleSystem.push_back(p);
    }
}

// --- Funkcja Resetowania/Inicjalizacji Gry ---
void resetGame(
    GameState& currentState,
    sf::Sprite& playerSprite,
    std::list<sf::Sprite>& enemies,
    std::list<sf::Sprite>& bullets,
    std::list<sf::Sprite>& enemyBullets,
    std::vector<Particle>& particles,
    int& score,
    sf::Text& scoreText,
    float& enemyDirection,
    const sf::Texture& enemyTexture, // Potrzebne do tworzenia wrogów
    float enemyScaleFactor,         // Skala wrogów
    float playerScaleFactor)        // Skala gracza (może niepotrzebna jeśli gracz nie jest resetowany w ten sposób)
{
    std::cout << "DEBUG: Resetting Game...\n";
    currentState = GameState::Playing;
    score = 0;
    scoreText.setString("Score: 0");
    scoreText.setCharacterSize(24);
    scoreText.setFillColor(sf::Color::White);
    bullets.clear();
    enemyBullets.clear();
    enemies.clear();
    particles.clear();

    // Reset gracza (pozycja i widoczność)
    sf::FloatRect playerLocalBounds = playerSprite.getLocalBounds(); // Ponownie pobierz localBounds na wszelki wypadek
    playerSprite.setOrigin(playerLocalBounds.left + playerLocalBounds.width / 2.0f, playerLocalBounds.top + playerLocalBounds.height / 2.0f);
    playerSprite.setPosition(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT - playerSprite.getGlobalBounds().height / 2.0f - 10.0f);
    playerSprite.setColor(sf::Color::White);

    // Stwórz wrogów na nowo
    int enemiesPerRow = 10;
    int numRows = 4;
    sf::Sprite tempEnemySprite(enemyTexture);
    tempEnemySprite.setScale(enemyScaleFactor, enemyScaleFactor);
    sf::FloatRect scaledEnemyBounds = tempEnemySprite.getGlobalBounds();
    float enemySpacingX = scaledEnemyBounds.width * 1.4f;
    float enemySpacingY = scaledEnemyBounds.height * 1.4f;
    float startX = (SCREEN_WIDTH - (enemiesPerRow - 1) * enemySpacingX - scaledEnemyBounds.width) / 2.0f;
    float startY = 60.0f; // Upewnij się, że jest wystarczająco wysoko

    for (int j = 0; j < numRows; ++j) {
        for (int i = 0; i < enemiesPerRow; ++i) {
            sf::Sprite enemySprite(enemyTexture);
            enemySprite.setScale(enemyScaleFactor, enemyScaleFactor);
            enemySprite.setPosition(startX + i * enemySpacingX, startY + j * enemySpacingY);
            enemies.push_back(enemySprite);
        }
    }
    enemyDirection = 1.0f; // Reset kierunku wrogów
}


int main() {
    srand(static_cast<unsigned int>(time(0)));

    // --- Inicjalizacja Okna ---
    sf::RenderWindow window(sf::VideoMode(static_cast<unsigned int>(SCREEN_WIDTH), static_cast<unsigned int>(SCREEN_HEIGHT)), "Galaxy Invaders SFML");
    window.setFramerateLimit(60);

    // --- Ładowanie Zasobów ---
    sf::Texture playerTexture;
    if (!playerTexture.loadFromFile("player.png")) return EXIT_FAILURE;
    sf::Texture enemyTexture;
    if (!enemyTexture.loadFromFile("enemy.jpg")) return EXIT_FAILURE;
    sf::Texture bulletTexture;
    if (!bulletTexture.loadFromFile("bullet.png")) return EXIT_FAILURE;
    sf::Texture enemyBulletTexture;
    if (!enemyBulletTexture.loadFromFile("enemy_bullet.png")) return EXIT_FAILURE;
    sf::Font font;
    // Używaj ścieżki względnej, jeśli plik jest kopiowany przez CMake do katalogu build
    if (!font.loadFromFile("arial.ttf")) {
         // Spróbuj ścieżki z folderem resources, jeśli kopiowanie zawiodło lub uruchamiasz z innego miejsca
        if (!font.loadFromFile("resources/arial.ttf")) return EXIT_FAILURE;
    }


    // --- Ustawienia Skalowania ---
    const float playerScaleFactor = 0.04f;
    const float enemyScaleFactor = 0.05f;
    const float bulletScaleFactor = 0.1f;
    const float enemyBulletScaleFactor = 0.05f;

    // --- Tworzenie Obiektów Gry (początkowe) ---
    GameState currentState = GameState::MainMenu; // Zacznij od Menu Głównego

    sf::Sprite playerSprite; // Gracz jest tworzony raz
    playerSprite.setTexture(playerTexture);
    playerSprite.setScale(playerScaleFactor, playerScaleFactor);
    // Pozycja gracza jest ustawiana w resetGame()

    std::list<sf::Sprite> bullets;
    std::list<sf::Sprite> enemyBullets;
    std::list<sf::Sprite> enemies; // Pusta na początku
    std::vector<Particle> particles;

    int score = 0; // Wynik jest resetowany w resetGame()
    float enemyDirection = 1.0f; // Kierunek jest resetowany w resetGame()

    // Teksty
    sf::Text scoreText("Score: 0", font, 24);
    scoreText.setPosition(10.f, 10.f);
    scoreText.setFillColor(sf::Color::White);

    sf::Text titleText("GALAXY INVADERS", font, 60);
    titleText.setFillColor(sf::Color::Cyan);
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setOrigin(titleBounds.left + titleBounds.width / 2.0f, titleBounds.top + titleBounds.height / 2.0f);
    titleText.setPosition(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f - 100.f);

    sf::Text startText("Press SPACE to Start", font, 30);
    startText.setFillColor(sf::Color::White);
    sf::FloatRect startBounds = startText.getLocalBounds();
    startText.setOrigin(startBounds.left + startBounds.width / 2.0f, startBounds.top + startBounds.height / 2.0f);
    startText.setPosition(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);

    sf::Text gameOverText("GAME OVER", font, 70);
    gameOverText.setFillColor(sf::Color::Red);
    sf::FloatRect goTextBounds = gameOverText.getLocalBounds();
    gameOverText.setOrigin(goTextBounds.left + goTextBounds.width / 2.0f, goTextBounds.top + goTextBounds.height / 2.0f);
    gameOverText.setPosition(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f - 50.f);

    sf::Text levelWonText("LEVEL CLEARED!", font, 70);
    levelWonText.setFillColor(sf::Color::Green);
    sf::FloatRect lwTextBounds = levelWonText.getLocalBounds();
    levelWonText.setOrigin(lwTextBounds.left + lwTextBounds.width / 2.0f, lwTextBounds.top + lwTextBounds.height / 2.0f);
    levelWonText.setPosition(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f - 50.f);

    sf::Text finalScoreText("", font, 30);
    finalScoreText.setFillColor(sf::Color::White);

    sf::Text restartText("Press R to Restart", font, 20);
    restartText.setFillColor(sf::Color::Yellow);
    sf::FloatRect rTextBounds = restartText.getLocalBounds();
    restartText.setOrigin(rTextBounds.left + rTextBounds.width / 2.0f, rTextBounds.top + rTextBounds.height / 2.0f);
    restartText.setPosition(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f + 40.f);


    // --- Zmienne i Zegary Gry ---
    sf::Clock clock;
    sf::Clock playerShootCooldown;
    sf::Clock enemyShootTimer;
    sf::Clock animationClock;
    sf::Clock scoreAnimationTimer;

    bool scoreAnimating = false;
    const float scoreAnimationDuration = 0.2f;

    // --- Główna Pętla Gry ---
    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();

        // --- Obsługa Zdarzeń ---
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            switch (currentState) {
                case GameState::MainMenu:
                    if (event.type == sf::Event::KeyPressed) {
                        if (event.key.code == sf::Keyboard::Space) {
                            // Wywołaj reset gry przy starcie z menu
                            resetGame(currentState, playerSprite, enemies, bullets, enemyBullets, particles, score, scoreText, enemyDirection, enemyTexture, enemyScaleFactor, playerScaleFactor);
                            // Zresetuj zegary animacji itp.
                            animationClock.restart();
                            scoreAnimationTimer.restart();
                            playerShootCooldown.restart();
                            enemyShootTimer.restart();
                        } else if (event.key.code == sf::Keyboard::Escape) {
                             window.close(); // Wyjście z gry z menu
                        }
                    }
                    break;

                case GameState::Playing:
                    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space) {
                        if (playerShootCooldown.getElapsedTime().asSeconds() >= PLAYER_SHOOT_INTERVAL) {
                            sf::Sprite bulletSprite(bulletTexture);
                            bulletSprite.setScale(bulletScaleFactor, bulletScaleFactor);
                            sf::FloatRect scaledBulletBounds = bulletSprite.getGlobalBounds();
                            sf::Vector2f playerPos = playerSprite.getPosition();
                            sf::FloatRect currentPlBounds = playerSprite.getGlobalBounds();

                            bulletSprite.setPosition(
                                playerPos.x - scaledBulletBounds.width / 2.0f,
                                playerPos.y - currentPlBounds.height / 2.0f - scaledBulletBounds.height
                            );
                            bullets.push_back(bulletSprite);
                            playerShootCooldown.restart();
                        }
                    }
                    break;

                case GameState::GameOver:
                case GameState::LevelWon:
                    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R) {
                        // Wywołaj reset gry przy restarcie
                        resetGame(currentState, playerSprite, enemies, bullets, enemyBullets, particles, score, scoreText, enemyDirection, enemyTexture, enemyScaleFactor, playerScaleFactor);
                        // Zresetuj zegary animacji itp.
                        animationClock.restart();
                        scoreAnimationTimer.restart();
                        playerShootCooldown.restart();
                        enemyShootTimer.restart();
                    }
                    break;
            }
        } // Koniec pętli zdarzeń


        // --- Logika Gry (Tylko w stanie Playing) ---
        if (currentState == GameState::Playing) {
             // std::cout << "DEBUG: Frame Start - State: Playing\n"; // Odkomentuj do diagnozy

            // Ruch Gracza
            float playerMoveX = 0.0f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::A)) playerMoveX -= PLAYER_SPEED * deltaTime;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::D)) playerMoveX += PLAYER_SPEED * deltaTime;
            playerSprite.move(playerMoveX, 0.f);

            // Ograniczenie ruchu gracza
            sf::FloatRect currentPlBounds = playerSprite.getGlobalBounds();
            if (currentPlBounds.left < 0.f) playerSprite.setPosition(currentPlBounds.width / 2.0f, playerSprite.getPosition().y);
            if (currentPlBounds.left + currentPlBounds.width > SCREEN_WIDTH) playerSprite.setPosition(SCREEN_WIDTH - currentPlBounds.width / 2.0f, playerSprite.getPosition().y);

            // Ruch Pocisków Gracza
            for (auto it = bullets.begin(); it != bullets.end();) {
                it->move(0.f, -BULLET_SPEED * deltaTime);
                if (it->getPosition().y + it->getGlobalBounds().height < 0) it = bullets.erase(it);
                else ++it;
            }

            // Ruch Wrogów i Sprawdzanie Krawędzi/Dna
            moveEnemiesDown = false;
            // std::cout << "DEBUG: Checking Enemy Edge/Bottom...\n"; // Odkomentuj do diagnozy
            for (const auto& enemy : enemies) {
                sf::FloatRect enemyBounds = enemy.getGlobalBounds();
                // Sprawdzenie krawędzi
                if ((enemyDirection > 0 && enemyBounds.left + enemyBounds.width >= SCREEN_WIDTH - 5.f) ||
                    (enemyDirection < 0 && enemyBounds.left <= 5.f)) {
                    enemyDirection *= -1.0f;
                    moveEnemiesDown = true;
                    break; // Wystarczy jeden wróg na krawędzi
                }
                // Sprawdzenie czy wróg dotarł do dna (Game Over)
                // Spróbuj tego warunku, jeśli poprzedni był zbyt czuły:
                if (enemyBounds.top + enemyBounds.height >= SCREEN_HEIGHT - 50.f) { // Sprawdza czy wróg jest blisko samego dołu
                // Poprzedni warunek: if (enemyBounds.top + enemyBounds.height >= SCREEN_HEIGHT - playerSprite.getGlobalBounds().height - 20.f) {
                    // std::cout << "!!!! DEBUG: ENEMY REACHED BOTTOM DETECTED !!!! Y=" << enemyBounds.top + enemyBounds.height << "\n"; // Odkomentuj
                    currentState = GameState::GameOver;
                    createPlayerExplosion(particles, playerSprite.getPosition());
                    playerSprite.setColor(sf::Color::Transparent);
                    goto end_playing_logic; // Użyj goto, aby pominąć resztę logiki
                }
            }
            // Przesuń wszystkich wrogów
            for (auto& enemy : enemies) enemy.move(ENEMY_SPEED * enemyDirection * deltaTime, (moveEnemiesDown ? ENEMY_DROP_DISTANCE : 0.f));

            // Strzelanie Wrogów
            if (enemyShootTimer.getElapsedTime().asSeconds() >= ENEMY_SHOOT_INTERVAL && !enemies.empty()) {
                int randomIndex = rand() % enemies.size();
                auto it = std::next(enemies.begin(), randomIndex);
                if (it != enemies.end()) {
                    sf::Sprite enemyBulletSprite(enemyBulletTexture);
                    enemyBulletSprite.setScale(enemyBulletScaleFactor, enemyBulletScaleFactor);
                    sf::FloatRect scaledBulletBounds = enemyBulletSprite.getGlobalBounds();
                    sf::FloatRect enemyBounds = it->getGlobalBounds();
                    enemyBulletSprite.setPosition(
                        enemyBounds.left + enemyBounds.width / 2.0f - scaledBulletBounds.width / 2.0f,
                        enemyBounds.top + enemyBounds.height);
                    enemyBullets.push_back(enemyBulletSprite);
                    enemyShootTimer.restart();
                }
            }

            // Ruch Pocisków Wrogów i Kolizja z Graczem
            for (auto it = enemyBullets.begin(); it != enemyBullets.end();) {
                it->move(0.f, ENEMY_BULLET_SPEED * deltaTime);
                bool erased = false;
                if (it->getPosition().y > SCREEN_HEIGHT) {
                    it = enemyBullets.erase(it);
                    erased = true;
                } else if (it->getGlobalBounds().intersects(playerSprite.getGlobalBounds())) {
                    // std::cout << "!!!! DEBUG: PLAYER HIT BY ENEMY BULLET DETECTED !!!!\n"; // Odkomentuj
                    currentState = GameState::GameOver;
                    createPlayerExplosion(particles, playerSprite.getPosition());
                    playerSprite.setColor(sf::Color::Transparent);
                    it = enemyBullets.erase(it);
                    erased = true;
                    goto end_playing_logic; // Pomiń resztę logiki
                }
                if (!erased) ++it;
            }

            // Kolizje Pocisków Gracza z Wrogami
            for (auto bulletIt = bullets.begin(); bulletIt != bullets.end();) {
                bool bulletRemoved = false;
                // std::cout << "DEBUG: Checking PlayerBullet-Enemy Collision...\n"; // Odkomentuj
                for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
                    if (bulletIt->getGlobalBounds().intersects(enemyIt->getGlobalBounds())) {
                        // std::cout << "DEBUG: PenemyItlayerBullet-Enemy Collision DETECTED!\n"; // Odkomentuj
                        // --- ADD EXPLOSION CALL HERE ---
                        sf::FloatRect enemyBounds = enemyIt->getGlobalBounds();
                        createEnemyExplosion(particles, sf::Vector2f(enemyBounds.left + enemyBounds.width / 2.f, enemyBounds.top + enemyBounds.height / 2.f));
                        // --- END OF ADDED CODE ---
                        enemyIt = enemies.erase(enemyIt);
                        bulletIt = bullets.erase(bulletIt);
                        bulletRemoved = true;
                        score += 10;
                        scoreText.setString("Score: " + std::to_string(score));
                        scoreAnimating = true;
                        scoreAnimationTimer.restart();
                        scoreText.setCharacterSize(30);
                        scoreText.setFillColor(sf::Color::Yellow);
                        goto next_bullet; // Przejdź do następnej iteracji pętli pocisków
                    } else {
                        ++enemyIt;
                    }
                }
                next_bullet:; // Etykieta dla goto
                if (!bulletRemoved) ++bulletIt; // Tylko jeśli pocisk nie został usunięty
                else if (bulletIt == bullets.end()) break; // Jeśli usunięto ostatni pocisk
            }


            // Kolizje Gracza z Wrogami
            // std::cout << "DEBUG: Checking Player-Enemy Collision...\n"; // Odkomentuj
            for (const auto& enemy : enemies) {
                if (playerSprite.getGlobalBounds().intersects(enemy.getGlobalBounds())) {
                    // std::cout << "!!!! DEBUG: PLAYER-ENEMY COLLISION DETECTED !!!!\n"; // Odkomentuj
                    currentState = GameState::GameOver;
                    createPlayerExplosion(particles, playerSprite.getPosition());
                    playerSprite.setColor(sf::Color::Transparent);
                    goto end_playing_logic; // Pomiń resztę logiki
                }
            }

            // Sprawdzenie warunku wygranej
            if (enemies.empty()) {
                currentState = GameState::LevelWon;
                // Można dodać efekt dźwiękowy lub wizualny wygranej
            }

            // Koniec animacji wyniku
            if (scoreAnimating && scoreAnimationTimer.getElapsedTime().asSeconds() >= scoreAnimationDuration) {
                scoreAnimating = false;
                scoreText.setCharacterSize(24);
                scoreText.setFillColor(sf::Color::White);
            }

        } // Koniec if (currentState == GameState::Playing)
        end_playing_logic:; // Etykieta dla goto

        // --- Aktualizacja Cząsteczek (Zawsze) ---
        for (auto it = particles.begin(); it != particles.end();) {
            it->lifetime -= deltaTime;
            if (it->lifetime <= 0) it = particles.erase(it);
            else {
                it->shape.move(it->velocity * deltaTime);
                float alphaRatio = std::max(0.f, it->lifetime / 1.2f);
                sf::Color color = it->shape.getFillColor();
                color.a = static_cast<sf::Uint8>(200 * alphaRatio);
                it->shape.setFillColor(color);
                ++it;
            }
        }

        // --- Rysowanie ---
        window.clear(sf::Color(10, 0, 20)); // Ciemniejsze tło

        // Rysowanie zależne od stanu
        switch (currentState) {
             case GameState::MainMenu:
                { // Pulsowanie tekstu startowego
                    float time = animationClock.getElapsedTime().asSeconds();
                    float scaleFactor = 1.0f + 0.05f * sin(time * 4.0f);
                    startText.setScale(scaleFactor, scaleFactor);
                    window.draw(titleText);
                    window.draw(startText);
                    startText.setScale(1.0f, 1.0f); // Reset skali
                }
                break;

            case GameState::Playing:
                if(playerSprite.getColor() != sf::Color::Transparent) // Rysuj gracza tylko jeśli jest widoczny
                    window.draw(playerSprite);
                for (const auto& enemy : enemies) window.draw(enemy);
                for (const auto& bullet : bullets) window.draw(bullet);
                for (const auto& bullet : enemyBullets) window.draw(bullet);
                window.draw(scoreText);
                break;

            case GameState::GameOver:
            case GameState::LevelWon: // Wspólne rysowanie dla obu końcowych stanów
                {
                    sf::Text* mainText = (currentState == GameState::GameOver) ? &gameOverText : &levelWonText;
                    float time = animationClock.getElapsedTime().asSeconds();
                    float scaleFactor = 1.0f + 0.05f * sin(time * 5.0f);
                    mainText->setScale(scaleFactor, scaleFactor);
                    window.draw(*mainText);
                    mainText->setScale(1.0f, 1.0f);

                    finalScoreText.setString("Final Score: " + std::to_string(score));
                    sf::FloatRect fsBounds = finalScoreText.getLocalBounds();
                    finalScoreText.setOrigin(fsBounds.left + fsBounds.width / 2.0f, fsBounds.top + fsBounds.height / 2.0f);
                    finalScoreText.setPosition(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);
                    window.draw(finalScoreText);
                    window.draw(restartText);
                }
                break;
        }

        // Rysuj cząsteczki na wierzchu (zawsze)
        for (const auto& p : particles) {
            window.draw(p.shape);
        }

        window.display();
    } // Koniec głównej pętli

    return 0;
}
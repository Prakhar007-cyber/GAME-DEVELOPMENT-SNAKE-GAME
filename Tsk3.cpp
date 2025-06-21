#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <deque>
#include <random>
#include <iostream>
#include <sstream>

using namespace sf;
using namespace std;

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int GRID_SIZE = 20;
const int GRID_WIDTH = WINDOW_WIDTH / GRID_SIZE;
const int GRID_HEIGHT = WINDOW_HEIGHT / GRID_SIZE;

enum class Direction { UP, DOWN, LEFT, RIGHT };

class SnakeGame {
private:
    RenderWindow window;
    Clock gameClock;
    Time timePerFrame;
    float snakeSpeed;
    int score;
    int level;
    bool gameOver;
    bool paused;

    // Snake data
    deque<Vector2i> snake;
    Direction currentDir;
    Direction nextDir;

    // Food
    Vector2i food;
    bool foodSpawned;

    // Graphics
    Texture snakeTexture;
    Texture foodTexture;
    Texture backgroundTexture;
    Sprite snakeSprite;
    Sprite foodSprite;
    Sprite backgroundSprite;
    Font font;
    Text scoreText;
    Text levelText;
    Text gameOverText;

    // Sound
    SoundBuffer eatBuffer;
    SoundBuffer crashBuffer;
    Sound eatSound;
    Sound crashSound;
    Music backgroundMusic;

    // Random number generation
    random_device rd;
    mt19937 gen;
    uniform_int_distribution<> xDist;
    uniform_int_distribution<> yDist;

public:
    SnakeGame() : 
        window(VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "CODTECH Snake Game", Style::Close),
        timePerFrame(seconds(0.1f)),
        snakeSpeed(0.15f),
        score(0),
        level(1),
        gameOver(false),
        paused(false),
        currentDir(Direction::RIGHT),
        nextDir(Direction::RIGHT),
        foodSpawned(false),
        gen(rd()),
        xDist(0, GRID_WIDTH - 1),
        yDist(0, GRID_HEIGHT - 1)
    {
        // Initialize snake
        snake.push_back(Vector2i(5, GRID_HEIGHT / 2));
        snake.push_back(Vector2i(4, GRID_HEIGHT / 2));
        snake.push_back(Vector2i(3, GRID_HEIGHT / 2));

        // Load textures
        if (!snakeTexture.loadFromFile("snake.png")) {
            snakeTexture.create(GRID_SIZE, GRID_SIZE);
            snakeTexture.setRepeated(true);
        }
        if (!foodTexture.loadFromFile("food.png")) {
            foodTexture.create(GRID_SIZE, GRID_SIZE);
            foodTexture.setRepeated(true);
        }
        if (!backgroundTexture.loadFromFile("background.jpg")) {
            backgroundTexture.create(WINDOW_WIDTH, WINDOW_HEIGHT);
        }

        snakeSprite.setTexture(snakeTexture);
        foodSprite.setTexture(foodTexture);
        backgroundSprite.setTexture(backgroundTexture);

        // Load font
        if (!font.loadFromFile("arial.ttf")) {
            // Fallback if font not found
            font.loadFromFile("C:/Windows/Fonts/Arial.ttf");
        }

        // Setup text
        scoreText.setFont(font);
        scoreText.setCharacterSize(24);
        scoreText.setFillColor(Color::Green);
        scoreText.setPosition(10, 10);

        levelText.setFont(font);
        levelText.setCharacterSize(24);
        levelText.setFillColor(Color::Yellow);
        levelText.setPosition(10, 40);

        gameOverText.setFont(font);
        gameOverText.setCharacterSize(48);
        gameOverText.setFillColor(Color::Red);
        gameOverText.setString("GAME OVER\nPress R to restart");
        gameOverText.setPosition(WINDOW_WIDTH/2 - 150, WINDOW_HEIGHT/2 - 50);

        // Load sounds
        if (!eatBuffer.loadFromFile("eat.wav")) {
            // Generate a simple beep if sound file not found
            eatBuffer.loadFromSamples(nullptr, 0, 1, 44100);
        }
        if (!crashBuffer.loadFromFile("crash.wav")) {
            crashBuffer.loadFromSamples(nullptr, 0, 1, 44100);
        }

        eatSound.setBuffer(eatBuffer);
        crashSound.setBuffer(crashBuffer);

        // Load background music
        if (!backgroundMusic.openFromFile("background.ogg")) {
            // Continue without music if file not found
        } else {
            backgroundMusic.setLoop(true);
            backgroundMusic.play();
        }
    }

    void run() {
        while (window.isOpen()) {
            processEvents();
            if (!gameOver && !paused) {
                update();
            }
            render();
        }
    }

private:
    void processEvents() {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) {
                window.close();
            }

            if (event.type == Event::KeyPressed) {
                if (gameOver && event.key.code == Keyboard::R) {
                    resetGame();
                }

                if (event.key.code == Keyboard::P) {
                    paused = !paused;
                }

                if (!paused && !gameOver) {
                    switch (event.key.code) {
                        case Keyboard::Up:
                            if (currentDir != Direction::DOWN) nextDir = Direction::UP;
                            break;
                        case Keyboard::Down:
                            if (currentDir != Direction::UP) nextDir = Direction::DOWN;
                            break;
                        case Keyboard::Left:
                            if (currentDir != Direction::RIGHT) nextDir = Direction::LEFT;
                            break;
                        case Keyboard::Right:
                            if (currentDir != Direction::LEFT) nextDir = Direction::RIGHT;
                            break;
                    }
                }
            }
        }
    }

    void update() {
        static Time timeSinceLastUpdate = Time::Zero;
        timeSinceLastUpdate += gameClock.restart();

        if (timeSinceLastUpdate > seconds(snakeSpeed)) {
            timeSinceLastUpdate = Time::Zero;

            // Update direction
            currentDir = nextDir;

            // Move snake
            Vector2i newHead = snake.front();

            switch (currentDir) {
                case Direction::UP:    newHead.y--; break;
                case Direction::DOWN:  newHead.y++; break;
                case Direction::LEFT:  newHead.x--; break;
                case Direction::RIGHT: newHead.x++; break;
            }

            // Check wall collision
            if (newHead.x < 0 || newHead.x >= GRID_WIDTH || 
                newHead.y < 0 || newHead.y >= GRID_HEIGHT) {
                gameOver = true;
                crashSound.play();
                return;
            }

            // Check self collision
            for (auto segment : snake) {
                if (segment == newHead) {
                    gameOver = true;
                    crashSound.play();
                    return;
                }
            }

            snake.push_front(newHead);

            // Check food collision
            if (newHead == food) {
                eatSound.play();
                score += 10 * level;
                foodSpawned = false;
                
                // Level up every 50 points
                if (score / 50 > level - 1) {
                    level++;
                    snakeSpeed = max(0.05f, snakeSpeed - 0.02f); // Increase speed
                }
            } else {
                snake.pop_back();
            }

            // Spawn new food if needed
            if (!foodSpawned) {
                spawnFood();
            }
        }
    }

    void spawnFood() {
        bool validPosition = false;
        int attempts = 0;
        const int maxAttempts = 100;

        while (!validPosition && attempts < maxAttempts) {
            food.x = xDist(gen);
            food.y = yDist(gen);
            validPosition = true;

            // Check if food spawns on snake
            for (auto segment : snake) {
                if (segment == food) {
                    validPosition = false;
                    break;
                }
            }

            attempts++;
        }

        if (validPosition) {
            foodSpawned = true;
        }
    }

    void render() {
        window.clear();

        // Draw background
        window.draw(backgroundSprite);

        // Draw food
        if (foodSpawned) {
            foodSprite.setPosition(food.x * GRID_SIZE, food.y * GRID_SIZE);
            window.draw(foodSprite);
        }

        // Draw snake
        for (size_t i = 0; i < snake.size(); i++) {
            snakeSprite.setPosition(snake[i].x * GRID_SIZE, snake[i].y * GRID_SIZE);
            
            // Make head slightly different color
            if (i == 0) {
                snakeSprite.setColor(Color(255, 100, 100)); // Reddish head
            } else {
                snakeSprite.setColor(Color(100, 255, 100)); // Green body
            }
            
            window.draw(snakeSprite);
        }

        // Draw score and level
        stringstream ss;
        ss << "Score: " << score;
        scoreText.setString(ss.str());
        window.draw(scoreText);

        ss.str("");
        ss << "Level: " << level;
        levelText.setString(ss.str());
        window.draw(levelText);

        // Draw game over message if needed
        if (gameOver) {
            window.draw(gameOverText);
        }

        // Draw pause message if needed
        if (paused && !gameOver) {
            Text pauseText("PAUSED\nPress P to continue", font, 48);
            pauseText.setFillColor(Color::White);
            pauseText.setPosition(WINDOW_WIDTH/2 - 100, WINDOW_HEIGHT/2 - 50);
            window.draw(pauseText);
        }

        window.display();
    }

    void resetGame() {
        snake.clear();
        snake.push_back(Vector2i(5, GRID_HEIGHT / 2));
        snake.push_back(Vector2i(4, GRID_HEIGHT / 2));
        snake.push_back(Vector2i(3, GRID_HEIGHT / 2));
        
        currentDir = Direction::RIGHT;
        nextDir = Direction::RIGHT;
        
        score = 0;
        level = 1;
        snakeSpeed = 0.15f;
        
        gameOver = false;
        paused = false;
        foodSpawned = false;
        
        spawnFood();
    }
};

int main() {
    SnakeGame game;
    game.run();
    return 0;
}

#include <iostream>
#include <vector>
#include <conio.h>    // _kbhit(), _getch()
#include <windows.h>  // Sleep(), SetConsoleCursorPosition()
#include <ctime>      // time()

using namespace std;

// --- Utility Structure ---
struct Point {
    int x, y;
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

// --- Helper for Flicker-Free Rendering ---
void GoToXY(int x, int y) {
    COORD c;
    c.X = x;
    c.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

// --- Food Class ---
class Food {
private:
    Point position;
    char symbol;

public:
    Food() : symbol('\242') { // Cent symbol or 'O'
        position = { 0, 0 };
    }

    void Respawn(int width, int height, const vector<Point>& snakeBody) {
        bool onSnake;
        do {
            onSnake = false;
            // Generate random position within walls (padding of 1)
            position.x = (rand() % (width - 2)) + 1;
            position.y = (rand() % (height - 2)) + 1;

            // Ensure food doesn't spawn on snake
            for (const auto& part : snakeBody) {
                if (part == position) {
                    onSnake = true;
                    break;
                }
            }
        } while (onSnake);
    }

    Point GetPosition() const { return position; }
    char GetSymbol() const { return symbol; }
};

// --- Snake Class ---
class Snake {
private:
    vector<Point> body;
    Point direction;
    char headChar;
    char bodyChar;

public:
    Snake(int startX, int startY) {
        headChar = '\351'; // Theta or 'O'
        bodyChar = 'o';
        Reset(startX, startY);
    }

    void Reset(int x, int y) {
        body.clear();
        // Start with a small body
        body.push_back({ x, y });
        body.push_back({ x - 1, y });
        body.push_back({ x - 2, y });
        direction = { 1, 0 }; // Moving right initially
    }

    void Move() {
        // Move body: shift each part to the position of the one before it
        for (size_t i = body.size() - 1; i > 0; --i) {
            body[i] = body[i - 1];
        }
        // Move head
        body[0].x += direction.x;
        body[0].y += direction.y;
    }

    void Grow() {
        // Add a new segment at the end (position matches current tail)
        body.push_back(body.back());
    }

    void SetDirection(int dx, int dy) {
        // Prevent 180-degree turns (cannot go directly back)
        if (body.size() > 1) {
            if (direction.x == -dx && direction.y == -dy) return;
        }
        direction = { dx, dy };
    }

    bool CheckCollision(int width, int height) {
        Point head = body[0];

        // Wall Collision
        if (head.x <= 0 || head.x >= width - 1 || head.y <= 0 || head.y >= height - 1) {
            return true;
        }

        // Self Collision (check head against all other body parts)
        for (size_t i = 1; i < body.size(); ++i) {
            if (head == body[i]) return true;
        }

        return false;
    }

    Point GetHead() const { return body[0]; }
    const vector<Point>& GetBody() const { return body; }
    char GetHeadChar() const { return headChar; }
    char GetBodyChar() const { return bodyChar; }
};

// --- Game Class ---
class Game {
private:
    int width, height;
    int score;
    bool gameOver;
    Snake snake;
    Food food;
    int speed; // Sleep duration in ms

public:
    Game(int w, int h) 
        : width(w), height(h), snake(w / 2, h / 2), score(0), gameOver(false), speed(100) {
        srand(static_cast<unsigned>(time(0)));
        HideCursor();
    }

    void HideCursor() {
        CONSOLE_CURSOR_INFO cursorInfo;
        cursorInfo.dwSize = 1;
        cursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    }

    void Setup() {
        gameOver = false;
        score = 0;
        snake.Reset(width / 2, height / 2);
        food.Respawn(width, height, snake.GetBody());
    }

    void ProcessInput() {
        if (_kbhit()) {
            switch (_getch()) {
            case 'a': case 'A': case 75: // Left Arrow
                snake.SetDirection(-1, 0); break;
            case 'd': case 'D': case 77: // Right Arrow
                snake.SetDirection(1, 0); break;
            case 'w': case 'W': case 72: // Up Arrow
                snake.SetDirection(0, -1); break;
            case 's': case 'S': case 80: // Down Arrow
                snake.SetDirection(0, 1); break;
            case 'x': case 'X': 
                gameOver = true; break;
            }
        }
    }

    void Update() {
        snake.Move();

        // Check if food eaten
        if (snake.GetHead() == food.GetPosition()) {
            score += 10;
            snake.Grow();
            food.Respawn(width, height, snake.GetBody());
            // Optional: Increase speed slightly as game progresses
            // if (speed > 50) speed -= 2;
        }

        if (snake.CheckCollision(width, height)) {
            gameOver = true;
        }
    }

    void Draw() {
        GoToXY(0, 0);

        // Draw Top Border
        for (int i = 0; i < width; i++) cout << "\262";
        cout << endl;

        // Draw Rows
        for (int i = 1; i < height - 1; i++) {
            cout << "\262"; // Left Wall
            for (int j = 1; j < width - 1; j++) {
                Point current = { j, i };
                bool isPrinted = false;

                // Draw Snake Head
                if (snake.GetHead() == current) {
                    cout << snake.GetHeadChar();
                    isPrinted = true;
                }
                // Draw Food
                else if (food.GetPosition() == current) {
                    cout << food.GetSymbol();
                    isPrinted = true;
                }
                // Draw Snake Body
                else {
                    const auto& body = snake.GetBody();
                    for (size_t k = 1; k < body.size(); ++k) {
                        if (body[k] == current) {
                            cout << snake.GetBodyChar();
                            isPrinted = true;
                            break;
                        }
                    }
                }

                if (!isPrinted) cout << " ";
            }
            cout << "\262"; // Right Wall
            cout << endl;
        }

        // Draw Bottom Border
        for (int i = 0; i < width; i++) cout << "\262";
        cout << endl;

        // UI Info
        cout << "Score: " << score << "   \n";
        cout << "Controls: WASD or Arrows | X to Quit";
    }

    void Run() {
        Setup();
        while (!gameOver) {
            Draw();  
            ProcessInput();
            Update();
            Sleep(speed);
        }
        
        GoToXY(width / 2 - 5, height / 2);
        cout << "GAME OVER!";
        GoToXY(0, height + 2);
        system("pause");
    }
};

int main() {
    // Initialize game with width 50, height 20
    Game game(50, 20);
    game.Run();
    return 0;
}
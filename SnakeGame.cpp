#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <thread> // For sleep
#include <chrono> // For sleep duration

// Linux-specific headers for input handling
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

using namespace std;

// --- Linux Input Utils ---
// Implementation of _kbhit() for Linux
int _kbhit(void) {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

// Implementation of _getch() for Linux
char _getch() {
    char buf = 0;
    struct termios old = {0};
    if (tcgetattr(0, &old) < 0)
        perror("tcsetattr()");
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &old) < 0)
        perror("tcsetattr ICANON");
    if (read(0, &buf, 1) < 0)
        perror("read()");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if (tcsetattr(0, TCSADRAIN, &old) < 0)
        perror("tcsetattr ~ICANON");
    return (buf);
}

// --- Utility Structure ---
struct Point {
    int x, y;
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

// --- Helper for Flicker-Free Rendering ---
// Uses ANSI escape codes to move cursor
void GoToXY(int x, int y) {
    // ANSI uses 1-based indexing (Row;Col)
    cout << "\033[" << y + 1 << ";" << x + 1 << "H";
}

// --- Food Class ---
class Food {
private:
    Point position;
    char symbol;

public:
    Food() : symbol('*') { // Changed to standard ASCII '*'
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
        headChar = 'O'; // Changed to standard ASCII 'O'
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

    ~Game() {
        ShowCursor(); // Restore cursor on exit
    }

    void HideCursor() {
        cout << "\033[?25l"; // ANSI hide cursor
    }
    
    void ShowCursor() {
        cout << "\033[?25h"; // ANSI show cursor
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
            case 'a': case 'A': 
                snake.SetDirection(-1, 0); break;
            case 'd': case 'D': 
                snake.SetDirection(1, 0); break;
            case 'w': case 'W': 
                snake.SetDirection(0, -1); break;
            case 's': case 'S': 
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
        }

        if (snake.CheckCollision(width, height)) {
            gameOver = true;
        }
    }

    void Draw() {
        // Move cursor top-left rather than clearing screen (reduces flicker)
        GoToXY(0, 0);

        // Draw Top Border
        for (int i = 0; i < width; i++) cout << "#";
        cout << endl;

        // Draw Rows
        for (int i = 1; i < height - 1; i++) {
            cout << "#"; // Left Wall
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
            cout << "#"; // Right Wall
            cout << endl;
        }

        // Draw Bottom Border
        for (int i = 0; i < width; i++) cout << "#";
        cout << endl;

        // UI Info
        cout << "Score: " << score << "   \n";
        cout << "Controls: WASD | X to Quit  ";
    }

    void Run() {
        // Clear screen once at start
        cout << "\033[2J"; 
        
        Setup();
        while (!gameOver) {
            Draw();  
            ProcessInput();
            Update();
            // Portable C++ sleep
            std::this_thread::sleep_for(std::chrono::milliseconds(speed));
        }
        
        GoToXY(width / 2 - 5, height / 2);
        cout << "GAME OVER!";
        GoToXY(0, height + 2);
        
        ShowCursor();
        cout << "Press any key to exit...";
        _getch();
    }
};

int main() {
    // Initialize game with width 50, height 20
    Game game(50, 20);
    game.Run();
    return 0;
}
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <atomic>
#include <mutex>
#include <random>

/**********************************************************************************|
|*                                                                                *|
|*                              stoopid  Ping Pong                                *|
|*                                                                                *|
|**********************************************************************************/

enum Directions {left = 0, right = 1, up = 2, down = 3, upl = 4, upr = 5, downl = 6, downr = 7, no = -1};
const std::string ball_symbol = "○";

// Global object for synchronizing
std::mutex mtx;
std::atomic<bool> game_running(true);

const bool DEBUG_MODE = false;

int score = 0;

struct Board{
  int width = 75;
  int height = 25;
};
const Board board;

class Ball{
  private:
    int Xcord = board.width/2;
    int Ycord = board.height/2;
    int speed = 2;

    inline void upleft() {this->Ycord++; this->Xcord--;}
    inline void upright() {this->Ycord++; this->Xcord++;}
    inline void downleft() {this->Ycord--; this->Xcord--;}
    inline void downright() {this->Ycord--; this->Xcord++;}
  public:
    int direction = upl;

  inline int get_x() {return Xcord;}
  inline int get_y() {return Ycord;}
  void update() {
    switch(direction) {
      case left:
        if(Xcord >= 0){
          this->Xcord = Xcord-speed;
        } else {
          this->direction = right;
          this->Xcord++;
        }
        break;
      case right:
        if(Xcord <= board.width) {
          this->Xcord = Xcord+speed;
        } else {
          this->direction = left;
          this->Xcord--;
        }
        break;

      case upl:
        if(Ycord < board.height && Xcord-1 > 0) {
          upleft();
        } else if (Ycord == board.height) {
          this->direction = downl;
          downleft();
        } else if (Ycord == board.height && Xcord == 1) {
          this->direction = downr;
          downright();
        }else {
          this->direction = upr;
          upright();
        }
        break;
      case upr:
        if(Ycord < board.height && Xcord+1 < board.width) {
          upright();
        } else if(Ycord == board.height && Xcord+1 == board.width) {
          this->direction = downl;
          downleft();
        }else if (Xcord+1 == board.width){
          this->direction = upl;
          upleft();
        } else if (Ycord == board.height){
          this->direction = downr;
          downright();
        } 
        break;
      case downl:
        if(Ycord > 1 && Xcord-1 > 0) {
          downleft();
        }  else if (Xcord-1 == 0){
          this->direction = downr;
          downright();
        } else if (Ycord == 1){
          this->direction = upl;
          upleft();
        } else {
          this->direction = upr;
          upright();
        }
        break;
      case downr:
        if(Ycord > 1 && Xcord+1 < board.width) {
          downright();
          break;
        } else if (Ycord == 1 && Xcord+1 < board.width) {
          this->direction = upr;
          upright();
        } else if (Xcord+1 == board.width && Ycord > 0){
          this->direction = downl;
          downleft();
        } else{
          this->direction = upl;
          upleft();
        }
        break;
    }
  }
};

class Paddle{
  private:
    std::atomic<int> Ycord_down = 9;
    int direction = -1; // static
  public:
    const int size = 5;
  inline int get_direction() {return direction;}
  inline int get_down() {return Ycord_down;}
  inline void up() {this->direction = 2;}
  inline void down() {this->direction = 3;}
  void update() {
    switch (direction) {
      case 2:
        if (Ycord_down+size-1 < board.height) {
          this->Ycord_down++;
        }
        break;
      case 3:
        if (Ycord_down-1 > 0) {
          this->Ycord_down--;
        }
        break;
    }
  }
};

void print_board(Ball& ball, Paddle& paddle) {
  using std::cout;
  system("clear");

  for(int y = board.height+1; y > 0; y--) {
    for(int x = 0; x <= board.width; x++) {
      // Printing '|' if it's paddle or right wall (only near a ball's Y location)
      if(y == board.height+1) {
        cout << "_";
      } else if(x == 0 && y >= paddle.get_down() && y <= paddle.get_down() + paddle.size-1){
        cout << "|";
      } else if (x == board.width && y >= ball.get_y()-1 && y <= ball.get_y()+1){
        cout << "|";
      } else if(x == ball.get_x() && y == ball.get_y()) {
        cout << ball_symbol;
      } else {cout << ' ';}
    }
    std::cout << std::endl;
  }
  for(int i = board.width; i > 0; i--) {cout << "-";}

}

void gameOver() {
  std::string text = "\e[1;31m _______  _______  _______  _______    _______           _______  _______ \n\e[1;31m(  ____ \\(  ___  )(       )(  ____ \\  (  ___  )|\\     /|(  ____ \\(  ____ )\n\e[1;31m| (    \\/| (   ) || () () || (    \\/  | (   ) || )   ( || (    \\/| (    )|\n\e[1;31m| |      | (___) || || || || (__      | |   | || |   | || (__    | (____)|\n\e[1;31m| | ____ |  ___  || |(_)| ||  __)     | |   | |( (   ) )|  __)   |     __)\n\e[1;31m| | \\_  )| (   ) || |   | || (        | |   | | \\ \\_/ / | (      | (\\ (   \n\e[1;31m| (___) || )   ( || )   ( || (____/\\  | (___) |  \\   /  | (____/\\| ) \\ \\__\n\e[1;31m(_______)|/     \\||/     \\|(_______/  (_______)   \\_/   (_______/|/   \\__/";
  system("clear");
  std::cout << text << '\n';
}

void setRawMode(bool enable);

void paddleControl(Paddle& paddle) {
  setRawMode(true);
  char key;
  while(read(STDIN_FILENO, &key, 1) == 1 && game_running) {
    if (key == 'q' || key == 3) {  // 3 = Ctrl+C
      game_running = false;
      break;
    }
    
    // Arrows processing (ANSI-codes: \x1B [A/B/C/D)
    if (key == '\x1B') {  // The start of ANSI sequence
        char seq[2];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) continue;
        if (read(STDIN_FILENO, &seq[1], 1) != 1) continue;
            
        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': paddle.up(); break;
                case 'B': paddle.down(); break;
            }
         }
      }
    continue;
    }
  setRawMode(false);
}

void gameVisual(Ball& ball, Paddle& paddle) {
  while (game_running) {
  std::this_thread::sleep_for(std::chrono::milliseconds(3));
    {
      std::lock_guard<std::mutex> lock(mtx);
      print_board(std::ref(ball), std::ref(paddle));
      std::cout << "⬆️ / ⬇️ / q | \e[38;5;93m Score: " << score << "\e[38;5;15m \n";
      if (DEBUG_MODE) {
      std::cout << "Ball direction: " << ball.direction << '\n';
      std::cout << "Ball X: " << ball.get_x() << '\n';
      std::cout << "Ball Y: " << ball.get_y() << '\n';
      }
    } 
  
  }
}

void gameLogic(Ball& ball, Paddle& paddle) {
  while (game_running) {
    paddle.update();
    ball.update();

    // Game over
    if(ball.get_x() == 1 && (ball.get_y() < paddle.get_down() || ball.get_y() > paddle.get_down()+paddle.size-1)) {
      game_running = false;
      gameOver();
      break;
    } else if (ball.get_x() == 1) {
      score++;
    }



    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // ~20 FPS
  }
}

int main() {
  using std::cout;
  using std::cin;
  using std::string;

  std::random_device rd; // device's enthropy
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> rand_dist(4, 7);
  int rand_roll = rand_dist(gen);

  Ball ball;
  ball.direction = rand_roll;
  Paddle paddle;

  std::thread pad_control(paddleControl, std::ref(paddle));
  std::thread game_visual(gameVisual, std::ref(ball), std::ref(paddle));
  std::thread game_logic(gameLogic, std::ref(ball), std::ref(paddle));

  pad_control.join();
  gameOver();
  return 0;
}

void setRawMode(bool enable) { 
  /* 
   * To be honest i have no idea what is going on here
   * But it makes my code able to read user input without Enter
   */ 
    static struct termios original;  
    if (enable) {
        tcgetattr(STDIN_FILENO, &original); 
        struct termios raw = original;
        raw.c_lflag &= ~(ICANON | ECHO | ISIG);  
        raw.c_cc[VMIN] = 1;   
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    } else {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
    }
}

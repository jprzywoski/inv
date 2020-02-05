/*
    ~ Shoot em up ~

    To compile:
      g++ -lSDL2 inv.cpp

    Controls:
      Left, Right, Space
*/

#include <SDL2/SDL.h>
#include <iostream>

struct Block
{
  double x;
  double y;
  int size;
};

bool collide(const Block &b1, const Block &b2);

struct Enemy
{
  Block b;
  bool alive;
  bool shotdown;
};

struct Player
{
  Block b;
};

struct State
{
  int w; // window width
  int h; // window height

  int stage; // level = 0, 1, 2 , ...

  static const int N = 100; // max number of enemies
  Enemy enemies[N];
  double global_time;

  Player player;
  Block bullet;
  bool can_shoot;
};

// init game state
void init_state(State &s, int stage);

// draw a block
void draw_block(SDL_Renderer *r, Block &b);
// render the state
void render(SDL_Renderer *renderer, State &s);

// run the game
void run(SDL_Renderer *renderer, State &s);

// update the game state
void update_state(State &s, double dt);

int main(int argc, char *args[])
{
  srand(time(NULL));

  // Game state
  State s;
  init_state(s, 0);

  SDL_Window *window;
  SDL_Renderer *rend;

  // Init SDL
  if (SDL_Init(SDL_INIT_EVERYTHING) == -1)
  {
    std::cout << " Failed to initialize SDL : " << SDL_GetError() << std::endl;
    exit(1);
  }

  // Create Window
  window =
      SDL_CreateWindow("Inv", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                       s.w, s.h, SDL_WINDOW_SHOWN);
  if (window == NULL)
  {
    std::cout << "Failed to create window : " << SDL_GetError();
    exit(1);
  }

  // Create Renderer
  rend = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (rend == NULL)
  {
    std::cout << "Failed to create renderer : " << SDL_GetError();
    exit(1);
  }

  // Setup Renderer
  // Set the size of renderer the same as the window
  SDL_RenderSetLogicalSize(rend, s.w, s.h);

  // Run the game
  run(rend, s);
}

void run(SDL_Renderer *rend, State &s)
{

  int time_prev = 0;
  int time = SDL_GetTicks();

  // main loop
  bool keep_running = true;
  while (keep_running)
  {

    time_prev = time;
    time = SDL_GetTicks();
    double dt = (time - time_prev) * 0.001; // in seconds

    // Process input
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_QUIT)
        keep_running = false;
      else if (event.type == SDL_KEYDOWN)
      {
        switch (event.key.keysym.sym)
        {
        case SDLK_ESCAPE:
          keep_running = false;
          break;
        }
      }
    }
    const Uint8 *keyboard_state = SDL_GetKeyboardState(NULL);
    // Move right
    if (keyboard_state[SDL_SCANCODE_RIGHT])
    {
      s.player.b.x += dt * 200.0;
      if (s.player.b.x > s.w - 1)
        s.player.b.x = s.w - 1;
    }
    // Move left
    if (keyboard_state[SDL_SCANCODE_LEFT])
    {
      s.player.b.x -= dt * 200.0;
      if (s.player.b.x < 0)
        s.player.b.x = 0;
    }
    // Shoot
    if (keyboard_state[SDL_SCANCODE_SPACE] && s.can_shoot)
    {
      s.can_shoot = false;
      s.bullet.x = s.player.b.x;
      s.bullet.y = s.player.b.y - s.player.b.size;
    }

    // Update the state
    update_state(s, dt);

    // Draw
    render(rend, s);

    SDL_Delay(2);

  } // end of the main loop
}

void init_state(State &s, int stage)
{
  s.w = 200;
  s.h = 300;
  s.player.b.size = 10;
  s.player.b.x = s.w / 2;
  s.player.b.y = s.h - 2 * s.player.b.size;

  s.bullet.x = 0;
  s.bullet.y = s.h * 2;
  s.bullet.size = 3;
  s.can_shoot = true;

  s.stage = stage;

  // Enemies
  s.global_time = 0.0;
  for (int i = 0; i < s.N; i++)
  {
    s.enemies[i].alive = false;
  }

  int enemy_size = 8;
  int ww = 4; // number of columns
  int hh = 4; // number of rows
  int shiftx = s.w / 2 - enemy_size * 4 * (ww - 1) / 2;
  int shifty = enemy_size * 2;
  for (int i = 0; i < ww; i++)
  {
    for (int j = 0; j < hh; j++)
    {
      int index = i + j * ww;
      s.enemies[index].alive = true;
      s.enemies[index].shotdown = false;
      s.enemies[index].b.x = i * enemy_size * 4 + shiftx;
      s.enemies[index].b.y = j * enemy_size * 4 + shifty;

      // varying size
      int size = enemy_size - (rand() % (stage + 1));
      if (size < enemy_size / 2)
        size = enemy_size / 2;
      s.enemies[index].b.size = size;
      // small shifts
      s.enemies[index].b.y += rand() % (enemy_size - size + 1) / 2;
      s.enemies[index].b.x +=
          rand() % (enemy_size - size + 1) / 2 * (rand() % 2 * 2 - 1);
    }
  }
}

bool collide(const Block &b1, const Block &b2)
{
  return (abs(b1.x - b2.x) < b1.size + b2.size) &&
         (abs(b1.y - b2.y) < b1.size + b2.size);
}

void update_state(State &s, double dt)
{
  // Bullet
  if (!s.can_shoot)
  {
    s.bullet.y -= 200.0 * dt;
    if (s.bullet.y < 0)
    {
      s.can_shoot = true;
      s.bullet.y = s.h * 2;
    }
  }

  // Move enemies - crazy movement
  s.global_time += dt;
  double t = s.global_time;
  double speed = (18.0 + 2.0 * s.stage) * sqrt(t);
  double freq = 6.0 + s.stage / 4;
  double z = 2.0 * sin(freq * t) + pow(sin(0.25 * freq * t), 19);

  double displace_x = speed * z * dt;
  double displace_y = 0.03 * speed * pow(z, 4) * dt;

  bool player_died = false;
  bool enemies_are_dead = true;

  for (int i = 0; i < s.N; i++)
  {
    if (s.enemies[i].alive)
    {

      if (s.enemies[i].shotdown)
      {
        // shotdown enemies
        s.enemies[i].b.y += 200.0 * dt;
        s.enemies[i].b.x +=
            100.0 * ((s.enemies[i].b.x < s.w / 2 ? -1 : 1) + 0.2 * z) * dt;
      }
      else
      {
        // normal enemies
        s.enemies[i].b.x += displace_x;
        s.enemies[i].b.y += displace_y;
      }

      // if collides with a bullet
      if (collide(s.enemies[i].b, s.bullet))
      {
        if (!s.enemies[i].shotdown)
          s.enemies[i].shotdown = true;

        s.can_shoot = true;
        s.bullet.y = s.h * 2;
      }

      // if collides with the player
      if (collide(s.enemies[i].b, s.player.b))
      {
        player_died = true;
      }

      // an enemy falls on the ground
      if (s.enemies[i].b.y > s.h + s.enemies[i].b.size)
      {
        s.enemies[i].alive = false;
      }

      enemies_are_dead = false;
    }
  }

  if (player_died)
  {
    int prev_stage = s.stage - 1;
    if (prev_stage < 0)
      prev_stage = 0;
    init_state(s, prev_stage);
  }

  if (enemies_are_dead)
  {
    int x = s.player.b.x;
    init_state(s, s.stage + 1);
    s.player.b.x = x;
  }
}

void draw_block(SDL_Renderer *rend, Block &b, int red, int green, int blue,
                int alpha)
{
  int x = static_cast<int>(b.x - b.size);
  int y = static_cast<int>(b.y - b.size);
  SDL_Rect r = {x, y, b.size * 2, b.size * 2};
  SDL_SetRenderDrawColor(rend, red, green, blue, alpha);
  SDL_RenderFillRect(rend, &r);
}

void render(SDL_Renderer *rend, State &s)
{
  SDL_SetRenderDrawColor(rend, 50, 50, 50, 255);
  SDL_RenderClear(rend);

  // Enemies
  for (int i = 0; i < s.N; i++)
  {
    if (s.enemies[i].alive)
    {
      if (s.enemies[i].shotdown)
        draw_block(rend, s.enemies[i].b, 160, 30, 30, 180);
      else
        draw_block(rend, s.enemies[i].b, 170, 0, 10, 255);
    }
  }

  // Player
  draw_block(rend, s.player.b, 255, 0, 0, 255);

  // Bullet
  draw_block(rend, s.bullet, 255, 0, 0, 255);

  // Stage number
  for (int i = 0; i < s.stage + 1; i++)
  {
    SDL_Rect r = {(i + 1) * 10, 10, 5, 5};
    SDL_SetRenderDrawColor(rend, 240, 170, 0, 200);
    SDL_RenderFillRect(rend, &r);
  }

  // Update the screen
  SDL_RenderPresent(rend);
}



#define MAX_SPEED 50     // maximaler speed (richtung 0)
#define SPEED_TRAP 0.67  // anteil von max_speed (wenn präzise hinter den ball fahren)

#define LIGHT_BARRIER_PIN 35

BohleBots g_bot = BohleBots();

Pixy2I2C g_pixy = Pixy2I2C();

int g_ball_direction = 0;
int g_compass;

bool g_sees_goal = false;
int g_goal_direction;
int g_goal_width;
int g_goal_distance;

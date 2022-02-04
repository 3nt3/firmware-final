
#include <CAN.h>

#include "Pixy2I2C.h"
#include "bohlebots.h"
#include "definitions.h"

// get_motor_ids spins each motor in the order of ids (1..4) for 4000ms each.
// used to identify pins since the fucking pcb is not documented.
void get_motor_ids() {
    for (int i = 1; i <= 4; i++) {
        Serial.printf("this is motor %d at speed %d\n", i, 100);
        g_bot.motor(i, 100);
        delay(4000);
        g_bot.motor(i, -100);
        delay(10);
        g_bot.motor(i, 0);
    }
}

void pixy_auswerten()  // wird nur aufgerufen, wenn die Pixy überhaupt etwas sieht
{
    int my_signature = 1;  // always use the first (=biggest) signature
    int sieht_farbe = g_pixy.ccc.blocks[0].m_signature;

    if (g_pixy.ccc.numBlocks > 0) {
        if (sieht_farbe == my_signature) {
            g_goal_direction = (g_pixy.ccc.blocks[0].m_x - 158) / 2;
            g_goal_width = g_pixy.ccc.blocks[0].m_width;
            g_goal_distance = pow(g_pixy.ccc.blocks[0].m_y - 80, 6) / 1000000;

            g_compass = -g_goal_direction;
            g_sees_goal = true;
            // topspeed=hightop;
            // seekspeed=seektop;
        } else  // soll hier überhaupt etwas geschehen?
        {
            g_goal_direction = g_compass / 8;

            Wire.beginTransmission(0x27);
            Wire.write(255 - 2);
            Wire.endTransmission();

            g_sees_goal = false;
            // topspeed=lowtop;
            // seekspeed=lowtop;
        }
    }

    // pixyzeit=0;
}

void pixy_read() {
    int i;
    // grab blocks!
    g_pixy.ccc.getBlocks();

    // If there are detect blocks, print them!
    if (sizeof(g_pixy.ccc.blocks) > 0) {
        pixy_auswerten();
    } else {
        g_goal_direction = -g_compass;
        // led_byte += 2;  // ledbyte um 2 erhöhen, wenn keine Pixy-Wert;
    }
}

void setup() {
    g_bot.set_bot_type(4);  // four wheels
    g_bot.init();
    g_bot.setze_kompass();  // resets the compass to 0

    // g_pixy.init();
    pinMode(35, INPUT);

    Serial.begin(115200);
    while (!Serial)
        ;  // do nothing until there is a serial connection available.

    Serial.printf("starting can bus.\n");
    if (!CAN.begin(500E3)) {
        Serial.printf("can bus failed to start. I will now do nothing forever :)\n");
        while (1)
            ;
    }  // start CAN bus at 500 kbps
    else {
        Serial.println("can bus started successfully.");
    }
}

void get_data() {
    g_bot.warte(10);
    int irData = 0;
    int ball_seen = 0;

    // light barrier
    // Serial.printf("yzzhdkl: %d\n", analogRead(LIGHT_BARRIER_PIN));

    CAN.beginPacket(0x03, 1, true);  // daten abfragen
    CAN.endPacket();

    while (!CAN.parsePacket()) delayMicroseconds(1);

    while (CAN.available()) {
        irData = CAN.read();
        g_ball_direction = (irData / 16) - 7;
        int zone = irData % 16;
        if (zone < 1)
            ball_seen = 0;
        else
            ball_seen = 1;
        if (ball_seen < 1) g_ball_direction = -8;
    }
    g_compass = g_bot.kompass();

    pixy_auswerten();
}

// direction_correction tries to adjust for error in the values received.
// maps (-n:n) to (-n+8:n-8) if that's understandable
int direction_correction(const int &d) {
    if (d >= -3 && d <= 4) return d;
    if (d < -3) return d + 8;
    if (d > 4) return d - 8;
}

// returns a side (forwards, left, right) for a given direction in the format no one understands
int side(const int &d) {
    if (d % 4 == 0) return 0;
    if (d > 0) return 1;
    if (d < 0) return -1;
}

// fuckery no one understands
//
// ERFAHRUNG
void action() {
    // stop fahring
    // if ball not seen
    if (g_ball_direction == -8) {
        fahre_corrected(0, 0, 0);
        return;
    }
    // set variable
    int turn = 0;
    // pixy
    if (g_sees_goal) turn = g_goal_direction;
    // else kompass
    else
        turn = -g_bot.kompass() / 6;

    // if ball in ballschale
    if (g_ball_direction == 0)
        // fahr in torrichtung
        fahre_corrected(0, MAX_SPEED, turn);
    else {
        // locate ball with IR-Ring
        int dir = ((g_ball_direction + 1) / 2) + (1 * side(g_ball_direction));
        // correct with ERFAHRUNG
        dir = direction_correction(dir);
        // fahr in ball direction
        fahre_corrected(dir, MAX_SPEED * SPEED_TRAP, turn);
    }
}

void debug_SerialOutput() {
    Serial.println("//////////////////////////////////////////////");
    Serial.println("ballDir:   " + String(g_ball_direction));
    Serial.println("sees goal: " + String(g_sees_goal));
    Serial.println("goalDir:   " + String(g_goal_direction));
    Serial.println("goalDist:  " + String(g_goal_distance));
    Serial.println("goalWidth: " + String(g_goal_width));
    Serial.printf("compass:   %d\n", g_bot.kompass());
    Serial.println("//////////////////////////////////////////////");
    Serial.println();
}

int cnt = 0;

void fahre_corrected(int direction, int speed, int turn) {
    g_bot.fahre(direction, speed, turn - (speed * 0.1));
}

void debugOutput(const int &n) {
    if (n <= 0) return;
    ++cnt;
    if (cnt >= n) {
        debug_SerialOutput();
        cnt = 0;
    }
}

void loop() {
    get_data();
    action();
    debugOutput(100);
}

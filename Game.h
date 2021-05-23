//
// Created by mateusz on 20.05.2021.
//

#ifndef ZADANIE_2_GAME_H
#define ZADANIE_2_GAME_H


#include <cstdint>
#include <complex>
#include <vector>
#include <algorithm>
#include "Messages.h"
#include "Random.h"

enum TurnDirection {
    STRAIGHT = 0, RIGHT = 1, LEFT = 2
};

class Player {
    std::string playerName;
    TurnDirection enteredDirection;
public:
    Player(const std::string &playerName, uint32_t number, TurnDirection direction) : playerName(playerName),
                                                                                      enteredDirection(direction) {

    }

    void setDirection(TurnDirection direction) {
        enteredDirection = direction;
    }

    [[nodiscard]] TurnDirection getEnteredDirection() const {
        return enteredDirection;
    }

    bool operator<(const Player &rhs) const {
        int compare = playerName.compare(rhs.playerName);
        return (compare < 0);
    }

    bool operator==(const Player &rhs) const {
        return playerName == rhs.playerName;
    }

    const std::string &getName() const {
        return playerName;
    }
};

class Worm {
    uint16_t direction; // Number from 0 to 360

    double x, y;
    TurnDirection recentTurn;

    static double degreesToRadians(uint16_t degrees) {
        static const double halfC = M_PI / 180;
        return double(degrees) * halfC;
    }

public:
    static const int N_DEGREES = 360;

    Worm() = default; // TODO co z tym, potrzebne by para zadziałała.

    Worm(double x, double y, uint16_t direction, TurnDirection turn) : x(x),
                                                                       y(y),
                                                                       direction(direction),
                                                                       recentTurn(turn) {}

    void move() {
        x += std::cos(degreesToRadians(direction));
        y += std::sin(degreesToRadians(direction));
    }

    void turn(int turningSpeed) {
        if (recentTurn == TurnDirection::RIGHT) {
            direction += turningSpeed;
        } else if (recentTurn == TurnDirection::LEFT) {
            direction -= turningSpeed;
        }
        direction %= N_DEGREES;
    }

    std::pair<uint32_t, uint32_t> getPixel() {
        return {x, y};
    }

};

class Game {
    int turningSpeed;
    uint32_t width, height;
    uint32_t gameID;
    int eventCount;
    std::map<Player, std::pair<uint32_t, Worm>> activePlayers;
    std::vector<std::vector<bool>> pixels; // false if has not been eaten

    bool isDead(uint32_t x, uint32_t y) {
        return pixels[x][y];
    }

    std::vector<std::string> generateNamesVector(const std::vector<Player> &players) {
        std::vector<std::string> ret(players.size());
        for (size_t i = 0; i < players.size(); i++) {
            ret[i] = players[i].getName();
        }
        return ret;
    }

public:
    Game(uint32_t width, uint32_t height, int turningSpeed) : width(width), height(height), eventCount(0),
                                            pixels(width, std::vector<bool>(height, false)), turningSpeed(turningSpeed) {};

    std::vector<Record> startNewGame(std::vector<Player> &gamePlayers, Random &random) {
        std::sort(gamePlayers.begin(), gamePlayers.end());
        std::vector<Record> records;
        gameID = random.generate();

        auto newGameEvent = std::make_shared<NewGameData>(NewGameData(width, height,generateNamesVector(gamePlayers)));
        records.emplace_back(Record(eventCount++, newGameEvent));
        for (size_t i = 0; i < gamePlayers.size(); i++) {

            double x = (random.generate() % width) + 0.5;
            double y = (random.generate() % height) + 0.5;
            if (isDead(x, y)) {
                records.emplace_back(
                        Record(eventCount++, std::make_shared<PlayerEliminatedData>(PlayerEliminatedData(i))));
            } else {
                records.emplace_back(
                        Record(eventCount++, std::make_shared<PixelEventData>(PixelEventData(i, x, y))));
                uint16_t direction = random.generate() % Worm::N_DEGREES;
                TurnDirection turnDirection = gamePlayers[i].getEnteredDirection();
                activePlayers[gamePlayers[i]] = std::pair<uint32_t, Worm>(i, Worm(x, y, direction, turnDirection));
                pixels[x][y] = true;
            }
        }
        return records;
    }

    auto doRound() {
        std::vector<Record> records;
        for(auto it = activePlayers.begin(), nextIt = it; it != activePlayers.end(); it = nextIt) {
            nextIt = it++;
            Worm &worm = it->second.second;
            auto pixelBefore = worm.getPixel();
            worm.turn(turningSpeed);
            worm.move();
            auto pixelNow = worm.getPixel();
            if (pixelBefore.first == pixelNow.first && pixelBefore.second == pixelNow.second) {
                // TODO sprawdziź czy można po prostu porównać pary.
                continue;
            }
            if (isDead(pixelNow.first, pixelNow.second)) {
                records.emplace_back(
                        Record(eventCount++, std::make_shared<PlayerEliminatedData>(PlayerEliminatedData(it->second.first))));
                activePlayers.erase(it);
            } else {
                pixels[pixelNow.first][pixelNow.second] = true;
                auto pixelEvent = std::make_shared<PixelEventData>(PixelEventData(it->second.first, pixelNow.first, pixelNow.second));
                records.emplace_back(Record(eventCount++, pixelEvent));
            }
        }
        return records;
    }

};

#endif //ZADANIE_2_GAME_H

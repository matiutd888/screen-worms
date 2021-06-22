//
// Created by mateusz on 20.05.2021.
//

#ifndef ZADANIE_2_GAME_H
#define ZADANIE_2_GAME_H

#include <cstdint>
#include <complex>
#include <vector>
#include <algorithm>
#include <iostream>
#include "Messages.h"
#include "Random.h"
#include <map>


enum TurnDirection {
    STRAIGHT = 0, RIGHT = 1, LEFT = 2
};

class Player {
    std::string playerName;
    TurnDirection enteredDirection;
    bool isReady;
    uint64_t sessionID;
public:
    Player(const std::string &playerName, TurnDirection direction, uint64_t sessionID) : playerName(playerName),
                                                                                      enteredDirection(direction),
                                                                                      sessionID(sessionID){
        if (direction != TurnDirection::STRAIGHT)
            isReady = true;
        else
            isReady = false;
    }

    uint64_t getSessionId() const {
        return sessionID;
    }

    friend std::ostream &operator<<(std::ostream &os, const Player &player) {
        os << "playerName: " << player.playerName;
        return os;
    }

    void setReady(bool isReady) {
        this->isReady = isReady;
    }

    void setDirection(TurnDirection direction);

    [[nodiscard]] TurnDirection getEnteredDirection() const {
        return enteredDirection;
    }

    bool isObserver() const {
        return playerName.empty();
    }

    bool isPlayerReady() const {
        return !isObserver() && isReady;
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
    Player() = default;
};

class Worm {
    double x;
    double y;
    int direction; // Number from 0 to 360
    TurnDirection recentTurn;

    static double degreesToRadians(uint16_t degrees) {
        static const double halfC = M_PI / 180;
        return double(degrees) * halfC;
    }

public:
    static const int N_DEGREES = 360;

    Worm() = default;

    Worm(double x, double y, int direction, TurnDirection turn) : x(x),
                                                                       y(y),
                                                                       direction(direction),
                                                                       recentTurn(turn) {}

    void move() {
        x += std::cos(degreesToRadians(direction));
        y += std::sin(degreesToRadians(direction));
        std::cout << "(x, y) = (" << x << ", " << y << "), direction = " << direction << std::endl; // DODANE
    }

    void turn(int turningSpeed) {
        if (recentTurn == TurnDirection::RIGHT) {
            direction += turningSpeed;
        } else if (recentTurn == TurnDirection::LEFT) {
            direction -= turningSpeed;
        }
        while(direction < 0) {
			direction += N_DEGREES;
		}
    }

    std::pair<uint32_t, uint32_t> getPixel() const {
        return {x, y};
    }
    
    std::pair<double, double> getCoordinates() const {
		return {x, y};
	}

    void setRecentTurn(TurnDirection recentTurn) {
        Worm::recentTurn = recentTurn;
    }

};

class Game {
    static inline const std::string TAG = "Game: ";
    uint32_t width, height;
    int eventCount;
    uint32_t gameID;
    std::vector<std::vector<bool>> pixels; // false if has not been eaten
    int turningSpeed;
    bool isGameRightNow;
    std::map<Player, std::pair<uint32_t, Worm>> activePlayers;

    bool isDead(double x, double y) {
        return (x >= width || y >= height || x < 0 || y < 0) || pixels[uint32_t(x)][uint32_t(y)];
    }

    static std::vector<std::string> generateNamesVector(const std::vector<Player> &players) {
        std::vector<std::string> ret(players.size());
        for (size_t i = 0; i < players.size(); i++) {
            ret[i] = players[i].getName();
        }
        return ret;
    }

    void clearBoard() {
        for (auto &it_v : pixels) {
            for(size_t i = 0; i < it_v.size(); i++) {
                it_v[i] = false;
            }
        }
    }

    void gameOver() {
        debug_out_1 << TAG << "Game Over!, won " << activePlayers.begin()->first << std::endl;
        isGameRightNow = false;
        clearBoard();
    }

    bool checkForGameEnd() {
        return activePlayers.size() == 1;
    }
public:

    Game(uint32_t width, uint32_t height, int turningSpeed) : width(width), height(height), eventCount(0),
                                                              pixels(width, std::vector<bool>(height, false)), turningSpeed(turningSpeed), isGameRightNow(false) {};

    std::vector<Record> startNewGame(std::vector<Player> &gamePlayers, Random &random);

    std::vector<Record> doRound();

    bool isGameNow() const {
        return isGameRightNow;
    }

    void updatePlayer(const Player &player);

    uint32_t getGameId() const {
        return gameID;
    }
};

#endif //ZADANIE_2_GAME_H

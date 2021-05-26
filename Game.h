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

    void setDirection(TurnDirection direction) {
        enteredDirection = direction;
        if (direction != TurnDirection::STRAIGHT) {
            isReady = true;
        }
    }

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
    Player() = default; // TODO naprawić to
};

class Worm {
    int direction; // Number from 0 to 360

    double x, y;
    TurnDirection recentTurn;

    static double degreesToRadians(uint16_t degrees) {
        static const double halfC = M_PI / 180;
        return double(degrees) * halfC;
    }

public:
    static const int N_DEGREES = 360;

    Worm() = default; // TODO co z tym, potrzebne by para zadziałała.

    Worm(double x, double y, int direction, TurnDirection turn) : x(x),
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

    void setRecentTurn(TurnDirection recentTurn) {
        Worm::recentTurn = recentTurn;
    }

};

class Game {
    static inline const std::string TAG = "Game: ";
    int turningSpeed;
    uint32_t width, height;
    uint32_t gameID;
    int eventCount;
    std::map<Player, std::pair<uint32_t, Worm>> activePlayers;
    std::vector<std::vector<bool>> pixels; // false if has not been eaten
    bool isGameRightNow;

    bool isDead(uint32_t x, uint32_t y) {
        return (x >= width || y >= height) || pixels[x][y];
    }

    std::vector<std::string> generateNamesVector(const std::vector<Player> &players) {
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
        std::cout << TAG << "Game Over!, won " << activePlayers.begin()->first << std::endl;
        isGameRightNow = false;
        clearBoard();
    }

    bool checkForGameEnd() {
        return activePlayers.size() == 1;
    }
public:

    Game(uint32_t width, uint32_t height, int turningSpeed) : width(width), height(height), eventCount(0),
                                                              pixels(width, std::vector<bool>(height, false)), turningSpeed(turningSpeed), isGameRightNow(
                    false) {};

    std::vector<Record> startNewGame(std::vector<Player> &gamePlayers, Random &random) {
        clearBoard();
        isGameRightNow = true;
        std::sort(gamePlayers.begin(), gamePlayers.end());
        std::vector<Record> records;
        gameID = random.generate();
        std::cout << "GAME: " << " generated GAme ID = " << gameID << std::endl;
        eventCount = 0;
        auto newGameEvent = std::make_shared<NewGameData>(NewGameData(width, height,generateNamesVector(gamePlayers)));
        records.emplace_back(Record(eventCount++, newGameEvent));
        std::cout << "GAME: GENERATING NEW_GAME, " << eventCount <<std::endl;
        for (size_t i = 0; i < gamePlayers.size(); i++) {
            std::cout << "Player " << i << gamePlayers[i] << std::endl;
            double x = (random.generate() % width) + 0.5;
            double y = (random.generate() % height) + 0.5;
            if (isDead(x, y)) {
                std::cout << "Eliminating Player " << gamePlayers[i] << std::endl;

                records.emplace_back(
                        Record(eventCount++, std::make_shared<PlayerEliminatedData>(PlayerEliminatedData(i))));
                if (checkForGameEnd()) {
                    std::cout << "GAME: " << "PUSHING GAME OVER! " << eventCount << std::endl;
                    records.emplace_back(
                            Record(eventCount++, std::make_shared<GameOver>(GameOver()))
                    );
                    gameOver();
                    return records;
                }
            } else {
                std::cout << "x, y = " << x<< " " << y << std::endl;
                records.emplace_back(
                        Record(eventCount++, std::make_shared<PixelEventData>(PixelEventData(i, x, y))));
                int direction = random.generate() % Worm::N_DEGREES;
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
            ++nextIt;
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
                std::cout << "Eliminating Player " << it->first << std::endl;
                records.emplace_back(
                        Record(eventCount++, std::make_shared<PlayerEliminatedData>(PlayerEliminatedData(it->second.first))));
                activePlayers.erase(it);
                if (checkForGameEnd()) {
                    std::cout << "GAME: " << "PUSHING GAME OVER! " << eventCount << std::endl;
                    records.emplace_back(
                            Record(eventCount++, std::make_shared<GameOver>(GameOver()))
                            );
                    gameOver();

                    return records;
                }
            } else {
                pixels[pixelNow.first][pixelNow.second] = true;
                auto pixelEvent = std::make_shared<PixelEventData>(PixelEventData(it->second.first, pixelNow.first, pixelNow.second));
                records.emplace_back(Record(eventCount++, pixelEvent));
            }
        }
        return records;
    }

    bool isGameNow() const {
        return isGameRightNow;
    }

    void updatePlayer(const Player &player) {
        auto it = activePlayers.find(player);
        if (it == activePlayers.end()) {
            debug_out_1 << "Player " << player << " not found!" << std::endl;
            return;
        }
        const Player &p = it->first;
        if (p.getSessionId() != player.getSessionId()) {
            debug_out_1 << "Player " << player << " differs with session ID!" << std::endl;
            return;
        }
        Worm &worm = it->second.second;
        worm.setRecentTurn(player.getEnteredDirection());
    }

    uint32_t getGameId() const {
        return gameID;
    }
};

#endif //ZADANIE_2_GAME_H

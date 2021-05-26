//
// Created by mateusz on 20.05.2021.
//

#include "Game.h"

void Game::updatePlayer(const Player &player) {
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

std::vector<Record> Game::doRound() {
    std::vector<Record> records;
    for(auto it = activePlayers.begin(), nextIt = it; it != activePlayers.end(); it = nextIt) {
        ++nextIt;
        Worm &worm = it->second.second;
        auto pixelBefore = worm.getPixel();
        worm.turn(turningSpeed);
        worm.move();
        auto pixelNow = worm.getPixel();
        if (pixelBefore.first == pixelNow.first && pixelBefore.second == pixelNow.second) {
            continue;
        }
        if (isDead(pixelNow.first, pixelNow.second)) {
            debug_out_1 << "Eliminating Player " << it->first << std::endl;
            records.emplace_back(
                    Record(eventCount++, std::make_shared<PlayerEliminatedData>(PlayerEliminatedData(it->second.first))));
            activePlayers.erase(it);
            if (checkForGameEnd()) {
                debug_out_1 << "GAME: " << "PUSHING GAME OVER! " << eventCount << std::endl;
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

std::vector<Record> Game::startNewGame(std::vector<Player> &gamePlayers, Random &random) {
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

void Player::setDirection(TurnDirection direction) {
    enteredDirection = direction;
    if (direction != TurnDirection::STRAIGHT) {
        isReady = true;
    }
}

#ifndef STUDENTWORLD_H_
#define STUDENTWORLD_H_

#include "GameWorld.h"
#include "Board.h"
#include <string>

// Students:  Add code to this file, StudentWorld.cpp, Actor.h, and Actor.cpp
#include <vector>
#include <set>
class Actor;
class Player;

class StudentWorld : public GameWorld
{
public:
    StudentWorld(std::string assetPath);
    virtual int init();
    virtual int move();
    virtual void cleanUp();
    
    // NON TRIVIAL FUNCTIONS
    ~StudentWorld();
    void getPlayersOnSquare(std::set<Player*>& playersOnSquare, int x, int y);
    void sortActors();
    Actor* getImpactableCollidingWith(Actor* actor);
    Actor* getRandomSquare(int x, int y); // Excludes x and y as a valid random square
    Actor* getSquareAt(int x, int y);
    std::string getGameStatusText() const;
    bool isOverlapping(Actor* a1, Actor* a2);
    bool isDirectionalSquare(int x, int y);
    
    // TRIVIAL FUNCTIONS
    void addActor(Actor* actor) { m_actors.push_back(actor); }
    int getBank() const { return m_bank; }
    void setBank(int coins) { m_bank = coins; }
    bool isEmptySquare(int x, int y) { return m_board->getContentsOf(x / SPRITE_WIDTH, y / SPRITE_HEIGHT) == Board::empty; }
    Player* getOtherPlayer(Player* player) { return (player == m_peach ? m_yoshi : m_peach); };
    
private:
    std::vector<Actor*> m_actors;
    Board* m_board;
    Player* m_peach; // keep players separate so it's easier to get their individual stats
    Player* m_yoshi;
    int m_bank;
};

#endif // STUDENTWORLD_H_
